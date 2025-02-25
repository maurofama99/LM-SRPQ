#include<vector>
#include<string>
#include<ctime>
#include<cstdlib>
#include<algorithm>
#include <fstream>
#include <sstream>

#include "fsa.h"
#include "rpq_forest.h"
#include "streaming_graph.h"
#include "s_path.h"

#define BACKWARD_RETENTION true

using namespace std;

struct Config {
    int algorithm{};
    std::string input_data_path;
    std::string output_base_folder;
    int size{};
    int slide{};
    int query_type{};
    std::vector<int> labels;
    double zscore{};
};

Config readConfig(const std::string& filename) {
    Config config;
    std::ifstream file(filename);
    std::string line;

    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    std::unordered_map<std::string, std::string> configMap;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            configMap[key] = value;
        }
    }

    // Convert values from the map
    config.algorithm = std::stoi(configMap["algorithm"]);
    config.input_data_path = configMap["input_data_path"];
    config.output_base_folder = configMap["output_base_folder"];
    config.size = std::stoi(configMap["size"]);
    config.slide = std::stoi(configMap["slide"]);
    config.query_type = std::stoi(configMap["query_type"]);

    // Parse extra_args
    std::istringstream extraArgsStream(configMap["labels"]);
    std::string arg;
    while (std::getline(extraArgsStream, arg, ',')) {
        config.labels.push_back(std::stoi(arg));
    }

    config.zscore = std::stod(configMap["zscore"]);

    return config;
}

bool compare(unsigned int a, unsigned int b) {
    return a > b;
}

class window {
public:
    unsigned int t_open;
    unsigned int t_close;
    timed_edge *first;
    timed_edge *last;
    bool evicted = false;

    // Constructor
    window(unsigned int t_open, unsigned int t_close, timed_edge *first, timed_edge *last) {
        this->t_open = t_open;
        this->t_close = t_close;
        this->first = first;
        this->last = last;
    }
};

void setup_automaton(int query_type, FiniteStateAutomaton *aut, const vector<int>& labels);

