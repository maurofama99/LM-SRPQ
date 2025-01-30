#pragma once
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#define max(x, y) (x>y?x:y)
using namespace std;

// this streaming graph class suppose that there may be duplicate edges in the streaming graph. It means the same edge (s, d, label) may appear multiple times in the stream.

class sg_edge;

struct timed_edge
        // this is the structure to maintain the time sequence list. It stores the tuples in the stream with time order, each tuple is an edge;
{
    timed_edge *next;
    timed_edge *prev; // the two pointers to maintain the list;
    sg_edge *edge_pt; // information of the tuple, including src, dst, edge label and timestamp;
    explicit timed_edge(sg_edge *edge_pt_ = nullptr) {
        edge_pt = edge_pt_;
        next = nullptr;
        prev = nullptr;
    }
};

struct edge_info // the structure as query result, include all the information of an edge;
{
    unsigned int s, d;
    int label;
    unsigned int timestamp;
    unsigned int expiration_time;
    edge_info(unsigned int src, unsigned int dst, unsigned int time, int label_, unsigned int expiration_time_) {
        s = src;
        d = dst;
        timestamp = time;
        expiration_time = expiration_time_;
        label = label_;
    }
};

class sg_edge {
public:
    int label;
    unsigned int timestamp;
    unsigned int expiration_time{};
    timed_edge *time_pos;
    unsigned int s, d;

    sg_edge(const unsigned int src, const unsigned int dst, const int label_, const unsigned int time) {
        s = src;
        d = dst;
        timestamp = time;
        label = label_;
        time_pos = nullptr;
    }
};


class streaming_graph {
public:
    unordered_map<unsigned int, vector<pair<unsigned int, sg_edge *> > > adjacency_list;

    int edge_num; // number of edges in the window
    timed_edge *time_list_head; // head of the time sequence list;
    timed_edge *time_list_tail; // tail of the time sequence list

    double mean = 0;
    double m2 = 0;
    unordered_map<unsigned int, int> density;

    double zscore_threshold = 1.5;
    int slide_threshold = 100;
    int saved_edges = 0;
    int slide;

    explicit streaming_graph() {
        edge_num = 0;
        time_list_head = nullptr;
        time_list_tail = nullptr;
    }

    ~streaming_graph() {
        // Free memory for all edges in the adjacency list
        for (auto& [_, edges] : adjacency_list) {
            for (auto& [_, edge] : edges) {
                delete edge;
            }
        }

        // Free memory for the timed edges list
        while (time_list_head) {
            timed_edge* temp = time_list_head;
            time_list_head = time_list_head->next;
            delete temp;
        }
    }

    void add_timed_edge(timed_edge *cur)
    // add an edge to the time sequence list. As edges arrive in time order, it will be added to the tail.
    {
        cur->next = nullptr;
        cur->prev = time_list_tail;
        if (!time_list_head) // if the list is empty, then this is the head.
            time_list_head = cur;
        if (time_list_tail)
            time_list_tail->next = cur;
        time_list_tail = cur; // cur is the new tail
    }

    void delete_timed_edge(timed_edge *cur) // delete an edge from the time list;
    {
        if (time_list_head == cur)
            time_list_head = cur->next;
        if (time_list_tail == cur)
            time_list_tail = cur->prev;
        if (cur->prev)
            (cur->prev)->next = cur->next; // disconnect it with its precursor and successor
        if (cur->next)
            (cur->next)->prev = cur->prev;
    }

    bool insert_edge(unsigned int from, unsigned int to, int label, unsigned int timestamp,
                         unsigned int expiration_time) {
        timed_edge *t_edge;

        // Check if the edge already exists in the adjacency list
        for (auto &[to_vertex, existing_edge]: adjacency_list[from]) {
            if (existing_edge->label == label && to_vertex == to) {
                existing_edge->timestamp = timestamp; // Update the timestamp if the edge exists
                existing_edge->expiration_time = expiration_time;
                delete_timed_edge(existing_edge->time_pos);

                // Update the time sequence list, ensuring the list remains sorted by timestamp
                t_edge = new timed_edge(existing_edge);
                add_timed_edge(t_edge);
                existing_edge->time_pos = t_edge;

                return false;
            }
        }

        auto *edge = new sg_edge(from, to, label, timestamp);

        edge->expiration_time = expiration_time;
        // Add the edge to the adjacency list if it doesn't exist
        if (adjacency_list.find(from) == adjacency_list.end()) {
            adjacency_list[from] = vector<pair<unsigned int, sg_edge *> >();
        }
        adjacency_list[from].emplace_back(to, edge);

        // update z score
        density[from]++;
        // cout << "density: " << density[from] << ", z_score: " << get_zscore(from) << endl;
        edge_num++;
        if (edge_num == 1) {
            mean = density[from];
            m2 = 0;
        } else {
            double old_mean = mean;
            mean += (density[from] - mean) / edge_num;
            m2 += (density[from] - old_mean) * (density[from] - mean);
        }

        t_edge = new timed_edge(edge);
        add_timed_edge(t_edge);
        edge->time_pos = t_edge;

        return true;
    }

