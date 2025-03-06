#ifndef S_PATH_H
#define S_PATH_H

#include <queue>
#include <unordered_set>
#include "fsa.h"
#include "rpq_forest.h"
#include "streaming_graph.h"

struct tree_expansion {
    long long vb;
    long long sb;
    long long vd;
    long long sd;
    long long edge_id;
};

struct visited_pair {
    long long vertex;
    long long state;

    bool operator==(const visited_pair &other) const {
        return vertex == other.vertex && state == other.state;
    }
};

struct visitedpairHash {
    size_t operator()(const visited_pair &p) const {
        return hash<long long>()(p.vertex) ^ hash<long long>()(p.state);
    }
};

struct result {
    long long destination;
    long long timestamp;

    bool operator==(const result &other) const {
        return destination == other.destination;
    }
};

struct resultHash {
    size_t operator()(const result &p) const {
        return hash<long long>()(p.destination);
        //^ hash<long long>()(p.timestamp);
    }
};

class SPathHandler {
public:
    FiniteStateAutomaton &fsa;
    Forest &forest;
    streaming_graph &sg;

    unordered_map<long long, std::unordered_set<result, resultHash> > result_set;
    // Maps source vertex to destination vertex and timestamp of path creation
    long long results_count = 0;

    SPathHandler(FiniteStateAutomaton &fsa, Forest &forest, streaming_graph &sg)
        : fsa(fsa), forest(forest), sg(sg) {
    }

    void printResultSet() {
        for (const auto &[source, destinations]: result_set) {
            for (const auto &destination: destinations) {
                cout << "Path from " << source << " to " << destination.destination << " at time " << destination.
                        timestamp << endl;
            }
        }
    }

    // export the result set into a file in form of csv with columns source, destination, timestamp
    void exportResultSet(const string &filename) {
        ofstream file(filename);
        for (const auto &[source, destinations]: result_set) {
            for (const auto &destination: destinations) {
                file << source << "," << destination.destination << "," << destination.timestamp << endl;
            }
        }
        file.close();
    }

    void pattern_matching(const sg_edge *edge) {
        auto statePairs = fsa.getStatePairsWithTransition(edge->label); // (sb, sd)
        for (const auto &sb_sd: statePairs) {
            // forall (sb, sd) where delta(sb, label) = sd
            if (sb_sd.first == 0 && !forest.hasTree(edge->s)) {
                // if sb=0 and there is no tree with root vertex vb
                forest.addTree(edge->id, edge->s, 0);
            }
            for (auto tree: forest.findTreesWithNode(edge->s, sb_sd.first)) {
                // for all Trees that contain ⟨vb,sb⟩
                std::unordered_set<visited_pair, visitedpairHash> visited; // set of visited pairs (vertex, state)
                queue<tree_expansion> Q; // (⟨vb,sb⟩, <vd,sd>, edge_id)
                Q.push((tree_expansion){edge->s, sb_sd.first, edge->d, sb_sd.second, edge->id});
                // push (⟨vb,sb⟩, <vd,sd>, edge_id) into Q
                visited.insert((visited_pair){edge->s, sb_sd.first});
                while (!Q.empty()) {
                    auto element = Q.front();
                    Q.pop(); // (⟨vi,si⟩, <vj,sj>, edge_id)
                    if (auto vj_sj_node = forest.findNodeInTree(tree.rootVertex, element.vd, element.sd); !vj_sj_node) {
                        // if tree does not contain <vj,sj>
                        // add <vj,sj> into tree with parent vi_si
                        if (!forest.addChildToParent(tree.rootVertex, element.vb, element.sb, element.edge_id,
                                                     element.vd, element.sd)) continue;
                    } else {
                        // if tree already contains <vj,sj>
                        // add vi_si to the list of candidate parents of <vj,sj>
                        if (auto candidate_parent = forest.findNodeInTree(tree.rootVertex, element.vb, element.sb))
                            vj_sj_node->candidate_parents.emplace_back(candidate_parent);
                    }
                    // update result set
                    if (fsa.isFinalState(element.sd)) {
                        // check if in the result set we already have a path from root to element.vd
                        if (result_set[tree.rootVertex].insert((result){element.vd, edge->timestamp}).second) {
                            results_count++;
                        }
                    }
                    // for all vertex <vq,sq> where exists a successor vertex in the snapshot graph where the label the same as the transition in the automaton from the state sj to state sq, push into Q
                    for (auto successors: sg.get_all_suc(element.vd)) {
                        // if the transition exits in the automaton from sj to sq
                        if (auto sq = fsa.getNextState(element.sd, successors.label); sq != -1) {
                            if (visited.count((visited_pair){successors.d, sq}) <= 0) {
                                // if <vq,sq> is not in visited
                                Q.push((tree_expansion){element.vd, element.sd, successors.d, sq, successors.id});
                                visited.insert((visited_pair){element.vd, element.sd});
                            }
                        }
                    }
                }
            }
        }
    }

