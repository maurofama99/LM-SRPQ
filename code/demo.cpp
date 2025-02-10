#include<vector>
#include<string>
#include<ctime>
#include<cstdlib>
#include<algorithm>
#include<chrono>
#include "./source/S-PATH.h"
#include "./source/LM-SRPQ.h"

using namespace std;

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

int main(int argc, char *argv[]) {
    double candidate_rate = 0.2;
    double benefit_threshold = 1.5;
    unsigned int algorithm = atoi(argv[1]);
    string data_path = argv[2];
    ifstream fin(data_path.c_str());
    unsigned int days = atoi(argv[3]);
    unsigned int hour = atoi(argv[4]);
    unsigned int query_type = atoi(argv[5]);
    unsigned int window_type = 0;
    unsigned int scores[7];

    if (fin.fail()) {
        cerr << "Error opening file" << endl;
        exit(1);
    }

    auto *aut = new automaton;

    /*
    // Factory method pattern to create window operator
    window_factory *window_factory;
    window_operator *windowOperator;
    switch (window_type) {
        case 0: // TRIANGLE
            window_factory = new class triangle_factory();
            windowOperator = window_factory->createWindowOperator();
            break;
        case 1: // TRANSITIVITY
            window_factory = new class transitivity_factory();
            windowOperator = window_factory->createWindowOperator();
            break;
        case 2: // LABEL
            window_factory = new class label_factory();
            windowOperator = window_factory->createWindowOperator();
            break;
        case 3: // CENTRALITY
            window_factory = new class centrality_factory();
            windowOperator = window_factory->createWindowOperator();
            break;
        default:
            throw std::invalid_argument(&"Unknown window operator type: "[window_type]);
    }
    */

    unsigned int state_num = 0;

    if (query_type == 1) //Q1 a*
    {
        state_num = 1;
        aut->set_final_state(0);
        aut->insert_edge(0, 0, atoi(argv[6]));
        scores[0] = 6;
    } else if (query_type == 2) // Q2 a?b*
    {
        state_num = 2;
        aut->set_final_state(1);
        /// 0 is also a final state, namely empty string is acceptable, and (v, v) is a result pair, but I omit result like (v, v) in queries.
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(0, 1, atoi(argv[7]));
        aut->insert_edge(1, 1, atoi(argv[7]));
        scores[0] = 0; //score of s0 is set to 0 as it has no incoming edge
        scores[1] = 6;
    } else if (query_type == 3) // Q3 ab*
    {
        state_num = 2;
        aut->set_final_state(1);
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(1, 1, atoi(argv[7]));
        scores[0] = 0;
        scores[1] = 6;
    } else if (query_type == 4) /// Q4: abc
    {
        state_num = 4;
        aut->set_final_state(3);
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(1, 2, atoi(argv[7]));
        aut->insert_edge(2, 3, atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 2;
        scores[2] = 1;
        scores[3] = 0;
    } else if (query_type == 5) // Q5 abc*
    {
        state_num = 3;
        aut->set_final_state(2);
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(1, 2, atoi(argv[7]));
        aut->insert_edge(2, 2, atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 7;
        scores[2] = 6;
    } else if (query_type == 6) // Q6 ab*c
    {
        state_num = 3;
        aut->set_final_state(2);
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(1, 1, atoi(argv[7]));
        aut->insert_edge(1, 2, atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 7;
        scores[2] = 0;
    } else if (query_type == 7) // Q7 (a1+a2+a3)b*
    {
        state_num = 2;
        aut->set_final_state(1);
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(0, 1, atoi(argv[7]));
        aut->insert_edge(0, 1, atoi(argv[8]));
        aut->insert_edge(1, 1, atoi(argv[9]));
        scores[0] = 0;
        scores[1] = 6;
    } else if (query_type == 8) // Q7 a*b*
    {
        state_num = 2;
        aut->set_final_state(0);
        aut->set_final_state(1);
        aut->insert_edge(0, 0, atoi(argv[6]));
        aut->insert_edge(0, 1, atoi(argv[7]));
        aut->insert_edge(1, 1, atoi(argv[7]));
        scores[0] = 13;
        scores[1] = 6;
    } else if (query_type == 9) // Q9 ab*c*
    {
        state_num = 3;
        aut->set_final_state(1);
        aut->set_final_state(2);
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(1, 1, atoi(argv[7]));
        aut->insert_edge(1, 2, atoi(argv[8]));
        aut->insert_edge(2, 2, atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 13; /// 2 loops, 1 edge 1->2, thus 2*6+1 = 13
        scores[2] = 6;
    } else if (query_type == 10) /// Q10 (a1+a2+a3)*
    {
        state_num = 1;
        aut->set_final_state(0);
        aut->insert_edge(0, 0, atoi(argv[6]));
        aut->insert_edge(0, 0, atoi(argv[7]));
        aut->insert_edge(0, 0, atoi(argv[8]));
        scores[0] = 6;
    } else if (query_type == 11) { // abcd
        state_num = 5;
        aut->set_final_state(4);
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(1, 2, atoi(argv[7]));
        aut->insert_edge(2, 3, atoi(argv[8]));
        aut->insert_edge(3, 4, atoi(argv[9]));
        scores[0] = 0;
        scores[1] = 13; /// 2 loops, 1 edge 1->2, thus 2*6+1 = 13
        scores[2] = 6;
        scores[3] = 0;
        scores[4] = 13; /// 2 loops, 1 edge 1->2, thus 2*6+1 = 13

    }
    else {
        cout << "wrong query type" << endl;
        return 0;
    }

    unsigned int w = days *3600 ;
    auto *sg = new streaming_graph();
    unsigned long s, d, l;
    unsigned long long t;
    string prefix = "./"; // path of the output files
    string postfix = ".txt";
    unsigned int slice = 0;
    unsigned int t0 = 0;
    vector<unsigned int> insertion_time;
    unsigned int edge_number = 0;

    if (algorithm == 1) {
        int checkpoint = 1;
        auto *f1 = new RPQ_forest(sg, aut);
        ofstream fout1((prefix + "S-PATH-memory" + postfix).c_str());
        // memory of S-PATH, memory measured at each checkpoint will be output, including
        ofstream fout2((prefix + "S-PATH-insertion-latency" + postfix).c_str());
        ofstream fout3((prefix + "S-PATH-speed" + postfix).c_str()); //time used by two algorithms
        clock_t start = clock();

        // convert size and slide from hours to seconds
        unsigned int size = w;
        unsigned int slide = hour *3600;
        sg->slide = slide;
        if (size % slide != 0) {
            printf("size is not a multiple of slide\n");
            return 0;
        }

        unsigned int time;
        unsigned frontier = 0;
        unsigned eviction_trigger = size;

        vector<window> windows;
        unsigned int current_window_index = 0;
        unsigned int window_offset = 0;
        windows.emplace_back(0, size, nullptr, nullptr);

        while (fin >> s >> d >> l >> t) {
            edge_number++;
            if (t0 == 0) {
                t0 = t;
                time = 1;
            } else time = t - t0;

            if (aut->acceptable_labels.find(l) == aut->acceptable_labels.end()) // if the edge is not a part of the regular expression, we do not process this edge;
                continue;

            // scope
            double c_sup = ceil(time / slide) * slide;
            double o_i = c_sup - size;

            unsigned int window_close;
            do {
                window_close = o_i + size;
                if (windows[current_window_index].t_close < window_close) {
                    windows.emplace_back(o_i, window_close, nullptr, nullptr);
                    current_window_index++;
                    // cout << "window [" << o_i << ", " << window_close << "] created\n";
                }
                o_i += slide;
            } while (o_i <= time);

            // add + query
            timed_edge* t_edge;
            sg_edge* new_edge = f1->insert_edge_extended(s, d, l, time, window_close);

            // duplicate handling, important to not evict edges being updated
            if (!new_edge) {
                // search for the duplicate
                auto existing_edge = sg->search_existing_edge(s, d, l);

                // update window open pointers if needed
                for (size_t i = window_offset; i < windows.size(); i++) {
                    if (windows[i].first == existing_edge->time_pos) {
                        // shift window first to next element
                        windows[i].first = existing_edge->time_pos->next;
                    }
                }

                // update edge list (erase and re-append)
                sg->delete_timed_edge(existing_edge->time_pos);

                new_edge = existing_edge;
                new_edge->timestamp = time;
            }

            t_edge = new timed_edge(new_edge);
            sg->add_timed_edge(t_edge);
            new_edge->time_pos = t_edge;

            vector<size_t> to_evict;
            bool evict = false;
            for (size_t i = window_offset; i < windows.size(); i++) {
                if (windows[i].t_open <= time && time < windows[i].t_close) { // add to window
                    if (!windows[i].first) windows[i].first = t_edge;
                    windows[i].last = t_edge;
                } else if (time > windows[i].t_close) { // schedule for eviction
                    window_offset = i+1;
                    to_evict.push_back(i);
                    evict = true;
                }
            }

            // evict
            if (evict) {
                //cout << "adjacency list size ante evict: " << sg->adjacency_list.size() << endl;
                vector<edge_info> deleted_edges;
                timed_edge* evict_start_point = windows[to_evict[0]].first;
                timed_edge* evict_end_point = windows[to_evict.back()+1].first;

                if (!evict_start_point) {
                    cout << "evict start point is null" << endl;
                    exit(1);
                }

                if (evict_end_point == nullptr) {
                    evict_end_point = sg->time_list_tail;
                    cout << "Evict end point is null, evicting whole buffer." << endl;
                }

                /*
                cout << "end point timestamps: " << evict_end_point->edge_pt->timestamp << endl;
                cout << "windows[to_evict.back()+1].first timestamp " << windows[to_evict.back()+1].first->edge_pt->timestamp << endl;
                cout << "evict end point: " << evict_end_point << ", timed edge pointer: " << evict_end_point->edge_pt->time_pos << endl;
                */

                timed_edge* current = evict_start_point;
                while (current && current != evict_end_point) {
                    // cout << "evicting edge (" << current->edge_pt->s << ", " << current->edge_pt->d << ", " << current->edge_pt->label << ", " << current->edge_pt->timestamp << ")\n";
                    auto cur_edge = current->edge_pt;
                    deleted_edges.emplace_back(cur_edge->s, cur_edge->d, cur_edge->timestamp, cur_edge->label, cur_edge->expiration_time);
                    sg->remove_edge(cur_edge->s, cur_edge->d, cur_edge->label); // update adjacency list of snapshot graph
                    auto tmp  = current->next;
                    sg->delete_timed_edge(current);
                    current = tmp;
                }
                for (unsigned long i : to_evict) {
                    windows[i].evicted = true;
                    windows[i].first = nullptr;
                    windows[i].last = nullptr;
                }
                to_evict.clear();

                sg->time_list_head = evict_end_point;
                sg->time_list_head->prev = nullptr;

                f1->expire(time, deleted_edges);
            }

            if (time >= checkpoint*3600) {
                checkpoint += checkpoint;
                f1->count(fout1);
                /*
                for (const auto &window : windows) {
                    if (window.evicted) continue;
                    cout << "Window [" << window.t_open << ", " << window.t_close << "]\n";
                    if (window.first) {
                        cout << "First Edge: (" << window.first->edge_pt->s << ", " << window.first->edge_pt->d << ", " << window.first->edge_pt->label << ", " << window.first->edge_pt->timestamp << ")\n";
                    }
                    if (window.last) {
                        cout << "Last Edge: (" << window.last->edge_pt->s << ", " << window.last->edge_pt->d << ", " << window.last->edge_pt->label << ", " << window.last->edge_pt->timestamp << ")\n";
                    }
                    cout << endl;
                }
                */
            }
        }
        f1->count(fout1);

        printf("edge number: %u\n", edge_number);
        printf("saved edges: %d\n", sg->saved_edges);
        printf("avg degree: %f\n", sg->mean);
        printf("RESULTS:\ndistinct paths: %d, peek memory: %f, average memory: %f\n", f1->distinct_results, f1->peek_memory, f1->memory_current_avg);
        printf("expand calls: %u\n", f1->expand_call);

        clock_t finish = clock();
        cout << "insert finished " << endl;
        unsigned int time_used = (double) (finish - start) / CLOCKS_PER_SEC;
        cout << "S-PATH time used " << time_used << endl;
        cout << "S-PATH speed " << double(edge_number) / time_used << endl;
        fout3 << " S-PATH time used " << time_used << endl;
        fout3 << "S-PATH speed " << double(edge_number) / time_used << endl;
        sort(insertion_time.begin(), insertion_time.end(), compare);
        if (insertion_time.size() > (edge_number / 100))
            insertion_time.erase(insertion_time.begin() + (edge_number / 100), insertion_time.end());
        for (int i = 0; i < insertion_time.size(); i++)
            fout2 << insertion_time[i] << endl;

        insertion_time.clear();
        fout1.close();
        fout2.close();
        fout3.close();
        delete f1;
    }

    if (algorithm == 2) {
        ofstream fout1(
            (prefix + "LM-memory_" + std::to_string(days) + "_" + std::to_string(hour) + "_" + to_string(query_type) +
             postfix).c_str()); // memory of S-PATH, memory measured at each checkpoint will be output, including
        ofstream fout2((prefix + "LM-insertion-latency" + postfix).c_str());
        ofstream fout3((prefix + "LM-speed" + postfix).c_str()); //time used by two algorithms
        ofstream fout4((prefix + "LM-output" + postfix).c_str());

        auto *f2 = new LM_forest(sg, aut);
        for (int i = 0; i < state_num; i++)
            f2->aut_scores[i] = scores[i];
        clock_t start = clock();
        while (fin >> s >> d >> l >> t) {
            edge_number++;
            if (t0 == 0)
                t0 = t;
            unsigned int time = t - t0 + 1;

            auto insertion_start = std::chrono::steady_clock::now();
            f2->update_snapshot_graph(l, t, s, d);
            f2->insert_edge(s, d, l, time);
            auto insertion_end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<chrono::microseconds>(insertion_end - insertion_start);
            insertion_time.push_back(duration.count());

            if (time / (3600 * hour) > slice) {
                slice++;
                f2->expire(time);
                // f2->dynamic_lm_select(candidate_rate, benefit_threshold);

                cout << slice << " slices have been inserted" << endl;
                if (slice > 0 && slice % (days * 24 / hour) == 0)
                    f2->count(fout1);
            }
        }
        clock_t finish = clock();
        cout << "insert finished " << endl;
        unsigned int time_used = (double) (finish - start) / CLOCKS_PER_SEC;
        cout << "LM-SRPQ time used " << time_used << endl;
        cout << "LM-SRPQ speed " << (double) edge_number / time_used << endl;
        fout3 << " LM-SRPQ time used " << time_used << endl;
        fout3 << "LM-SRPQ speed " << (double) edge_number / time_used << endl;

        sort(insertion_time.begin(), insertion_time.end(), compare);
        if (insertion_time.size() > (edge_number / 100))
            insertion_time.erase(insertion_time.begin() + (edge_number / 100), insertion_time.end());
        for (int i = 0; i < insertion_time.size(); i++)
            fout2 << insertion_time[i] << endl;

        insertion_time.clear();
        fout1.close();
        fout2.close();
        fout3.close();
        delete f2;
    }

    delete sg;
    return 0;
}
