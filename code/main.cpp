#include<vector>
#include<string>
#include<ctime>
#include<cstdlib>
#include <fstream>
#include <sstream>

#include "source/fsa.h"
#include "source/rpq_forest.h"
#include "source/streaming_graph.h"
#include "source/s_path.h"
#include "source/lmsrpq/LM-SRPQ.h"
#include "source/lmsrpq/S-PATH.h"

using namespace std;

typedef struct Config {
    int algorithm;
    std::string input_data_path;
    std::string output_base_folder;
    int size;
    int slide;
    int query_type;
    std::vector<int> labels;
    double zscore;
} config;

config readConfig(const std::string &filename) {
    config config;
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

vector<int> setup_automaton(int query_type, FiniteStateAutomaton *aut, const vector<int> &labels);

int main(int argc, char *argv[]) {
    string config_path = argv[1];
    config config = readConfig(config_path.c_str());

    int algorithm = config.algorithm;
    ifstream fin(config.input_data_path.c_str());
    string output_base_folder = config.output_base_folder;
    unsigned int size = config.size;
    unsigned int slide = config.slide;
    int query_type = config.query_type;
    double zscore = config.zscore;
    bool BACKWARD_RETENTION = zscore != 0;

    unsigned int watermark = strtol(argv[2], nullptr, 10);
    unsigned int current_time = 0;
    bool handle_ooo = false;

    if (size % slide != 0) {
        printf("ERROR: Size is not a multiple of slide\n");
        exit(1);
    }

    if (fin.fail()) {
        cerr << "Error opening file" << endl;
        exit(1);
    }

    if (algorithm != 3 and watermark != 0) {
        cerr << "Out of order not supported for this algorithm" << endl;
        exit(1);
    }

    auto *aut = new FiniteStateAutomaton();
    vector<int> scores = setup_automaton(query_type, aut, config.labels);
    auto *f = new Forest();
    auto *sg = new streaming_graph(zscore);

    vector<window> windows;

    unsigned int s, d, l;
    unsigned int t;

    unsigned int t0 = 0;
    unsigned int edge_number = 0;
    unsigned int time;
    int timestamp;

    unsigned int last_window_index = 0;
    unsigned int window_offset = 0;
    windows.emplace_back(0, size, nullptr, nullptr);

    bool evict = false;
    vector<size_t> to_evict;
    unsigned int last_expired_window = 0;

    int checkpoint = 1;

    auto *f1 = new RPQ_forest(sg, aut); // TS Propagation

    auto *f2 = new LM_forest(sg, aut); // Landmark Trees
    double candidate_rate = 0.2;
    double benefit_threshold = 1.5;
    for (int i = 0; i < scores.size(); i++)
        f2->aut_scores[i] = scores.at(i);

    auto query = new SPathHandler(*aut, *f, *sg); // Density-based Retention

    clock_t start = clock();
    while (fin >> s >> d >> l >> t) {
        edge_number++;
        if (t0 == 0) {
            t0 = t;
            timestamp = 1;
        } else timestamp = t - t0;

        if (timestamp < 0) continue;
        time = timestamp;

        // process the edge if the label is part of the query
        if (!aut->hasLabel(l))
            continue;

        int watermark_gap = current_time - time;

        double c_sup = ceil(static_cast<double>(time) / static_cast<double>(slide)) * slide;
        double o_i = c_sup - size;
        unsigned int window_close;

        handle_ooo = false;
        if (time >= current_time) { // in-order element
            if (time == current_time) time++;
            current_time = time;
            do {
                window_close = o_i + size;
                if (windows[last_window_index].t_close < window_close) {
                    windows.emplace_back(o_i, window_close, nullptr, nullptr);
                    last_window_index++;
                }
                o_i += slide;
            } while (o_i <= time);

        } else if (watermark_gap > 0 && watermark_gap <= watermark) { // out-of-order element before watermark expiration
            std::vector<std::pair<unsigned int, unsigned int> > windows_to_recover;
            do {
                window_close = o_i + size;
                if (windows[last_window_index].t_close < window_close) {
                    cerr << "ERROR: OOO Window not found." << endl;
                    exit(1);
                }
                for (size_t i = last_expired_window; i < windows.size(); ++i) {
                    if (auto &win = windows[i]; win.t_open == o_i && window_close == win.t_close) {
                        if (win.evicted) windows_to_recover.emplace_back(o_i, window_close);
                        else handle_ooo = true; // true iff the element belongs to an active window
                    }
                }
                o_i += slide;
            } while (o_i <= time);

            // the element is in an expired window
            query->compute_missing_results(edge_number, s, d, l, time, window_close, windows_to_recover);
            if (!handle_ooo) continue;

        } else continue; // out-of-order element after watermark already expired

        /* SCOPE */
        timed_edge *t_edge;
        sg_edge *new_sgt;

        if (algorithm == 1) new_sgt = f1->insert_edge_spath(edge_number, s, d, l, time, window_close);
        else if (algorithm == 2) new_sgt = f2->insert_edge_lmsrpq(edge_number, s, d, l, time, window_close);
        else if (algorithm == 3) new_sgt = sg->insert_edge(edge_number, s, d, l, time, window_close);
        else {
            cerr << "ERROR: Wrong algorithm selection." << endl;
            exit(1);
        }

        // duplicate handling in tuple list, important to not evict an updated edge
        if (!new_sgt) {
            // search for the duplicate
            auto existing_edge = sg->search_existing_edge(s, d, l);
            if (!existing_edge) {
                cerr << "ERROR: Existing edge not found." << endl;
                exit(1);
            }

            if (existing_edge->timestamp > time) continue;

            // adjust window boundaries if needed
            for (size_t i = window_offset; i < windows.size(); i++) {
                if (windows[i].first == existing_edge->time_pos) {
                    if (windows[i].first != windows[i].last) { // if the window has more than one element
                        if (existing_edge->time_pos->next) windows[i].first = existing_edge->time_pos->next;
                        else {
                            cerr << "ERROR: Time position not found." << endl;
                            exit(1);
                        }
                    } else {
                        windows[i].first = nullptr;
                        windows[i].last = nullptr;
                    }
                }
                if (windows[i].last == existing_edge->time_pos) {
                    if (windows[i].first != windows[i].last) { // if the window has more than one element
                        if (existing_edge->time_pos->prev) windows[i].last = existing_edge->time_pos->prev;
                        else {
                            cerr << "ERROR: Time position not found." << endl;
                            exit(1);
                        }
                    } else {
                        windows[i].first = nullptr;
                        windows[i].last = nullptr;
                    }
                }
            }

            // update edge list (erase and re-append)
            sg->delete_timed_edge(existing_edge->time_pos);
            new_sgt = existing_edge;
            new_sgt->timestamp = time;
            new_sgt->expiration_time = window_close;
        }

        // add edge to time list
        t_edge = new timed_edge(new_sgt);
        if (handle_ooo) sg->add_timed_edge_inorder(t_edge); // ooo: add element in order into already existing window
        else sg->add_timed_edge(t_edge);                    // io: append element to last window

        // add edge to window and check for window eviction
        for (size_t i = window_offset; i < windows.size(); i++) {
            if (windows[i].t_open <= time && time < windows[i].t_close) {
                // add new sgt to window
                if (!windows[i].first || time <= windows[i].first->edge_pt->timestamp) {
                    if (handle_ooo && windows[i].first->prev != t_edge) {
                        cerr << "ERROR: Window first is not the previous element." << endl;
                        exit(1);
                    }
                    if (!windows[i].first) windows[i].last = t_edge;
                    windows[i].first = t_edge;
                }
                else if (!windows[i].last || time > windows[i].last->edge_pt->timestamp) {
                    if (handle_ooo && windows[i].last->next != t_edge) {
                        cerr << "ERROR: Window last is not the next element." << endl;
                        exit(1);
                    }
                    windows[i].last = t_edge;
                }
            } else if (time >= windows[i].t_close) {

                if (watermark!=0) { // persist current query state when window slides
                    if (to_evict.empty()) {
                        f->deepCopyTreesAndVertexTreeMap(windows[i].t_open, windows[i].t_close);
                        sg->deep_copy_adjacency_list(windows[i].t_open, windows[i].t_close);
                    } else { // no need of copying the same state multiple times since the slide is empty
                        f->backup_map[std::make_pair(windows[i].t_open, windows[i].t_close)] = f->backup_map[std::make_pair(windows[to_evict[0]].t_open,windows[to_evict[0]].t_close)];
                    }
                }

                // schedule window for eviction
                window_offset = i + 1;
                to_evict.push_back(i);
                evict = true;
            }
        }

        new_sgt->time_pos = t_edge;

        /* QUERY */
        if (algorithm == 3) query->pattern_matching(new_sgt);

        /* EVICT */
        if (evict) {
            std::unordered_set<unsigned int> deleted_vertexes;
            vector<edge_info> deleted_edges;
            timed_edge *evict_start_point = windows[to_evict[0]].first;
            timed_edge *evict_end_point = windows[to_evict.back() + 1].first;

            if (!evict_start_point) {
                cerr << "ERROR: Evict start point is null." << endl;
                exit(1);
            }

            if (evict_end_point == nullptr) {
                evict_end_point = sg->time_list_tail;
                cout << "WARNING: Evict end point is null, evicting whole buffer." << endl;
            }

            timed_edge *current = evict_start_point;
            while (current && current != evict_end_point) {
                auto cur_edge = current->edge_pt;
                auto next = current->next;

                if (sg->get_zscore(cur_edge->s) > sg->zscore_threshold && BACKWARD_RETENTION) {
                    sg->saved_edges++;
                    auto shift = 1 + to_evict.back() + static_cast<size_t>(std::ceil(sg->get_zscore(cur_edge->s)));
                    auto target_window_index = shift < last_window_index ? shift : last_window_index;
                    sg->shift_timed_edge(cur_edge->time_pos, windows[target_window_index].first);
                } else {
                    if (algorithm == 3) {
                        deleted_vertexes.insert(cur_edge->s); // schedule for further RQP Forest deletion
                        deleted_vertexes.insert(cur_edge->d);
                    }

                    if (algorithm == 1 || algorithm == 2)
                        deleted_edges.emplace_back(cur_edge->s, cur_edge->d, cur_edge->timestamp, cur_edge->label, cur_edge->expiration_time, -1);

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

            if (algorithm == 1)
                f1->expire(time, deleted_edges);
            else if (algorithm == 2) {
                f2->expire(time, deleted_edges);
                f2->dynamic_lm_select(candidate_rate, benefit_threshold);
            } else if (algorithm == 3) {
                f->expire(deleted_vertexes);
                deleted_vertexes.clear();
            }

            evict = false;
        }

        if (watermark != 0) {
            for (unsigned int i = last_expired_window; i < windows.size(); i++) {
                if (windows[i].t_close <= time - watermark) {
                    f->deleteExpiredForest(windows[i].t_open, windows[i].t_close);
                    sg->delete_expired_adj(windows[i].t_open, windows[i].t_close);
                    last_expired_window = i;
                } else break;
            }
        }

        if (time >= checkpoint * 3600) {
            checkpoint += checkpoint;

            printf("processed edges: %u\n", edge_number);
            printf("saved edges: %d\n", sg->saved_edges);
            printf("avg degree: %f\n", sg->mean);
            if (algorithm == 1) {
                cout << "resulting paths: " << f1->distinct_results << "\n\n";
            } else if (algorithm == 2) {
                cout << "resulting paths: " << f2->distinct_results << "\n\n";
            } else if (algorithm == 3) {
                cout << "resulting paths: " << query->results_count << "\n\n";
            }
        }
    }

    clock_t finish = clock();
    unsigned int time_used = (double) (finish - start) / CLOCKS_PER_SEC;
    cout << "execution time: " << time_used << endl;

    // Construct output file path
    std::string retention = BACKWARD_RETENTION ? std::to_string(zscore) : "false";
    std::string output_file = config.output_base_folder + "output_" + std::to_string(algorithm) + "_" +
                              std::to_string(query_type) + "_" + std::to_string(size) + "_" + std::to_string(slide) +
                              "_" + retention + ".txt";
    std::string output_file_csv = config.output_base_folder + "output_" + std::to_string(algorithm) + "_" +
                                  std::to_string(query_type) + "_" + std::to_string(size) + "_" + std::to_string(slide)
                                  + "_" + retention + ".csv";

    // Open file for writing
    std::ofstream outFile(output_file.c_str());
    if (!outFile) {
        std::cerr << "Error opening file for writing: " << output_file << std::endl;

        if (algorithm == 1) {
            cout << "resulting paths: " << f1->distinct_results << "\n";
        } else if (algorithm == 2) {
            cout << "resulting paths: " << f2->distinct_results << "\n";
        } else if (algorithm == 3) {
            cout << "resulting paths: " << query->results_count << "\n";
        }

        cout << "processed edges: " << edge_number << "\n";
        cout << "saved edges: " << sg->saved_edges << "\n";
        cout << "execution time: " << time_used << "\n";
        return EXIT_FAILURE;
    }

    // Write data to the file
    if (algorithm == 1) {
        outFile << "resulting paths: " << f1->distinct_results << "\n";
    } else if (algorithm == 2) {
        outFile << "resulting paths: " << f2->distinct_results << "\n";
        outFile << "landmark trees: " << f2->landmarks_count << "\n";
    } else if (algorithm == 3) {
        outFile << "resulting paths: " << query->results_count << "\n";
    }
    outFile << "processed edges: " << edge_number << "\n";
    outFile << "saved edges: " << sg->saved_edges << "\n";
    outFile << "execution time: " << time_used << "\n";

    if (algorithm == 1) {
        f1->export_result(output_file_csv);
    } else if (algorithm == 2) {
        f2->export_result(output_file_csv);
    } else if (algorithm == 3) {
        query->exportResultSet(output_file_csv);
    }

    // Close the file
    outFile.close();

    std::cout << "Results written to: " << output_file << std::endl;

    delete sg;
    return 0;
}

// Set up the automaton correspondant for each query
vector<int> setup_automaton(int query_type, FiniteStateAutomaton *aut, const vector<int> &labels) {
    vector<int> scores;

    switch (query_type) {
        case 1:
            aut->addFinalState(0);
            aut->addTransition(0, 0, labels[0]);
            scores.emplace(scores.begin(), 6);
            break;
        case 2:
            aut->addFinalState(1);
            aut->addTransition(0, 1, labels[0]);
            aut->addTransition(0, 1, labels[1]);
            aut->addTransition(1, 1, labels[1]);
            scores.emplace(scores.begin(), 0); //score of s0 is set to 0 as it has no incoming edge
            scores.emplace(scores.begin() + 1, 6);
            break;
        case 3: // ab*
            aut->addFinalState(1);
            aut->addTransition(0, 1, labels[0]);
            aut->addTransition(1, 1, labels[1]);
            scores.emplace(scores.begin(), 0);
            scores.emplace(scores.begin() + 1, 6);
            break;
        case 4:
            aut->addFinalState(3);
            aut->addTransition(0, 1, labels[0]);
            aut->addTransition(1, 2, labels[1]);
            aut->addTransition(2, 3, labels[2]);
            scores.emplace(scores.begin(), 0);
            scores.emplace(scores.begin() + 1, 2); // 1 loop, 1 edge 0->1, thus 1*6+1 = 7
            scores.emplace(scores.begin() + 2, 1);
            scores.emplace(scores.begin() + 3, 0);
            break;
        case 5:
            aut->addFinalState(2);
            aut->addTransition(0, 1, labels[0]);
            aut->addTransition(1, 2, labels[1]);
            aut->addTransition(2, 2, labels[2]);
            scores.emplace(scores.begin(), 0);
            scores.emplace(scores.begin() + 1, 7);
            scores.emplace(scores.begin() + 2, 6);
            break;
        case 6:
            aut->addFinalState(2);
            aut->addTransition(0, 1, labels[0]);
            aut->addTransition(1, 1, labels[1]);
            aut->addTransition(1, 2, labels[2]);
            scores.emplace(scores.begin(), 0);
            scores.emplace(scores.begin() + 1, 7);
            scores.emplace(scores.begin() + 2, 0);
            break;
        case 7:
            aut->addFinalState(1);
            aut->addTransition(0, 1, labels[0]);
            aut->addTransition(0, 1, labels[1]);
            aut->addTransition(0, 1, labels[2]);
            aut->addTransition(1, 1, labels[3]);
            scores.emplace(scores.begin(), 0);
            scores.emplace(scores.begin() + 1, 6);
            break;
        case 8:
            aut->addFinalState(0);
            aut->addFinalState(1);
            aut->addTransition(0, 0, labels[0]);
            aut->addTransition(0, 1, labels[1]);
            aut->addTransition(1, 1, labels[1]);
            scores.emplace(scores.begin(), 13);
            scores.emplace(scores.begin() + 1, 6);
            break;
        case 9:
            aut->addFinalState(1);
            aut->addFinalState(2);
            aut->addTransition(0, 1, labels[0]);
            aut->addTransition(1, 1, labels[1]);
            aut->addTransition(1, 2, labels[2]);
            aut->addTransition(2, 2, labels[2]);
            scores.emplace_back(0);
            scores.emplace_back(13); // 2 loops, 1 edge 1->2, thus 2*6+1 = 13
            scores.emplace_back(6);
            break;
        case 10:
            aut->addFinalState(0);
            aut->addTransition(0, 0, labels[0]);
            aut->addTransition(0, 0, labels[1]);
            aut->addTransition(0, 0, labels[2]);
            scores.emplace_back(6);
            break;
        case 11:
            aut->addFinalState(4);
            aut->addTransition(0, 1, labels[0]);
            aut->addTransition(1, 2, labels[1]);
            aut->addTransition(2, 3, labels[2]);
            aut->addTransition(3, 4, labels[3]);
            scores.emplace(scores.begin(), 0);
            scores.emplace(scores.begin() + 1, 13); // 2 loops, 1 edge 1->2, thus 2*6+1 = 13
            scores.emplace(scores.begin() + 2, 6);
            scores.emplace(scores.begin() + 3, 0);
            scores.emplace(scores.begin() + 4, 13); // 2 loops, 1 edge 1->2, thus 2*6+1 = 13
            break;
        default:
            cout << "ERROR: Wrong query type" << endl;
            exit(1);
    }

    return scores;
}
