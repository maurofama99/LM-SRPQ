#ifndef S_PATH_H
#define S_PATH_H

#include <queue>
#include <unordered_set>
#include "fsa.h"
#include "rpq_forest.h"
#include "streaming_graph.h"

struct tree_expansion {
    unsigned int vb;
    int sb;
    unsigned int vd;
    int sd;
    int edge_id;
};

struct visited_pair {
    unsigned int vertex;
    int state;

    bool operator==(const visited_pair &other) const {
        return vertex == other.vertex && state == other.state;
    }
};

struct visitedpairHash {
    size_t operator()(const visited_pair &p) const {
        return hash<unsigned int>()(p.vertex) ^ hash<int>()(p.state);
    }
};

struct result {
    unsigned int destination;
    unsigned int timestamp;

    bool operator==(const result &other) const {
        return destination == other.destination;
    }
};

struct resultHash {
    size_t operator()(const result &p) const {
        return hash<unsigned int>()(p.destination);
        //^ hash<unsigned int>()(p.timestamp);
    }
};

class SPathHandler {
public:
    FiniteStateAutomaton &fsa;
    Forest &forest;
    streaming_graph &sg;

    unordered_map<unsigned int, std::unordered_set<result, resultHash> > result_set;
    // Maps source vertex to destination vertex and timestamp of path creation
    int results_count = 0;

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
                    if (!forest.findNodeInTree(tree.rootVertex, element.vd, element.sd)) {
                        // if tree does not contain <vj,sj>
                        // add <vj,sj> into tree with parent vi_si
                        if (!forest.addChildToParent(tree.rootVertex, element.vb, element.sb, element.edge_id,
                                                     element.vd, element.sd)) continue;
                        // } else if (forest.findNodeInTree(tree.rootVertex, element.vb, element.sb) && element.vb == edge->s && element.vd == edge->d) { // if tree contains <vi,si> update subtree starting from <vi,si> to respect LILO
                        //  forest.changeParent(tree.rootVertex, element.vd, element.sd, element.vb, element.sb);
                    } else // TODO Implement FIFO parent update
                        continue;

                    if (fsa.isFinalState(element.sd)) {
                        // update result set
                        // check if in the result set we already have a path from root to element.vd
                        if (result_set[tree.rootVertex].insert((result){element.vd, edge->timestamp}).second) {
                            results_count++;
                        }
                    }
                    /*
                    else if (forest.findNodeInTree(tree.rootVertex, element.vb, element.sb) && (element.vb == edge->s && element.vd == edge->d)) { // if tree contains <vi,si>
                        // update the parent of the node to respect FIFO
                        forest.changeParent(tree.rootVertex, element.vd, element.sd, element.vb, element.sb);
                    } else continue;
                    */
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
                                 vector<std::pair<unsigned int, unsigned int> > &windows) {
        if (windows.empty()) return;

        const auto edge = new sg_edge(id, s, d, time, window_close);

        // retrieve the forest associated to the windows to which the element belongs, add the ooo element and compute results
        for (auto window: windows) {

            // Recover the forest and the snapshot graph of the corresponding window
            auto backupforest = new Forest();
            backupforest->trees = forest.backup_map[std::make_pair(window.first, window.second)].first;
            backupforest->vertex_tree_map = forest.backup_map[std::make_pair(window.first, window.second)].second;

            auto backupsg = new streaming_graph(-1);
            backupsg->adjacency_list = sg.backup_aj[std::make_pair(window.first, window.second)];

            // Update result set and relative forest
            auto statePairs = fsa.getStatePairsWithTransition(edge->label); // (sb, sd)
            for (const auto &sb_sd: statePairs) {
                // forall (sb, sd) where delta(sb, label) = sd
                if (sb_sd.first == 0 && !backupforest->hasTree(edge->s)) {
                    // if sb=0 and there is no tree with root vertex vb
                    backupforest->addTree(edge->id, edge->s, 0);
                }
                for (auto tree: backupforest->findTreesWithNode(edge->s, sb_sd.first)) {
                    // for all Trees that contain ⟨vb,sb⟩
                    std::unordered_set<visited_pair, visitedpairHash> visited; // set of visited pairs (vertex, state)
                    queue<tree_expansion> Q; // (⟨vb,sb⟩, <vd,sd>, edge_id)
                    Q.push((tree_expansion){edge->s, sb_sd.first, edge->d, sb_sd.second, edge->id});
                    // push (⟨vb,sb⟩, <vd,sd>, edge_id) into Q
                    visited.insert((visited_pair){edge->s, sb_sd.first});
                    while (!Q.empty()) {
                        auto element = Q.front();
                        Q.pop(); // (⟨vi,si⟩, <vj,sj>, edge_id)
                        if (!backupforest->findNodeInTree(tree.rootVertex, element.vd, element.sd)) {
                            // if tree does not contain <vj,sj>
                            // add <vj,sj> into tree with parent vi_si
                            if (!backupforest->addChildToParent(tree.rootVertex, element.vb, element.sb,
                                                                element.edge_id, element.vd, element.sd)) continue;
                            // } else if (forest.findNodeInTree(tree.rootVertex, element.vb, element.sb) && element.vb == edge->s && element.vd == edge->d) { // if tree contains <vi,si> update subtree starting from <vi,si> to respect LILO
                            //  forest.changeParent(tree.rootVertex, element.vd, element.sd, element.vb, element.sb);
                        } else // TODO Implement FIFO parent update
                            continue;

                        if (fsa.isFinalState(element.sd)) {
                            // update result set
                            // check if in the result set we already have a path from root to element.vd
                            if (result_set[tree.rootVertex].insert((result){element.vd, edge->timestamp}).second) {
                                results_count++;
                            }
                        }
                        /*
                        else if (forest.findNodeInTree(tree.rootVertex, element.vb, element.sb) && (element.vb == edge->s && element.vd == edge->d)) { // if tree contains <vi,si>
                            // update the parent of the node to respect FIFO
                            forest.changeParent(tree.rootVertex, element.vd, element.sd, element.vb, element.sb);
                        } else continue;
                        */
                        // for all vertex <vq,sq> where exists a successor vertex in the snapshot graph where the label the same as the transition in the automaton from the state sj to state sq, push into Q
                        for (auto successors: backupsg->get_all_suc(element.vd)) {
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
    }
};

#endif //S_PATH_H
