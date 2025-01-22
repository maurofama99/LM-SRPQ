#pragma once
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <set>
#include <algorithm>
#include <queue>
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

// TODO Propagated expiration time in the edge infos
struct edge_info // the structure as query result, include all the information of an edge;
{
    unsigned int s, d;
    int label;
    unsigned int timestamp;
    unsigned int expiration_time;
    // TODO remove expiration time to make it work for every algorithm, with exp working only S-PATH
    edge_info(unsigned int src, unsigned int dst, unsigned int time, int label_, unsigned int expiration_time_) {
        s = src;
        d = dst;
        timestamp = time;
        expiration_time = expiration_time_;
        label = label_;
    }
};

// TODO Added expiration timestamp
class sg_edge {
public:
    int label;
    unsigned int timestamp;
    unsigned int expiration_time{};
    timed_edge *time_pos;


    unsigned int s, d;
    sg_edge *src_prev;
    sg_edge *src_next; // cross list, maintaining the graph structure
    sg_edge *dst_next;
    sg_edge *dst_prev;

    sg_edge(unsigned int src, unsigned int dst, unsigned int label_, unsigned int time) {
        s = src;
        d = dst;
        timestamp = time;
        label = label_;
        src_next = nullptr;
        src_prev = nullptr;
        dst_next = nullptr;
        dst_prev = nullptr;
        time_pos = nullptr;
    }

    ~sg_edge()
    = default;
};

struct neighbor_list // this struct serves as the head pointed of the neighbor list of a vertex
{
    sg_edge *list_head;

    explicit neighbor_list(sg_edge *head = nullptr) {
        list_head = head;
    }
};


class streaming_graph {
public:
    unordered_map<unsigned int, neighbor_list> g; // successor list
    // unordered_map<unsigned int, neighbor_list> rg; // precursor list

    unordered_map<unsigned int, vector<pair<unsigned int, sg_edge *> > > adjacency_list;

    int edge_num; // number of edges in the window
    timed_edge *time_list_head; // head of the time sequence list;
    timed_edge *time_list_tail; // tail of the time sequence list


    explicit streaming_graph(int w) {
        edge_num = 0;
        time_list_head = nullptr;
        time_list_tail = nullptr;
    }

