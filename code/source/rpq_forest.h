//
// Created by Mauro Famà on 12/02/2025.
//

#ifndef RPQ_FOREST_H
#define RPQ_FOREST_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include "streaming_graph.h"

struct Node {
    int id;
    int vertex;
    int state;
    std::vector<Node*> children;
    Node* parent;

    Node(int id, int vertex, int state, Node* parent)
        : id(id), vertex(vertex), state(state), parent(parent){}
};

struct Tree {
    int rootVertex;
    Node* rootNode;
    bool expired;

    bool operator==(const Tree& other) const {
        return rootVertex == other.rootVertex;
    }
};

struct TreeHash {
    size_t operator()(const Tree& p) const {
        return hash<int>()(p.rootVertex);
    }
};

class Forest {
public:
    std::unordered_map<int, Tree> trees; // Currently active trees, Key: root vertex, Value: Tree
    std::unordered_map<int, std::unordered_set<Tree, TreeHash>> vertex_tree_map; // Maps vertex to tree to which it belongs to

    ~Forest() {
        for (auto& pair : trees) {
            deleteTree(pair.second.rootNode);
        }
    }

    // a vertex can be root of only one tree
    // proof: since we have only one initial state and the tree has an initial state as root
    // it exists only one pair vertex-state that can be root of a tree
    void addTree(int rootId, int rootVertex, int rootState) {
        if (trees.find(rootVertex) == trees.end()) {
            trees[rootVertex] = {rootVertex, new Node(rootId, rootVertex, rootState, nullptr), false};
            vertex_tree_map[rootVertex].insert(trees[rootVertex]);
        }
    }

    void addChildToParent(int rootVertex, int parentVertex, int parentState, int childId, int childVertex, int childState) {
        Node* parent = findNodeInTree(rootVertex, parentVertex, parentState);
        if (parent) {
            parent->children.push_back(new Node(childId, childVertex, childState, parent));
            vertex_tree_map[childVertex].insert(trees[rootVertex]);
        }
    }

    bool hasTree(int rootVertex) {
        return trees.find(rootVertex) != trees.end();
    }

    Node* findNodeInTree(int rootVertex, int vertex, int state, Node** parent = nullptr) {
        Node* root = trees[rootVertex].rootNode;
        return searchNodeParent(root, vertex, state, parent);
    }

    std::vector<Tree> findTreesWithNode(int vertex, int state) {
        std::vector<Tree> result;
        for (auto &tree: trees) {
            Node *node = searchNode(tree.second.rootNode, vertex, state);
            if (node) result.push_back(tree.second);
        }
        return result;
    }

    std::unordered_set<Tree, TreeHash> getTreesOfVertex(int vertex) const {
        if (vertex_tree_map.find(vertex) != vertex_tree_map.end()) {
            return vertex_tree_map.at(vertex);
        }
        return {};
    }

    void expire(const std::unordered_set<int> & vertexes) {
        std::cout << "DEBUG: Deleting vertex ";
        for (auto toprint: vertexes) std::cout << toprint << ", ";
        cout << std::endl;

        // Map each vertex to the trees it belongs to
        // When we need to delete a vertex, we trace back to the trees and navigate them until we find the corresponding node
        // From the corresponding node, we delete the node and its entire subtree
        // During the traversal of the subtree, for each node
            // we access the map and remove the current tree from the vertex we are visiting
        // Remove the vertex from the map
        for (int vertex : vertexes) {
            if (vertex_tree_map.find(vertex) == vertex_tree_map.end()) continue;
            auto treesSet = getTreesOfVertex(vertex);
            for (auto tree : treesSet) {
                Node* current = searchNodeNoState(tree.rootNode, vertex);
                if (current) {
                    if (current->parent) {
                        deleteSubTreeIterative(current, tree.rootVertex);
                    } else { // current vertex is the root node
                        deleteTreeIterative(tree.rootNode, tree.rootVertex);
                        tree.expired = true;
                        trees.erase(vertex);
                    }
                }
            }
            vertex_tree_map.erase(vertex);
        }
    }