int main(int argc, char *argv[]) {

    string config_path = argv[1];
    Config config = readConfig(config_path);

    int algorithm = config.algorithm;
    ifstream fin(config.input_data_path.c_str());
    string output_base_folder = config.output_base_folder;
    unsigned int size = config.size;
    unsigned int slide = config.slide;
    int query_type = config.query_type;
    double zscore = config.zscore;

    if (size % slide != 0) {
        printf("ERROR: Size is not a multiple of slide\n");
        exit(1);
    }

    if (fin.fail()) {
        cerr << "Error opening file" << endl;
        exit(1);
    }

    auto *aut = new FiniteStateAutomaton();
    setup_automaton(query_type, aut, config.labels);
    auto *f = new Forest();
    auto *sg = new streaming_graph(zscore);

    vector<window> windows;

    unsigned int s, d, l;
    unsigned int t;

    unsigned int t0 = 0;
    vector<unsigned int> insertion_time;
    unsigned int edge_number = 0;

    if (algorithm == 1) {
        int checkpoint = 1;
        auto query = new SPathHandler(*aut, *f, *sg);

        clock_t start = clock();

        unsigned int time;
        unsigned int last_window_index = 0;
        unsigned int window_offset = 0;
        windows.emplace_back(0, size, nullptr, nullptr);

        while (fin >> s >> d >> l >> t) {
            edge_number++;
            if (t0 == 0) {
                t0 = t;
                time = 1;
            } else time = t - t0;

            // process the edge if the label is part of the query
            if (!aut->hasLabel(l))
                continue;

            /* SCOPE */
            double c_sup = ceil(time / slide) * slide;
            double o_i = c_sup - size;

            unsigned int window_close;
            do {
                window_close = o_i + size;
                if (windows[last_window_index].t_close < window_close) {
                    windows.emplace_back(o_i, window_close, nullptr, nullptr);
                    last_window_index++;
                    // cout << "DEBUG: Window [" << o_i << ", " << window_close << "] created\n";
                }
                o_i += slide;
            } while (o_i <= time);

            /* ADD */
            // update adjacency list of snapshot graph, if edge exists, return null
            // cout << "DEBUG: Inserting edge (" << s << ", " << d << ", " << l << ", " << time << ")\n";
            sg_edge *new_sgt = sg->insert_edge(edge_number, s, d, l, time, window_close);

            // duplicate handling in tuple list, important to not evict an updated edge
            if (!new_sgt) {
                // search for the duplicate
                auto existing_edge = sg->search_existing_edge(s, d, l);
                if (!existing_edge) {
                    cout << "ERROR: Existing edge not found." << endl;
                    exit(1);
                }

                // update window open pointers if needed
                for (size_t i = window_offset; i < windows.size(); i++) {
                    if (windows[i].first == existing_edge->time_pos) {
                        // shift window first to next element
                        windows[i].first = existing_edge->time_pos->next;
                    }
                }

                // update edge list (erase and re-append)
                sg->delete_timed_edge(existing_edge->time_pos);

                new_sgt = existing_edge;
                new_sgt->timestamp = time;
                new_sgt->expiration_time = window_close;
            }

            // add edge to time list
            auto t_edge = new timed_edge(new_sgt);
            sg->add_timed_edge(t_edge);
            new_sgt->time_pos = t_edge;

            bool evict = false;
            vector<size_t> to_evict;
            // add edge to window and check for window eviction
            for (size_t i = window_offset; i < windows.size(); i++) {
                if (windows[i].t_open <= time && time < windows[i].t_close) {
                    // add new sgt to window
                    if (!windows[i].first) windows[i].first = t_edge;
                    windows[i].last = t_edge;
                } else if (time > windows[i].t_close) {
                    // schedule window for eviction
                    window_offset = i + 1;
                    to_evict.push_back(i);
                    evict = true;
                }
            }

            /* QUERY */
            // add sgt to RPQ forest
            query->s_path(new_sgt);
            // f->printForest();

            /* EVICT */
            if (evict) {
                // cout << "DEBUG: Evicting window\n";
                std::unordered_set<unsigned int> deleted_vertexes;
                timed_edge *evict_start_point = windows[to_evict[0]].first;
                timed_edge *evict_end_point = windows[to_evict.back() + 1].first;

                if (!evict_start_point) {
                    cout << "ERROR: Evict start point is null." << endl;
                    exit(1);
                }

                if (evict_end_point == nullptr) {
                    evict_end_point = sg->time_list_tail;
                    cout << "WARNING: Evict end point is null, evicting whole buffer." << endl;
                }

                timed_edge *current = evict_start_point;
                while (current && current != evict_end_point) {
                    // cout << "DEBUG: Evicting edge (" << current->edge_pt->s << ", " << current->edge_pt->d << ", " << current->edge_pt->label << ", " << current->edge_pt->timestamp << ")\n";
                    auto cur_edge = current->edge_pt;
                    auto next = current->next;

                    if (sg->get_zscore(cur_edge->s) > sg->zscore_threshold && BACKWARD_RETENTION) {
                        // cout << "DEBUG: Saved edge (" << cur_edge->s << ", " << cur_edge->d << ", " << cur_edge->label << ", " << cur_edge->timestamp << ", z_score: " << sg->get_zscore(cur_edge->s) << endl;
                        sg->saved_edges++;
                        auto shift = 1 + to_evict.back() + static_cast<size_t>(std::ceil(sg->get_zscore(cur_edge->s)));
                        auto target_window_index = shift < last_window_index? shift : last_window_index;
                        sg->shift_timed_edge(cur_edge->time_pos, windows[target_window_index].first);
                    } else {
                        deleted_vertexes.insert(cur_edge->s); // schedule for further RQP Forest deletion
                        deleted_vertexes.insert(cur_edge->d);
                        sg->remove_edge(cur_edge->s, cur_edge->d, cur_edge->label); // delete from adjacency list
                        sg->delete_timed_edge(current); // delete from window state store
                    }

                    current = next;
                }

                // mark window as evicted
                for (unsigned long i: to_evict) {
                    windows[i].evicted = true;
                    windows[i].first = nullptr;
                    windows[i].last = nullptr;
                }
                to_evict.clear();

                // reset time list pointers
                sg->time_list_head = evict_end_point;
                sg->time_list_head->prev = nullptr;

                // update RPQ Forest
                f->expire(deleted_vertexes);
                deleted_vertexes.clear();

                // f->printForest();
                // cout << "DEBUG: Window evicted\n\n";
            }

            /* DEBUG: Print active windows */
            /*
            for (const auto &window: windows) {
                if (window.evicted) continue;
                cout << "Window [" << window.t_open << ", " << window.t_close << "]\n";
                if (window.first) {
                    cout << "First Edge: (" << window.first->edge_pt->s << ", " << window.first->edge_pt->d << ", " <<
                            window.first->edge_pt->label << ", " << window.first->edge_pt->timestamp << ")\n";
                }
                if (window.last) {
                    cout << "Last Edge: (" << window.last->edge_pt->s << ", " << window.last->edge_pt->d << ", " <<
                            window.last->edge_pt->label << ", " << window.last->edge_pt->timestamp << ")\n";
                }
                cout << endl;
            }
            */

            if (time >= checkpoint * 3600) {
                checkpoint += checkpoint;

                printf("processed edges: %u\n", edge_number);
                printf("saved edges: %d\n", sg->saved_edges);
                printf("avg degree: %f\n", sg->mean);
                printf("results: %d\n\n", query->results_count);
            }
        }

        clock_t finish = clock();
        unsigned int time_used = (double) (finish - start) / CLOCKS_PER_SEC;
        cout << "execution time: " << time_used << endl;

        // Construct output file path
        std::string retention = BACKWARD_RETENTION ? std::to_string(zscore) : "false";
        std::string output_file = config.output_base_folder + "output_" + std::to_string(query_type) + "_" + std::to_string(size) + "_" + std::to_string(slide) + "_" + retention + ".txt";
        std::string output_file_csv = config.output_base_folder + "output_" + std::to_string(query_type) + "_" + std::to_string(size) + "_" + std::to_string(slide) + "_" + retention + ".csv";

        // Open file for writing
        std::ofstream outFile(output_file);
        if (!outFile) {
            std::cerr << "Error opening file for writing: " << output_file << std::endl;
            cout << "resulting paths: " << query->results_count << "\n";
            cout << "processed edges: " << edge_number << "\n";
            cout << "saved edges: " << sg->saved_edges << "\n";
            cout << "execution time: " << time_used << "\n";
            return EXIT_FAILURE;
        }

        // Write data to the file
        outFile << "resulting paths: " << query->results_count << "\n";
        outFile << "processed edges: " << edge_number << "\n";
        outFile << "saved edges: " << sg->saved_edges << "\n";
        outFile << "execution time: " << time_used << "\n";

        query->exportResultSet(output_file_csv);

        // Close the file
        outFile.close();

        std::cout << "Results written to: " << output_file << std::endl;

        delete f;
    }

    delete
            sg;
    return
            0;
}

