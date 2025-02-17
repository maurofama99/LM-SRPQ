//
// Created by Mauro Famà on 12/02/2025.
//

#ifndef S_PATH_H
#define S_PATH_H

#include <queue>
#include <unordered_set>
#include "fsa.h"
#include "rpq_forest.h"
#include "streaming_graph.h"

struct tree_expansion {
    int vb;
    int sb;
    int vd;
    int sd;
    int edge_id;
};

struct visited_pair {
    int vertex;
    int state;

    bool operator==(const visited_pair& other) const {
        return vertex == other.vertex && state == other.state;
    }
};

struct visitedpairHash {
    size_t operator()(const visited_pair& p) const {
        return hash<int>()(p.vertex) ^ hash<int>()(p.state);
    }
};

class SPathHandler {
public:

    FiniteStateAutomaton& fsa;
    Forest& forest;
    streaming_graph& sg;

    unordered_map<int,std::vector<::pair<int,unsigned>>> result; // Maps source vertex to destination vertex and timestamp of path creation
    int results_count = 0;

    SPathHandler(FiniteStateAutomaton& fsa, Forest& forest, streaming_graph& sg)
        : fsa(fsa), forest(forest), sg(sg) {
    }

    void printResultSet() const {
        for (const auto &pair : result) {
            if (pair.second.empty()) {
                continue;
            }
            std::cout << "Vertex " << pair.first << ":\n";
            for (const auto &res : pair.second) {
                std::cout << "  Destination: " << res.first << ", Timestamp: " << res.second << "\n";
            }
        }
    }

    void s_path(sg_edge* edge) {
        auto statePairs = fsa.getStatePairsWithTransition(edge->label); // (sb, sd)
        for (const auto& sb_sd : statePairs) {   // forall (sb, sd) where delta(sb, label) = sd
            if (sb_sd.first == 0 && !forest.hasTree(edge->s)) { // if sb=0 and there is no tree with root vertex vb
                forest.addTree(edge->id, edge->s, 0);
            }
            for (auto tree : forest.findTreesWithNode(edge->s, sb_sd.first)) {   // for all Trees that contain ⟨vb,sb⟩
                std::unordered_set<visited_pair, visitedpairHash> visited; // set of visited pairs (vertex, state)
                queue <tree_expansion> Q; // (⟨vb,sb⟩, <vd,sd>, edge_id)
                Q.push(tree_expansion{edge->s, sb_sd.first, edge->d, sb_sd.second, edge->id}); // push (⟨vb,sb⟩, <vd,sd>, edge_id) into Q
                visited.insert({edge->s, sb_sd.first});
                while (!Q.empty()) {
                    auto element = Q.front();
                    Q.pop(); // (⟨vi,si⟩, <vj,sj>, edge_id)
                    if (forest.findNodeInTree(tree.rootVertex, element.vd, element.sd) == nullptr) { // if tree does not contain <vj,sj>
                        // add <vj,sj> into tree with parent vi_si
                        forest.addChildToParent(tree.rootVertex, element.vb, element.sb, element.edge_id, element.vd, element.sd);
                        if (fsa.isFinalState(element.sd)) {
                            // update result set
                            result[tree.rootVertex].emplace_back(element.vd, edge->timestamp);
                            results_count++;
                        }
                    }
                    // for all vertex <vq,sq> where exists a successor vertex in the snapshot graph where the label the same as the transition in the automaton from the state sj to state sq, push into Q
                    for (auto successors: sg.get_all_suc(element.vd)) {
                        // if the transition exits in the automaton from sj to sq
                        auto sq = fsa.getNextState(element.sd, successors.label);
                        if (sq != -1) {
                            if (visited.count({successors.d, sq}) <= 0) { // if <vq,sq> is not in visited
                                Q.push(tree_expansion{element.vd, element.sd, successors.d, sq, successors.id});
                                visited.insert({element.vd, element.sd});
                            }
                        }
                    }
                }
            }
        }
    }

};

#endif //S_PATH_H