    ~streaming_graph() {
        unordered_map<unsigned int, neighbor_list>::iterator it;
        for (it = g.begin(); it != g.end(); it++) // delete the edges
        {
            sg_edge *tmp = it->second.list_head;
            sg_edge *parent = tmp;
            while (tmp) {
                parent = tmp;
                tmp = tmp->src_next;
                delete parent;
            }
        }
        g.clear();
        // rg.clear();
        // each edge only has one copy, liked by cross lists. Thus we only need to scan the successor lists to delete all edges;

        // Free memory for all edges in the adjacency list
        for (auto &[_, edges]: adjacency_list) {
            for (auto &[_, edge]: edges) {
                delete edge;
            }
        }

        // Free memory for the timed edges list
        while (time_list_head) {
            timed_edge *temp = time_list_head;
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

    bool insert_edge_new(unsigned int from, unsigned int to, int label, unsigned int timestamp,
                     unsigned int expiration_time) {
        sg_edge *edge;

        bool edge_exists = false;
        // Check if the edge already exists in the adjacency list
        for (auto &[to_vertex, existing_edge]: adjacency_list[from]) {
            if (existing_edge->label == label && to_vertex == to) {
                existing_edge->timestamp = timestamp; // Update the timestamp if the edge exists
                delete_timed_edge(existing_edge->time_pos);
                edge = existing_edge;
                edge_exists = true;
                break;
            }
        }

        if (!edge_exists) {
            edge = new sg_edge(from, to, label, timestamp);
            edge_num++;
            edge->expiration_time = expiration_time;
            // Add the edge to the adjacency list if it doesn't exist
            if (adjacency_list.find(from) == adjacency_list.end()) {
                adjacency_list[from] = vector<pair<unsigned int, sg_edge *> >();
            }
            adjacency_list[from].emplace_back(to, edge);
        }

        // Update the time sequence list, ensuring the list remains sorted by timestamp
        auto *t_edge = new timed_edge(edge);

        if (!time_list_head) {
            // If the time sequence list is empty, just set the head and tail
            time_list_head = t_edge;
            time_list_tail = t_edge;
        } else {
            timed_edge *current = time_list_head;
            while (current && current->edge_pt->timestamp <= timestamp) {
                current = current->next;
            }

            if (!current) {
                // Insert at the end of the time sequence list
                time_list_tail->next = t_edge;
                t_edge->prev = time_list_tail;
                time_list_tail = t_edge;
            } else if (current == time_list_head) {
                // Insert at the head if timestamp is the smallest
                t_edge->next = time_list_head;
                time_list_head->prev = t_edge;
                time_list_head = t_edge;
            } else {
                // Insert in the middle of the time sequence list
                timed_edge *prev_node = current->prev;
                prev_node->next = t_edge;
                t_edge->prev = prev_node;
                t_edge->next = current;
                current->prev = t_edge;
            }
        }
        return !edge_exists;
    }


    // todo adding exp
    bool insert_edge_extended(int s, int d, int label, int timestamp, unsigned exp)
    // insert an edge in the streaming graph, bool indicates if it is a new edge (not appear before)
    {
        auto *cur = new timed_edge;
        // create a new timed edge and insert it to the tail of the time sequence list, as in a streaming graph the inserted edge is always the latest
        add_timed_edge(cur);

        auto it = g.find(s);

        if (it != g.end()) {
            sg_edge *tmp = it->second.list_head;
            while (tmp) {
                if (tmp->label == label && tmp->d == d)
                // If we find the edge in the cross list, this is not a new edge, and we only update its timestamp and pointer to the time sequence list;
                {
                    delete_timed_edge(tmp->time_pos);
                    tmp->time_pos = cur;
                    cur->edge_pt = tmp;
                    tmp->timestamp = timestamp;
                    tmp->expiration_time = exp;
                    return false;
                }
                tmp = tmp->src_next;
            }
            edge_num++; // if not appears before
            tmp = new sg_edge(s, d, label, timestamp); // create a new sg_edge
            tmp->expiration_time = exp;
            tmp->time_pos = cur;
            cur->edge_pt = tmp;
            tmp->src_next = it->second.list_head; // add it to the front of the successor list of the src node
            if (it->second.list_head)
                it->second.list_head->src_prev = tmp;
            it->second.list_head = tmp;
            /*auto it2 = rg.find(d);
            if (it2 != rg.end()) {
                tmp->dst_next = it2->second.list_head; // add it to the front of the precursor list of the dst node
                if (it2->second.list_head)
                    it2->second.list_head->dst_prev = tmp;
                it2->second.list_head = tmp;
            } else {
                rg[d] = neighbor_list(tmp);
            }*/
            //update_core(tmp->s, tmp->d);
            return true;
        }
        // else the source vertex does not show up in the graph, we need to add a neighbor list, and add the new edge to the neighbor list.
        edge_num++;
        auto *tmp = new sg_edge(s, d, label, timestamp);
        tmp->expiration_time = exp;
        tmp->time_pos = cur;
        cur->edge_pt = tmp;
        g[s] = neighbor_list(tmp);
        /*auto it2 = rg.find(d);
        if (it2 != rg.end()) {
            tmp->dst_next = it2->second.list_head;
            if (it2->second.list_head)
                it2->second.list_head->dst_prev = tmp;
            it2->second.list_head = tmp;
        } else rg[d] = neighbor_list(tmp);*/

        //update_core(tmp->s, tmp->d);
        return true;
    }

    void expire_edge(sg_edge *edge_to_delete) // this function deletes an expired edge from the streaming graph.
    {
        edge_num--;
        unsigned s = edge_to_delete->s;
        unsigned d = edge_to_delete->d;

        if (edge_to_delete->src_prev)
        // If this edge is not the head of the neighbor list, we split it from the neighbor list.
        {
            edge_to_delete->src_prev->src_next = edge_to_delete->src_next;
            if (edge_to_delete->src_next)
                edge_to_delete->src_next->src_prev = edge_to_delete->src_prev;
        } else // otherwise we need to change the head of the nighbor list to the next edge
        {
            auto it = g.find(edge_to_delete->s);
            if (edge_to_delete->src_next) {
                it->second.list_head = edge_to_delete->src_next;
                edge_to_delete->src_next->src_prev = nullptr;
            } else // if there is no next edge, this neighbor list is empty, and we can delete it.
                g.erase(it);
        }

        if (edge_to_delete->dst_prev)
        // update the neighbor list of the destination vertex, similar to the update in for the source vertex.
        {
            edge_to_delete->dst_prev->dst_next = edge_to_delete->dst_next;
            if (edge_to_delete->dst_next)
                edge_to_delete->dst_next->dst_prev = edge_to_delete->dst_prev;
        } else {
            /*auto it = rg.find(edge_to_delete->d);
            if (edge_to_delete->dst_next) {
                it->second.list_head = edge_to_delete->dst_next;
                edge_to_delete->dst_next->dst_prev = nullptr;
            } else
                rg.erase(it);*/
        }

        // core_removal(s, d);
    }


    /**
     *
     * @param timestamp current time
     * @param deleted_edges return vector with deleted edges
     */
    void expire(unsigned int timestamp, vector<edge_info> &deleted_edges)
    // this function is used to find all expired edges and remove them from the graph.
    {
        while (time_list_head) {
            sg_edge *cur_edge = time_list_head->edge_pt;
            /*
            if (cur->timestamp + window_size >= timestamp) // The later edges are still in the sliding window, and we can stop the expiration.
                break;
            */
            if (cur_edge->expiration_time >= timestamp) {
                // The later edges are still in the sliding window, and we can stop the expiration.
                break;
            }

            /*
            if (timestamp - cur_edge->timestamp < 0.1 * window_size) {
                cout << "edge core: (s) " << core_map[cur_edge->s] << " (d) " << core_map[cur_edge->d] << endl;
                cur_edge->expiration_time += extend_expiration(cur_edge, timestamp);

                // shift the element until is in order of expiration time
                timed_edge *cur = time_list_head;
                if (!cur || !cur->next) exit(98);

                // continue shifting while the current node's expiration is greater than the next node's timestamp
                while (cur->next && cur->edge_pt->expiration_time >= cur->next->edge_pt->expiration_time) {
                    timed_edge *next_node = cur->next;

                    cur->next = next_node->next;
                    if (next_node->next) {
                        next_node->next->prev = cur;
                    } else {
                        time_list_tail = cur; // update the tail if the current node becomes the last node
                    }

                    next_node->prev = cur->prev;
                    if (cur->prev) {
                        cur->prev->next = next_node;
                    } else {
                        time_list_head = next_node; // update the head if the current node was the first node
                    }

                    next_node->next = cur;
                    cur->prev = next_node;
                }

                extended++;
            } else { */
            expire_edge(cur_edge);
            deleted_edges.emplace_back(cur_edge->s, cur_edge->d, cur_edge->timestamp, cur_edge->label,
                                       cur_edge->expiration_time);
            // we record the information of expired edges. This information will be used to find expired tree nodes in S-PATH or LM-SRPQ.
            timed_edge *tmp = time_list_head;
            time_list_head = time_list_head->next; // delete the time sequence list unit
            if (time_list_head)
                time_list_head->prev = nullptr;
            delete tmp;
            delete cur_edge;
            //}
        }
        // cout << "extended: " << extended << " vs expired: " << expired << endl;
        if (!time_list_head) // if the list is empty, the tail pointer should also be NULL;
            time_list_tail = nullptr;
    }

    // debug
    /*void get_suc(unsigned int s, int label, vector<unsigned int> &sucs)
    // get the successors of s, connected by edges with given label
    {
        auto it = g.find(s);
        if (it != g.end()) {
            sg_edge *tmp = it->second.list_head;
            while (tmp) {
                if (tmp->label == label)
                    sucs.push_back(tmp->d);
                tmp = tmp->src_next;
            }
        }
    }*/

    // debug
    /*void get_prev(unsigned int d, int label, vector<unsigned int> &prevs)
    // get the precursors of d, connected by edges with given label
    {
        auto it = rg.find(d);
        if (it != rg.end()) {
            sg_edge *tmp = it->second.list_head;
            while (tmp) {
                if (tmp->label == label)
                    prevs.push_back(tmp->s);
                tmp = tmp->dst_next;
            }
        }
    }*/

    // debug
    /*
    void get_all_suc(unsigned int s, vector<pair<unsigned int, unsigned int> > &sucs)
    // get all the successors, each pair is successor node ID + edge label
    {
        auto it = g.find(s);
        if (it != g.end()) {
            sg_edge *tmp = it->second.list_head;
            while (tmp) {
                sucs.emplace_back(tmp->d, tmp->label);
                tmp = tmp->src_next;
            }
        }
    }
    */


    // debug
    /*void get_all_prev(unsigned int d, vector<pair<unsigned int, unsigned int> > &prevs)
    // get all the precursors, each pair is precursor node ID + edge label
    {
        auto it = rg.find(d);
        if (it != rg.end()) {
            sg_edge *tmp = it->second.list_head;
            while (tmp) {
                prevs.emplace_back(tmp->s, tmp->label);
                tmp = tmp->dst_next;
            }
        }
    }*/


    // debug
    /*// the following functions are variant of the former 4 functions, except that timestamp information is also included. The reported result is stored with edge_info structure;
    void get_timed_suc(unsigned int s, int label, vector<edge_info> &sucs) {
        auto it = g.find(s);
        if (it != g.end()) {
            sg_edge *tmp = it->second.list_head;
            while (tmp) {
                if (tmp->label == label)
                    // todo added exp
                    sucs.emplace_back(tmp->s, tmp->d, tmp->timestamp, tmp->label, tmp->expiration_time);
                tmp = tmp->src_next;
            }
        }
    }*/


    // debug
    /*void get_timed_prev(unsigned int d, int label, vector<edge_info> &prevs) {
        auto it = rg.find(d);
        if (it != rg.end()) {
            sg_edge *tmp = it->second.list_head;
            while (tmp) {
                if (tmp->label == label)
                    // todo added exp
                    prevs.emplace_back(tmp->s, tmp->d, tmp->timestamp, tmp->label, tmp->expiration_time);
                tmp = tmp->dst_next;
            }
        }
    }*/


    // DEBUG reimplement, should return the successors of a vertex
    /*
    void get_timed_all_suc(unsigned int s, vector<edge_info> &sucs) {
        auto it = g.find(s);
        if (it != g.end()) {
            sg_edge *tmp = it->second.list_head;
            while (tmp) {
                // todo added exp
                sucs.emplace_back(tmp->s, tmp->d, tmp->timestamp, tmp->label, tmp->expiration_time);
                tmp = tmp->src_next;
            }
        }
    }
    */
    void get_timed_all_suc(unsigned int s, vector<edge_info> &sucs) {
        if (adjacency_list.find(s) == adjacency_list.end()) {
            return; // No outgoing edges for vertex s
        }

        for (const auto &[to, edge]: adjacency_list[s]) {
            sucs.emplace_back(s, to, edge->timestamp, edge->label, edge->expiration_time);
        }
    }


    // debug
    /*void get_timed_all_prev(unsigned int d, vector<edge_info> &prevs) {
        auto it = rg.find(d);
        if (it != rg.end()) {
            sg_edge *tmp = it->second.list_head;
            while (tmp) {
                // todo added exp
                prevs.emplace_back(tmp->s, tmp->d, tmp->timestamp, tmp->label, tmp->expiration_time);
                tmp = tmp->dst_next;
            }
        }
    }*/


    // DEBUG - Used but very inefficient, reimplement with incrementalization
    /*
    void get_src_degree(unsigned int s, map<unsigned int, unsigned int> &degree_map)
    // count the degree of out edges of a vertex s.
    {
        auto it = g.find(s);
        if (it != g.end()) {
            sg_edge *tmp = it->second.list_head;
            while (tmp) {
                if (degree_map.find(tmp->label) != degree_map.end())
                    degree_map[tmp->label]++;
                else
                    degree_map[tmp->label] = 1;
                tmp = tmp->src_next;
            }
        }
    }
    */
    void get_src_degree(unsigned int s, map<unsigned int, unsigned int> &degree_map) {
        if (adjacency_list.find(s) == adjacency_list.end()) {
            return; // No outgoing edges for vertex s
        }

        for (const auto &[_, edge]: adjacency_list[s]) {
            degree_map[edge->label]++;
        }
    }

    /*

        // DEBUG
        void get_dst_degree(unsigned int d, map<unsigned int, unsigned int> &degree_map)
        // count the degree of in edges of a vertex d.
        {
            auto it = rg.find(d);
            if (it != g.end()) {
                sg_edge *tmp = it->second.list_head;
                while (tmp) {
                    if (degree_map.find(tmp->label) != degree_map.end())
                        degree_map[tmp->label]++;
                    else
                        degree_map[tmp->label] = 1;
                    tmp = tmp->dst_next;
                }
            }
        }


        // DEBUG
        /** Compute the degree of a vertex
         * @return The sum of in-degree and out-degree of vertex s
         * */
    /*unsigned int get_total_degree(unsigned int s) {
        std::map<unsigned int, unsigned int> out_degree_map;
        std::map<unsigned int, unsigned int> in_degree_map;

        // Populate out-degree and in-degree maps
        get_src_degree(s, out_degree_map); // Outgoing edges
        get_dst_degree(s, in_degree_map); // Incoming edges

        // Compute the total out-degree
        unsigned int total_out_degree = 0;
        for (const auto &pair: out_degree_map) {
            total_out_degree += pair.second;
        }

        // Compute the total in-degree
        unsigned int total_in_degree = 0;
        for (const auto &pair: in_degree_map) {
            total_in_degree += pair.second;
        }

        // Return the sum of in-degree and out-degree
        return total_out_degree + total_in_degree;
    }*/


    // DEBUG
    /*
    unsigned int compute_memory() // compute the memory of the streaming graph.
    {
        unsigned int total_memory = sizeof(unordered_map<unsigned long long, neighbor_list>) * 2 + 4 * 2 + 8 * 2;
        // memory of the two unordered_map, two integer (window size and edge num), two pointer (head and tail pointer of the time sequence list)
        total_memory += g.size() * 24 + g.bucket_count() * 8;
        // each KV has 16 byte, plus a pointer of 8 byte. Each bucket has another pointer, points to the KV pair list.
        total_memory += rg.size() * 24 + rg.bucket_count() * 8;
        total_memory += edge_num * (56 + 24);
        // each edge has two structure, 56 byte for the sg_edge and 24 byte for the timed_edge;
        return total_memory;
    }
    */

    // DEBUG
    /*unsigned int get_vertice_num() // count the number of vertices in the streaming graph
    {
        unordered_set<unsigned int> S;
        for (auto &it: g) {
            S.insert(it.first);
            sg_edge *tmp = it.second.list_head;
            while (tmp) {
                S.insert(tmp->d);
                tmp = tmp->src_next;
            }
        }
        return S.size();
    }*/
    /*
        // DEBUG
        unsigned int get_edge_num() // return the number of edges in the streaming graph.
        {
            return edge_num;
        }


    */


    /*
 //     -- Naive Version --
 void update_core_decomposition(unsigned int s, unsigned int d) {
     degree[s]++; //fixme place at edge addition
     degree[d]++;

     queue<unsigned int> affected_nodes;
     affected_nodes.push(s);
     affected_nodes.push(d);

     while (!affected_nodes.empty()) {
         unsigned int node = affected_nodes.front();
         affected_nodes.pop();

         unsigned int current_core = core_map[node];
         unsigned int new_core = min(current_core + 1, degree[node]);

         if (new_core > current_core) {
             core_map[node] = new_core;

             // propagate the core increment to neighbors
             sg_edge *tmp = g[node].list_head;
             while (tmp) {
                 unsigned int neighbor = tmp->d;
                 if (core_map[neighbor] < new_core) {
                     affected_nodes.push(neighbor);
                 }
                 tmp = tmp->src_next;
             }

             tmp = rg[node].list_head;
             while (tmp) {
                 unsigned int neighbor = tmp->s;
                 if (core_map[neighbor] < new_core) {
                     affected_nodes.push(neighbor);
                 }
                 tmp = tmp->dst_next;
             }
         }
     }
 }
 */


    /*void remove_candidates(vector<unsigned int> &VC, vector<unsigned int> &O_K_prime, unsigned int w, unsigned int K) {

        deque<unsigned int> Q;

        // for each neighbor w' of w in g (successor list), update deg+ and enqueue if necessary
        auto it = g.find(w); // Get the adjacency list of vertex w in the successor list
        if (it != g.end()) {
            sg_edge *tmp = it->second.list_head;
            while (tmp) {
                unsigned int w_prime = tmp->d;
                if (find(VC.begin(), VC.end(), w_prime) != VC.end()) {
                    // w' in VC
                    deg_plus[w_prime]--; // Decrease deg+ of w'
                    if (deg_plus[w_prime] + deg_star[w_prime] <= K) {
                        Q.push_back(w_prime); // Enqueue if deg+ + deg* <= K
                    }
                }
                tmp = tmp->src_next;
            }
        }


        while (!Q.empty()) {
            unsigned int w_prime = Q.front();
            Q.pop_front();

            // update deg+ and deg* of w' and move it from VC to O_K_prime
            deg_plus[w_prime] += deg_star[w_prime];
            deg_star[w_prime] = 0;

            // remove w' from VC and append it to O_K_prime
            VC.erase(remove(VC.begin(), VC.end(), w_prime), VC.end());
            O_K_prime.push_back(w_prime);

            // for each neighbor w'' of w', update degrees and enqueue if necessary
            auto it_w_prime = g.find(w_prime);
            if (it_w_prime != g.end()) {
                sg_edge *tmp_w_prime = it_w_prime->second.list_head;
                while (tmp_w_prime) {
                    unsigned int w_double_prime = tmp_w_prime->d;

                    if (core_map[w_double_prime] == K) {
                        // Only consider neighbors with core(w'') == K
                        if (core_map[w] <= core_map[w_double_prime]) {
                            deg_star[w_double_prime]--;
                        } else if (core_map[w_prime] <= core_map[w_double_prime] && find(VC.begin(), VC.end(), w_double_prime) != VC.end()) {
                            deg_star[w_double_prime]--;
                        }

                        // If the conditions are met, enqueue w''
                        if (deg_star[w_double_prime] + deg_plus[w_double_prime] <= K && find(
                                Q.begin(), Q.end(), w_double_prime) == Q.end()) {
                            Q.push_back(w_double_prime);
                        }
                    }

                    tmp_w_prime = tmp_w_prime->src_next;
                }
            }

            // handle the case where w'' is in VC and deg* was updated
            auto it_w_prime_double = g.find(w_prime);
            if (it_w_prime_double != g.end()) {
                sg_edge *tmp_double_prime = it_w_prime_double->second.list_head;
                while (tmp_double_prime) {
                    unsigned int w_double_prime = tmp_double_prime->d;
                    if (find(VC.begin(), VC.end(), w_double_prime) != VC.end()) {
                        // w'' in VC
                        deg_plus[w_double_prime]--;
                        if (deg_star[w_double_prime] + deg_plus[w_double_prime] <= K && find(
                                Q.begin(), Q.end(), w_double_prime) == Q.end()) {
                            Q.emplace_back(w_double_prime); // Enqueue if necessary
                        }
                    }
                    tmp_double_prime = tmp_double_prime->src_next;
                }
            }
        }
    }


    void update_core(unsigned int u, unsigned int v) {
        /* Preparing Phase #1#
        unsigned int K = min(core_map[u], core_map[v]);

        if (core_map[u] <= core_map[v]) {
            deg_plus[u]++;
        } else {
            deg_plus[v]++;
        }

        if (deg_plus[u] <= K && deg_plus[v] <= K) {
            return;
        }

        /* Core Phase #1#
        vector<unsigned int> VC; // candidate vertices for core increment
        vector<unsigned int> O_K_prime; // updated K-order
        vector<unsigned int> OK; // sequence of vertices in k-order whose core numbers are k

        for (auto& node: core_map) {
            if (node.second == K) {
                OK.push_back(node.first);
            }
        }

        size_t i = 0;
        while (i < OK.size()) {
            unsigned int vi = OK[i];

            unsigned int mcd_vi = 0;
            auto it = g.find(vi);
            if (it != g.end()) {
                sg_edge *tmp = it->second.list_head;
                while (tmp) {
                    unsigned int w = tmp->d;
                    if (core_map[w] >= core_map[vi]) {
                        mcd_vi++;
                    }
                    tmp = tmp->src_next;
                }
            }

            mcd[vi] = mcd_vi;

            if (deg_star[vi] + deg_plus[vi] > K) {
                OK.erase(OK.begin() + i);
                VC.push_back(vi);

                // Update neighbors' degree star
                auto it = g.find(vi);
                if (it != g.end()) {
                    sg_edge *tmp = it->second.list_head;
                    while (tmp) {
                        unsigned int w = tmp->d;
                        if (core_map[w] == K && core_map[vi] <= core_map[w]) {
                            deg_star[w]++;
                        }
                        tmp = tmp->src_next;
                    }
                }
                i++;
            } else if (deg_star[vi] == 0) {
                size_t j = i + 1;
                while (j < OK.size() && deg_star[OK[j]] == 0 && deg_plus[OK[j]] <= K) {
                    j++;
                }

                for (size_t k = i; k < j; k++) {
                    O_K_prime.push_back(OK[k]);
                }
                OK.erase(OK.begin() + i, OK.begin() + j);
                i = j;
            } else {
                O_K_prime.push_back(vi);
                OK.erase(OK.begin() + i);
                deg_plus[vi] += deg_star[vi];
                deg_star[vi] = 0;
                remove_candidates(VC, O_K_prime, vi, K);
                i++;
            }
        }

        /* Ending Phase #1#
        for (unsigned int w: VC) {
            deg_star[w] = 0;
            core_map[w]++;
        }
        */

    /*
    // insert vertices into OK+1 in k-order
    core_order.erase(remove_if(core_order.begin(), core_order.end(), [&](unsigned int node) {
        return core_map[node] == K;
    }), core_order.end());

    core_order.insert(core_order.end(), VC.begin(), VC.end());
    sort(core_order.begin(), core_order.end(), [&](unsigned int a, unsigned int b) {
        return make_pair(core_map[a], a) < make_pair(core_map[b], b);
    });
    */

    // for (const auto& [vertex, mcd_value] : mcd) {
    //     cout << "Vertex " << vertex << " has mcd: " << mcd_value << endl;
    // }
    /*
    }

    std::vector<unsigned int> find_VStar(unsigned int u, unsigned int v, unsigned int K) {
        std::unordered_set<unsigned int> visited;  // Set to track visited nodes
        std::vector<unsigned int> V_star;  // Resulting set of affected vertices
        std::stack<unsigned int> dfs_stack;  // Stack for DFS

        // Start DFS from the root vertex u (or v if core(v) = K)
        dfs_stack.push(u);
        visited.insert(u);

        // DFS loop
        while (!dfs_stack.empty()) {
            unsigned int w = dfs_stack.top();
            dfs_stack.pop();

            // If the current vertex's core degree is less than K, add it to V*
            if (mcd[w] < K) {
                V_star.push_back(w);
                core_map[w]--;  // Decrease the core number of w

                // For each neighbor of w, update mcd(z) and continue DFS if necessary
                auto it = g.find(w);
                if (it != g.end()) {
                    sg_edge* tmp = it->second.list_head;
                    while (tmp) {
                        unsigned int z = tmp->d;
                        if (core_map[z] == K) {
                            mcd[z]--;  // Decrease mcd(z)
                        }
                        if (mcd[z] < K && visited.find(z) == visited.end()) {
                            visited.insert(z);
                            dfs_stack.push(z);  // Push the neighbor onto the stack for further DFS
                        }
                        tmp = tmp->src_next;
                    }
                }
            }
        }

        return V_star;
    }

    void core_removal(unsigned int u, unsigned int v) {
        unsigned int K = std::min(core_map[u], core_map[v]);

        // Update mcd based on core values
        if (core_map[u] <= core_map[v]) {
            mcd[u]--;
        } else {
            mcd[v]--;
        }

        // Find V* using the DFS-based traversal algorithm (implemented separately)
        std::vector<unsigned int> V_star = find_VStar(u, v, K);

        // Update the k-order and degrees of vertices in V*
        for (unsigned int w : V_star) {
            deg_plus[w] = 0;
            // Update neighbors of w
            auto it = g.find(w);
            if (it != g.end()) {
                sg_edge* tmp = it->second.list_head;
                while (tmp) {
                    unsigned int w_prime = tmp->d;
                    if (core_map[w_prime] == K && core_map[w_prime] <= core_map[w]) {
                        deg_plus[w_prime]--;
                    }
                    if (core_map[w_prime] >= K || std::find(V_star.begin(), V_star.end(), w_prime) != V_star.end()) {
                        deg_plus[w]++; // Increment deg+ if condition holds
                    }
                    tmp = tmp->src_next;
                }
            }
        }

        // Further code to clean up and sort core_order if needed
        // Sort core_order based on updated core values
        /*
        std::sort(core_order.begin(), core_order.end(), [&](unsigned int a, unsigned int b) {
            return core_map[a] < core_map[b];
        });
        #1#
    }
    */


    bool insert_edge(int s, int d, int label, int timestamp)
    // insert an edge in the streaming graph, bool indicates if it is a new edge (not appear before)
    {
        auto *cur = new timed_edge;
        // create a new timed edge and insert it to the tail of the time sequence list, as in a streaming graph the inserted edge is always the latest
        add_timed_edge(cur);

        auto it = g.find(s);

        if (it != g.end()) {
            sg_edge *tmp = it->second.list_head;
            while (tmp) {
                if (tmp->label == label && tmp->d == d)
                // If we find the edge in the cross list, this is not a new edge, and we only update its timestamp and pointer to the time sequence list;
                {
                    delete_timed_edge(tmp->time_pos);
                    tmp->time_pos = cur;
                    cur->edge_pt = tmp;
                    tmp->timestamp = timestamp;
                    return false;
                }
                tmp = tmp->src_next;
            }
            edge_num++; // if not appears before
            tmp = new sg_edge(s, d, label, timestamp); // create a new sg_edge
            tmp->time_pos = cur;
            cur->edge_pt = tmp;
            tmp->src_next = it->second.list_head; // add it to the front of the successor list of the src node
            if (it->second.list_head)
                it->second.list_head->src_prev = tmp;
            it->second.list_head = tmp;
            /*auto it2 = rg.find(d);
            if (it2 != rg.end()) {
                tmp->dst_next = it2->second.list_head; // add it to the front of the precursor list of the dst node
                if (it2->second.list_head)
                    it2->second.list_head->dst_prev = tmp;
                it2->second.list_head = tmp;
            } else {
                rg[d] = neighbor_list(tmp);
            }*/
            return true;
        }
        // else the source vertex does not show up in the graph, we need to add a neighbor list, and add the new edge to the neighbor list.
        edge_num++;
        auto *tmp = new sg_edge(s, d, label, timestamp);
        tmp->time_pos = cur;
        cur->edge_pt = tmp;
        g[s] = neighbor_list(tmp);
        /*auto it2 = rg.find(d);
        if (it2 != rg.end()) {
            tmp->dst_next = it2->second.list_head;
            if (it2->second.list_head)
                it2->second.list_head->dst_prev = tmp;
            it2->second.list_head = tmp;
        } else
            rg[d] = neighbor_list(tmp);*/
        return true;
    }
};