// Set up the automaton correspondant for each query
void setup_automaton(int query_type, FiniteStateAutomaton *aut, const vector<int>& labels) {
    int state_num = 0;

    if (query_type == 1) {
        state_num = 1;
        aut->addFinalState(0);
        aut->addTransition(0, 0, labels[0]);
    } else if (query_type == 2) {
        state_num = 2;
        aut->addFinalState(1);
        aut->addTransition(0, 1, labels[0]);
        aut->addTransition(0, 1, labels[1]);
        aut->addTransition(1, 1, labels[1]);
    } else if (query_type == 3) { // ab*
        state_num = 2;
        aut->addFinalState(1);
        aut->addTransition(0, 1, labels[0]);
        aut->addTransition(1, 1, labels[1]);
    } else if (query_type == 4) {
        state_num = 4;
        aut->addFinalState(3);
        aut->addTransition(0, 1, labels[0]);
        aut->addTransition(1, 2, labels[1]);
        aut->addTransition(2, 3, labels[2]);
    } else if (query_type == 5) {
        state_num = 3;
        aut->addFinalState(2);
        aut->addTransition(0, 1, labels[0]);
        aut->addTransition(1, 2, labels[1]);
        aut->addTransition(2, 2, labels[2]);
    } else if (query_type == 6) {
        state_num = 3;
        aut->addFinalState(2);
        aut->addTransition(0, 1, labels[0]);
        aut->addTransition(1, 1, labels[1]);
        aut->addTransition(1, 2, labels[2]);
    } else if (query_type == 7) {
        state_num = 2;
        aut->addFinalState(1);
        aut->addTransition(0, 1, labels[0]);
        aut->addTransition(0, 1, labels[1]);
        aut->addTransition(0, 1, labels[2]);
        aut->addTransition(1, 1, labels[3]);
    } else if (query_type == 8) {
        state_num = 2;
        aut->addFinalState(0);
        aut->addFinalState(1);
        aut->addTransition(0, 0, labels[0]);
        aut->addTransition(0, 1, labels[1]);
        aut->addTransition(1, 1, labels[1]);
    } else if (query_type == 9) {
        state_num = 3;
        aut->addFinalState(1);
        aut->addFinalState(2);
        aut->addTransition(0, 1, labels[0]);
        aut->addTransition(1, 1, labels[1]);
        aut->addTransition(1, 2, labels[2]);
        aut->addTransition(2, 2, labels[2]);
    } else if (query_type == 10) {
        state_num = 1;
        aut->addFinalState(0);
        aut->addTransition(0, 0, labels[0]);
        aut->addTransition(0, 0, labels[1]);
        aut->addTransition(0, 0, labels[2]);
    } else if (query_type == 11) {
        state_num = 5;
        aut->addFinalState(4);
        aut->addTransition(0, 1, labels[0]);
        aut->addTransition(1, 2, labels[1]);
        aut->addTransition(2, 3, labels[2]);
        aut->addTransition(3, 4, labels[3]);
    } else {
        cout << "ERROR: Wrong query type" << endl;
        exit(1);
    }
}