    void compute_missing_results(unsigned id, unsigned s, unsigned d, unsigned l, unsigned time, unsigned window_close,
                                 vector<std::pair<long long, long long> > &windows, // windows to recover
                                 unordered_map<long long, vector<sg_edge *> > &windows_backup) {
        if (windows.empty()) return;

        const auto edge = new sg_edge(id, s, d, time, window_close);

        // retrieve the forest associated to the windows to which the element belongs, add the ooo element and compute results
        for (auto window: windows) {

            auto backupsg = new streaming_graph(-1);
            auto backupforest = new Forest();
            vector<sg_edge *> to_process;

            if (!windows_backup.empty()) {
                for (const auto &recovered_edge: windows_backup[window.second]) {
                    auto inserted_edge = backupsg->insert_edge(recovered_edge->id, recovered_edge->s,
                                           recovered_edge->d, recovered_edge->label,
                                           recovered_edge->timestamp,
                                           recovered_edge->expiration_time);
                    if (inserted_edge != nullptr) {
                        to_process.emplace_back(inserted_edge);
                    }
                }
            } else {
                // Recover the forest and the snapshot graph of the corresponding window
                backupforest->trees = forest.backup_map[std::make_pair(window.first, window.second)].first;
                backupforest->vertex_tree_map = forest.backup_map[std::make_pair(window.first, window.second)].second;

                backupsg->adjacency_list = sg.backup_aj[std::make_pair(window.first, window.second)];
            }

            auto ooo_edge = backupsg->insert_edge(edge->id, edge->s, edge->d, edge->label, edge->timestamp,
                                                          edge->expiration_time);
            if (ooo_edge != nullptr) {
                to_process.emplace_back(ooo_edge);
            }

            for (auto curr_edge: to_process) {
                // Update result set and relative forest
                auto statePairs = fsa.getStatePairsWithTransition(curr_edge->label); // (sb, sd)
                for (const auto &sb_sd: statePairs) {
                    // forall (sb, sd) where delta(sb, label) = sd
                    if (sb_sd.first == 0 && !backupforest->hasTree(curr_edge->s)) {
                        // if sb=0 and there is no tree with root vertex vb
                        backupforest->addTree(curr_edge->id, curr_edge->s, 0);
                    }
                    for (auto tree: backupforest->findTreesWithNode(curr_edge->s, sb_sd.first)) {
                        // for all Trees that contain ⟨vb,sb⟩
                        std::unordered_set<visited_pair, visitedpairHash> visited;
                        // set of visited pairs (vertex, state)
                        queue<tree_expansion> Q; // (⟨vb,sb⟩, <vd,sd>, edge_id)
                        Q.push((tree_expansion){curr_edge->s, sb_sd.first, curr_edge->d, sb_sd.second, curr_edge->id});
                        // push (⟨vb,sb⟩, <vd,sd>, edge_id) into Q
                        visited.insert((visited_pair){curr_edge->s, sb_sd.first});
                        while (!Q.empty()) {
                            auto element = Q.front();
                            Q.pop(); // (⟨vi,si⟩, <vj,sj>, edge_id)
                            if (!backupforest->findNodeInTree(tree.rootVertex, element.vd, element.sd)) {
                                // if tree does not contain <vj,sj>
                                // add <vj,sj> into tree with parent vi_si
                                if (!backupforest->addChildToParent(tree.rootVertex, element.vb, element.sb,
                                                                    element.edge_id, element.vd, element.sd))
                                    continue;
                            } else
                                continue;

                            if (fsa.isFinalState(element.sd)) {
                                // update result set
                                // check if in the result set we already have a path from root to element.vd
                                if (result_set[tree.rootVertex].insert((result){
                                    element.vd, curr_edge->timestamp
                                }).second) {
                                    results_count++;
                                }
                            }
                            // for all vertex <vq,sq> where exists a successor vertex in the snapshot graph where the label the same as the transition in the automaton from the state sj to state sq, push into Q
                            for (auto successors: backupsg->get_all_suc(element.vd)) {
                                // if the transition exits in the automaton from sj to sq
                                if (auto sq = fsa.getNextState(element.sd, successors.label); sq != -1) {
                                    if (visited.count((visited_pair){successors.d, sq}) <= 0) {
                                        // if <vq,sq> is not in visited
                                        Q.push((tree_expansion){
                                            element.vd, element.sd, successors.d, sq, successors.id
                                        });
                                        visited.insert((visited_pair){element.vd, element.sd});
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
};

#endif //S_PATH_H