    bool remove_edge(const sg_edge *edge_to_delete) {
        const unsigned int from = edge_to_delete->s;
        const unsigned int to = edge_to_delete->d;
        const int label = edge_to_delete->label;

        // Check if the vertex exists in the adjacency list
        if (adjacency_list.find(from) == adjacency_list.end()) {
            return false; // Edge doesn't exist
        }

        auto &edges = adjacency_list[from];
        for (auto it = edges.begin(); it != edges.end(); ++it) {
            sg_edge *edge = it->second;

            // Check if this is the edge to remove
            if (it->first == to && edge->label == label) {

                // Remove the edge from the adjacency list
                edges.erase(it);

                double old_mean = mean;
                mean -= (mean - density[from]) / (edge_num - 1);
                m2 -= (density[from] - old_mean) * (density[from] - mean);

                density[from]--;
                edge_num--;

                // If the vertex has no more edges, remove it from the adjacency list
                if (edges.empty()) {
                    adjacency_list.erase(from);
                }

                return true; // Successfully removed
            }
        }
        return false; // Edge not found
    }

    /**
     * @param timestamp current time
     * @param deleted_edges return vector with deleted edges
     */
    vector<sg_edge*> expire(unsigned int timestamp, vector<edge_info> &deleted_edges)
    // this function is used to find all expired edges and remove them from the graph.
    {
        vector<sg_edge*> significant_edges;
        int evicted = 0;

        saved_edges = 0;
        while (time_list_head) {
            sg_edge *cur_edge = time_list_head->edge_pt;
            /*
            if (cur->timestamp + window_size >= timestamp) // The later edges are still in the sliding window, and we can stop the expiration.
                break;
            */
            if (cur_edge->expiration_time > timestamp) {
                // The later edges are still in the sliding window, and we can stop the expiration.
                // cout << "evicted " << evicted << ", saved_edges " << saved_edges << endl;
                break;
            }
            evicted++;

            if (timestamp - cur_edge->timestamp <= slide*slide_threshold and get_zscore(cur_edge->s) > zscore_threshold) {
                // cout << get_zscore(cur_edge->s) << endl;
                saved_edges++;
                auto saved_edge = new sg_edge(cur_edge->s, cur_edge->d, cur_edge->label, timestamp);
                significant_edges.emplace_back(saved_edge);
            }

            // if ( cur_edge->timestamp % 3600 == 0) cout <<"evict element at t " << cur_edge->timestamp /3600 << " ,evict time: " << timestamp /3600<< endl;
            deleted_edges.emplace_back(cur_edge->s, cur_edge->d, cur_edge->timestamp, cur_edge->label,
                                       cur_edge->expiration_time);
            remove_edge(cur_edge);

            // we record the information of expired edges. This information will be used to find expired tree nodes in S-PATH or LM-SRPQ.
            timed_edge *tmp = time_list_head;
            time_list_head = time_list_head->next; // delete the time sequence list unit
            if (time_list_head)
                time_list_head->prev = nullptr;
            delete tmp;
            delete cur_edge;
        }
        // cout << "extended: " << extended << " vs expired: " << expired << endl;
        if (!time_list_head) // if the list is empty, the tail pointer should also be NULL;
            time_list_tail = nullptr;

        return significant_edges;
    }

    void get_timed_all_suc(unsigned int s, vector<edge_info> &sucs) {
        if (adjacency_list.find(s) == adjacency_list.end()) {
            return; // No outgoing edges for vertex s
        }

        for (const auto &[to, edge]: adjacency_list[s]) {
            sucs.emplace_back(s, to, edge->timestamp, edge->label, edge->expiration_time);
        }
    }

    void get_src_degree(unsigned int s, map<unsigned int, unsigned int> &degree_map) {
        if (adjacency_list.find(s) == adjacency_list.end()) {
            return; // No outgoing edges for vertex s
        }

        for (const auto &[_, edge]: adjacency_list[s]) {
            degree_map[edge->label]++;
        }
    }

    double get_zscore(unsigned int vertex) {
        double variance = m2 / edge_num;
        double std_dev = sqrt(variance);

        if (std_dev == 0) return 0;

        return (density[vertex] - mean) / std_dev;
    }
};
