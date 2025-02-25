#include<vector>
#include<string>
#include<ctime>
#include<cstdlib>
#include<algorithm>
#include <fstream>

#include "fsa.h"
#include "rpq_forest.h"
#include "streaming_graph.h"
#include "s_path.h"

#define BACKWARD_RETENTION true

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

int setup_automaton(int query_type, FiniteStateAutomaton *aut, unsigned int *scores, char *argv[]);

int main(int argc, char *argv[]) {
    unsigned int algorithm = atoi(argv[1]);
    string data_path = argv[2];
    ifstream fin(data_path.c_str());
    unsigned int gap = atoi(argv[3]);
    unsigned int max_duration = atoi(argv[4]);
    unsigned int query_type = atoi(argv[5]);
    unsigned int scores[7];

    if (gap > max_duration) {
        printf("ERROR: Session gap is greater than max duration\n");
        exit(1);
    }

    if (fin.fail()) {
        cerr << "Error opening file" << endl;
        exit(1);
    }

    auto *aut = new FiniteStateAutomaton();
    setup_automaton(query_type, aut, scores, argv);
    auto *f = new Forest();
    auto *sg = new streaming_graph();

    // it exists only one initial state for fsa, hence, the pair <vertex,s0> is unique
    std::unordered_map<set<unsigned int>,window> windows;

    unsigned int s, d, l;
    unsigned int t;
    string prefix = "./"; // path of the output files
    string postfix = ".txt";

    unsigned int t0 = 0;
    vector<unsigned int> insertion_time;
    unsigned int edge_number = 0;

    if (algorithm == 1) {
        int checkpoint = 1;
        auto query = new SPathHandler(*aut, *f, *sg);

        ofstream fout1((prefix + "S-PATH-memory" + postfix).c_str());
        ofstream fout2((prefix + "S-PATH-insertion-latency" + postfix).c_str());
        ofstream fout3((prefix + "S-PATH-speed" + postfix).c_str()); //time used by two algorithms
        clock_t start = clock();

        unsigned int time;

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


            /* ADD */
            // update adjacency list of snapshot graph, if edge exists, return null
            // cout << "DEBUG: Inserting edge (" << s << ", " << d << ", " << l << ", " << time << ")\n";
            sg_edge *new_sgt = sg->insert_edge(edge_number, s, d, l, time, time + gap);

            if (!new_sgt) {
                // TODO Extend window close of the duplicate edge
                continue;
            }

            //f->findTreesWithNode();



            // add sgt to RPQ forest
            query->s_path(new_sgt);
            // f->printForest();

            // add edge to time list
            auto t_edge = new timed_edge(new_sgt);
            sg->add_timed_edge(t_edge);
            new_sgt->time_pos = t_edge;





            /* EVICT */


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
                printf("edge number: %u\n", edge_number);
                printf("saved edges: %d\n", sg->saved_edges);
                printf("avg degree: %f\n", sg->mean);
                printf("results: %d\n\n", query->results_count);
            }
        }

        // query->printResultSet();
        printf("results count: %d\n", query->results_count);
        query->exportResultSet("/Users/maurofama/Documents/phd/frames4pgs/CbAW4DGSP/code/scripts/neo4j/stream_results_my.csv");

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
        for (unsigned int i: insertion_time)
            fout2 << i << endl;

        insertion_time.clear();
        fout1.close();
        fout2.close();
        fout3.close();
        delete f;
    }

    delete
            sg;
    return
            0;
}

// Set up the automaton correspondant for each query
int setup_automaton(int query_type, FiniteStateAutomaton *aut, unsigned int *scores, char *argv[]) {
    int state_num = 0;

    if (query_type == 1) {
        state_num = 1;
        aut->addFinalState(0);
        aut->addTransition(0, 0, atoi(argv[6]));
        scores[0] = 6;
    } else if (query_type == 2) {
        state_num = 2;
        aut->addFinalState(1);
        aut->addTransition(0, 1, atoi(argv[6]));
        aut->addTransition(0, 1, atoi(argv[7]));
        aut->addTransition(1, 1, atoi(argv[7]));
        scores[0] = 0;
        scores[1] = 6;
    } else if (query_type == 3) { // ab*
        state_num = 2;
        aut->addFinalState(1);
        aut->addTransition(0, 1, atoi(argv[6]));
        aut->addTransition(1, 1, atoi(argv[7]));
        scores[0] = 0;
        scores[1] = 6;
    } else if (query_type == 4) {
        state_num = 4;
        aut->addFinalState(3);
        aut->addTransition(0, 1, atoi(argv[6]));
        aut->addTransition(1, 2, atoi(argv[7]));
        aut->addTransition(2, 3, atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 2;
        scores[2] = 1;
        scores[3] = 0;
    } else if (query_type == 5) {
        state_num = 3;
        aut->addFinalState(2);
        aut->addTransition(0, 1, atoi(argv[6]));
        aut->addTransition(1, 2, atoi(argv[7]));
        aut->addTransition(2, 2, atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 7;
        scores[2] = 6;
    } else if (query_type == 6) {
        state_num = 3;
        aut->addFinalState(2);
        aut->addTransition(0, 1, atoi(argv[6]));
        aut->addTransition(1, 1, atoi(argv[7]));
        aut->addTransition(1, 2, atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 7;
        scores[2] = 0;
    } else if (query_type == 7) {
        state_num = 2;
        aut->addFinalState(1);
        aut->addTransition(0, 1, atoi(argv[6]));
        aut->addTransition(0, 1, atoi(argv[7]));
        aut->addTransition(0, 1, atoi(argv[8]));
        aut->addTransition(1, 1, atoi(argv[9]));
        scores[0] = 0;
        scores[1] = 6;
    } else if (query_type == 8) {
        state_num = 2;
        aut->addFinalState(0);
        aut->addFinalState(1);
        aut->addTransition(0, 0, atoi(argv[6]));
        aut->addTransition(0, 1, atoi(argv[7]));
        aut->addTransition(1, 1, atoi(argv[7]));
        scores[0] = 13;
        scores[1] = 6;
    } else if (query_type == 9) {
        state_num = 3;
        aut->addFinalState(1);
        aut->addFinalState(2);
        aut->addTransition(0, 1, atoi(argv[6]));
        aut->addTransition(1, 1, atoi(argv[7]));
        aut->addTransition(1, 2, atoi(argv[8]));
        aut->addTransition(2, 2, atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 13;
        scores[2] = 6;
    } else if (query_type == 10) {
        state_num = 1;
        aut->addFinalState(0);
        aut->addTransition(0, 0, atoi(argv[6]));
        aut->addTransition(0, 0, atoi(argv[7]));
        aut->addTransition(0, 0, atoi(argv[8]));
        scores[0] = 6;
    } else if (query_type == 11) {
        state_num = 5;
        aut->addFinalState(4);
        aut->addTransition(0, 1, atoi(argv[6]));
        aut->addTransition(1, 2, atoi(argv[7]));
        aut->addTransition(2, 3, atoi(argv[8]));
        aut->addTransition(3, 4, atoi(argv[9]));
        scores[0] = 0;
        scores[1] = 13;
        scores[2] = 6;
        scores[3] = 0;
        scores[4] = 13;
    } else {
        cout << "ERROR: Wrong query type" << endl;
        exit(1);
    }

    return state_num;
}