    void printTree(Node *node, int depth = 0) const {
        if (!node) return;
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "Node(id: " << node->id << ", vertex: " << node->vertex << ", state: " << node->state << ")\n";
        for (Node *child: node->children) {
            printTree(child, depth + 1);
        }
    }

    void printForest() const {
        if (trees.empty()) {
            std::cout << "Empty forest\n";
            return;
        }
        for (const auto &pair: trees) {
            std::cout << "Tree with root vertex: " << pair.first << "\n";
            printTree(pair.second.rootNode);
            std::cout << std::endl;
        }
    }

private:

    Node* searchNodeParent(Node* node, int vertex, int state, Node** parent = nullptr) {
        if (!node) return nullptr;
        if (node->vertex == vertex && (node->state == state || state == -2)) return node;
        for (Node* child : node->children) {
            if (parent) *parent = node;
            Node* found = searchNodeParent(child, vertex, state, parent);
            if (found) return found;
        }
        return nullptr;
    }

    Node *searchNode(Node *node, int vertex, int state) {
        if (!node) return nullptr;
        if (node->vertex == vertex && node->state == state) return node;
        for (Node *child: node->children) {
            Node *found = searchNode(child, vertex, state);
            if (found) return found;
        }
        return nullptr;
    }

    Node *searchNodeNoState(Node *node, int vertex) {
        if (!node) return nullptr;
        if (node->vertex == vertex) return node;
        for (Node *child: node->children) {
            Node *found = searchNodeNoState(child, vertex);
            if (found) return found;
        }
        return nullptr;
    }

    void deleteSubTreeIterative(Node *node, int rootVertex) {
        if (!node) return;
        std::stack<Node *> stack;
        stack.push(node);
        while (!stack.empty()) {
            Node *current = stack.top();
            stack.pop();
            for (Node *child: current->children) {
                stack.push(child);
            }
            // if the vertex_tree_map at current->vertex contains the tree with rootVertex, remove it
            if (vertex_tree_map.find(current->vertex) != vertex_tree_map.end()) {
                auto it = vertex_tree_map[current->vertex].begin();
                while (it != vertex_tree_map[current->vertex].end()) {
                    if (it->rootVertex == rootVertex) {
                        vertex_tree_map[current->vertex].erase(it);
                        break;
                    }
                    ++it;
                }
            }
            if (vertex_tree_map[current->vertex].empty()) {
                vertex_tree_map.erase(current->vertex);
            }
            // Restore the parent's children pointers
            if (current->parent) {
                auto &siblings = node->parent->children;
                siblings.erase(std::remove(siblings.begin(), siblings.end(), node), siblings.end());
            } else {
                std::cout << "WARNING: Deleting root node in a subtree deletion procedure" << std::endl;
                delete current;
            }

        }
    }

    void deleteTreeIterative(Node* node, int rootVertex) {
        if (!node) return;
        std::stack<Node*> stack;
        stack.push(node);
        while (!stack.empty()) {
            Node* current = stack.top();
            stack.pop();
            for (Node* child : current->children) {
                stack.push(child);
            }
            // if the vertex_tree_map at current->vertex contains the tree with rootVertex, remove it
            if (vertex_tree_map.find(current->vertex) != vertex_tree_map.end()) {
                auto it = vertex_tree_map[current->vertex].begin();
                while (it != vertex_tree_map[current->vertex].end()) {
                    if (it->rootVertex == rootVertex) {
                        vertex_tree_map[current->vertex].erase(it);
                        break;
                    }
                    ++it;
                }
            }

            // if the vertex_tree_map at current->vertex is empty, remove the entry
            if (vertex_tree_map[current->vertex].empty()) {
                vertex_tree_map.erase(current->vertex);
            }
            delete current;
        }
    }

    void deleteTree(Node* node) {
        if (!node) return;
        for (Node* child : node->children) {
            deleteTree(child);
        }
        delete node;
    }

};

#endif //RPQ_FOREST_H
