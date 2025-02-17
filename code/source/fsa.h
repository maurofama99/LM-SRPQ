//
// Created by Mauro Famà on 12/02/2025.
//

#ifndef FSA_H
#define FSA_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class FiniteStateAutomaton {
public:
    struct Transition {
        int fromState;
        int toState;
        int label;
    };
    
    std::unordered_map<int, std::vector<Transition>> transitions;
    int initialState;
    std::unordered_set<int> finalStates;
    
    FiniteStateAutomaton() : initialState(0) {}
    
    void addTransition(int fromState, int toState, int label) {
        transitions[fromState].push_back({fromState, toState, label});
    }
    
    void addFinalState(int state) {
        finalStates.insert(state);
    }
    
    std::vector<std::pair<int, int>> getStatePairsWithTransition(int label) const {
        std::vector<std::pair<int, int>> statePairs;
        for (const auto& pair : transitions) {
            for (const auto& transition : pair.second) {
                if (transition.label == label) {
                    statePairs.emplace_back(transition.fromState, transition.toState);
                }
            }
        }
        return statePairs;
    }
    
    int getNextState(int currentState, int label) {
        for (const auto& transition : transitions[currentState]) {
            if (transition.label == label) {
                return transition.toState;
            }
        }
        return -1; // No valid transition
    }
    
    std::vector<std::pair<int, int>> getAllSuccessors(int state) {
        std::vector<std::pair<int, int>> successors;
        for (const auto& transition : transitions[state]) {
            successors.emplace_back(transition.label, transition.toState);
        }
        return successors;
    }
    bool hasLabel(const int label) const {
        for (const auto& pair : transitions) {
            for (const auto& transition : pair.second) {
                if (transition.label == label) {
                    return true;
                }
            }
        }
        return false;
    }

    bool isFinalState(int state) const {
        return finalStates.find(state) != finalStates.end();
    }
    
    void printTransitions() const {
        for (const auto& pair : transitions) {
            for (const auto& transition : pair.second) {
                std::cout << "From State " << transition.fromState << " to State " << transition.toState << " on Label '" << transition.label << "'\n";
            }
        }
    }
};

#endif //FSA_H
