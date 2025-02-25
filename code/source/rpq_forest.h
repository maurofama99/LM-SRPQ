//
// Created by Mauro Famà on 12/02/2025.
//

#ifndef RPQ_FOREST_H
#define RPQ_FOREST_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <stack>

#include "streaming_graph.h"

struct Node {
    int id;
    unsigned int vertex;
    int state;
    std::vector<Node*> children;
    Node* parent;

    Node(int child_id, unsigned int child_vertex, int child_state, Node * node) : id(child_id), vertex(child_vertex), state(child_state), parent(node) {};
};

struct Tree {
    unsigned int rootVertex;
    Node* rootNode;
    bool expired;

    bool operator==(const Tree& other) const {
        return rootVertex == other.rootVertex;
    }
};

struct TreeHash {
    size_t operator()(const Tree& p) const {
        return hash<unsigned int>()(p.rootVertex);
    }
};

// Custom hash function for std::pair<unsigned, unsigned>
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ (hash2 << 1); // Combine the two hash values
    }
};

class Forest {
public:
    std::unordered_map<unsigned int, Tree> trees; // Currently active trees, Key: root vertex, Value: Tree
    std::unordered_map<unsigned int, std::unordered_set<Tree, TreeHash>> vertex_tree_map; // Maps vertex to tree to which it belongs to
    // key: pair ts open, ts close, value: a pair of trees (first) and vertex_tree_map (second)
    std::unordered_map<std::pair<unsigned, unsigned>, std::pair<std::unordered_map<unsigned int, Tree>, std::unordered_map<unsigned int, std::unordered_set<Tree, TreeHash>>>, pair_hash> backup_map;

    ~Forest() {
        for (auto&[fst, snd] : trees) {
            deleteTreeRecursive(snd.rootNode, fst);
        }
    }

    // a vertex can be root of only one tree
    // proof: since we have only one initial state and the tree has an initial state as root
    // it exists only one pair vertex-state that can be root of a tree
    void addTree(int rootId, unsigned int rootVertex, int rootState) {
        if (trees.find(rootVertex) == trees.end()) {
            auto rootNode = new Node(rootId, rootVertex, rootState, nullptr);
            trees[rootVertex] = {rootVertex, rootNode, false};
            vertex_tree_map[rootVertex].insert(trees[rootVertex]);
        }
    }

    bool addChildToParent(unsigned int rootVertex, unsigned int parentVertex, int parentState, int childId, unsigned int childVertex, int childState) {
        if (Node* parent = findNodeInTree(rootVertex, parentVertex, parentState)) {
            parent->children.push_back(new Node(childId, childVertex, childState, parent));
            vertex_tree_map[childVertex].insert(trees[rootVertex]);
            return true;
        }
        return false;
    }

    void changeParent(unsigned int rootVertex, unsigned int childVertex, int childState, unsigned int newParentVertex, unsigned int newParentState) {
        Node* child = findNodeInTree(rootVertex, childVertex, childState);
        Node* newParent = findNodeInTree(rootVertex, newParentVertex, newParentState);

        if (child && newParent) {
            // Remove child from its current parent's children list
            if (child->parent) {
                auto& siblings = child->parent->children;
                siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
            } else {
                cout << "ERROR: Could not find new parent in tree" << endl;
                exit(1);
            }

            // Set the new parent
            child->parent = newParent;
            newParent->children.push_back(child);
        } else {
            if (!child) std::cout << "ERROR: Could not find child in tree" << std::endl;
            if (!newParent) std::cout << "ERROR: Could not find new parent in tree" << std::endl;
            exit(1);
        }
    }

    bool hasTree(unsigned int rootVertex) {
        return trees.find(rootVertex) != trees.end();
    }

    Node* findNodeInTree(unsigned int rootVertex, unsigned int vertex, int state, Node** parent = nullptr) {
        Node* root = trees[rootVertex].rootNode;
        return searchNodeParent(root, vertex, state, parent);
    }

    std::set<unsigned int> getKeySet(unsigned int vertex) {
        std::set<unsigned int> result;

        if (vertex_tree_map.count(vertex)) {
            for (auto tree: vertex_tree_map.find(vertex)->second) {
                if (searchNodeNoState(tree.rootNode, vertex)) result.emplace(tree.rootNode->vertex);
            }
        }
        return result;
    }

    /**
     * @param vertex id of the vertex in the AL
     * @param state state associated to the vertex node
     * @return the set of trees to which the node @param vertex @param state belongs to
     */
    std::vector<Tree> findTreesWithNode(unsigned int vertex, int state) {
        std::vector<Tree> result;
        // TODO Optimize by storing the state in the vertex-tree map
        if (vertex_tree_map.count(vertex)) {
            for (auto tree: vertex_tree_map.find(vertex)->second) {
                if (searchNode(tree.rootNode, vertex, state)) result.push_back(tree);
            }
        }
        return result;
    }

    void expire(const std::unordered_set<unsigned int> & vertexes) {
        // std::cout << "DEBUG: Deleting vertex ";
        // for (auto toprint: vertexes) std::cout << toprint << ", ";
        // cout << std::endl;

        // Map each vertex to the trees it belongs to
        // When we need to delete a vertex, we trace back to the trees and navigate them until we find the corresponding node
        // From the corresponding node, we delete the node and its entire subtree
        // During the traversal of the subtree, for each node
            // we access the map and remove the current tree from the vertex we are visiting
        // Remove the vertex from the map
        for (auto vertex : vertexes) {
            if (vertex_tree_map.find(vertex) == vertex_tree_map.end()) continue;
            auto treesSet = vertex_tree_map.at(vertex);
            for (auto [rootVertex, rootNode, expired] : treesSet) {
                if (Node* current = searchNodeNoState(rootNode, vertex)) {
                    if (current->parent) {
                        deleteSubTree(current, rootVertex);
                    } else { // current vertex is the root node
                        deleteTreeIterative(rootNode, rootVertex);
                        trees.erase(vertex);
                    }
                }
            }
            vertex_tree_map.erase(vertex);
        }
    }

    void deepCopyTreesAndVertexTreeMap(unsigned key1, unsigned key2) {
        std::unordered_map<unsigned int, Tree> trees_copy;
        std::unordered_map<unsigned int, std::unordered_set<Tree, TreeHash>> vertex_tree_map_copy;

        // Deep copy trees
        for (const auto& [rootVertex, tree] : trees) {
            Tree tree_copy = deepCopyTree(tree);
            trees_copy[rootVertex] = tree_copy;
        }

        // Deep copy vertex_tree_map
        for (const auto& [vertex, tree_set] : vertex_tree_map) {
            std::unordered_set<Tree, TreeHash> tree_set_copy;
            for (const auto& tree : tree_set) {
                tree_set_copy.insert(trees_copy[tree.rootVertex]);
            }
            vertex_tree_map_copy[vertex] = tree_set_copy;
        }

        // Store the copies in the backup_map
        backup_map[{key1, key2}] = {trees_copy, vertex_tree_map_copy};
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

    Tree deepCopyTree(const Tree& tree) {
        Tree tree_copy;
        tree_copy.rootVertex = tree.rootVertex;
        tree_copy.expired = tree.expired;
        tree_copy.rootNode = deepCopyNodeIterative(tree.rootNode);
        return tree_copy;
    }

    Node* deepCopyNode(Node* node, Node* parent) {
        if (!node) return nullptr;
        Node* node_copy = new Node(node->id, node->vertex, node->state, parent);
        for (Node* child : node->children) {
            node_copy->children.push_back(deepCopyNode(child, node_copy));
        }
        return node_copy;
    }

    Node* deepCopyNodeIterative(Node* root) {
        if (!root) return nullptr;

        std::unordered_map<Node*, Node*> node_map;
        std::queue<Node*> node_queue;
        node_queue.push(root);

        Node* root_copy = new Node(root->id, root->vertex, root->state, nullptr);
        node_map[root] = root_copy;

        while (!node_queue.empty()) {
            Node* current = node_queue.front();
            node_queue.pop();

            Node* current_copy = node_map[current];

            for (Node* child : current->children) {
                if (node_map.find(child) == node_map.end()) {
                    Node* child_copy = new Node(child->id, child->vertex, child->state, current_copy);
                    node_map[child] = child_copy;
                    node_queue.push(child);
                }
                current_copy->children.push_back(node_map[child]);
            }
        }

        return root_copy;
    }

    Node* searchNodeParent(Node* node, unsigned int vertex, int state, Node** parent = nullptr) {
        if (!node) return nullptr;
        if (node->vertex == vertex && node->state == state) return node;
        for (Node* child : node->children) {
            if (parent) *parent = node;
            Node* found = searchNodeParent(child, vertex, state, parent);
            if (found) return found;
        }
        return nullptr;
    }

    Node *searchNode(Node *node, unsigned int vertex, int state) {
        if (!node) return nullptr;
        if (node->vertex == vertex && node->state == state) return node;
        for (Node *child: node->children) {
            Node *found = searchNode(child, vertex, state);
            if (found) return found;
        }
        return nullptr;
    }

    Node *searchNodeNoState(Node *node, unsigned int vertex) {
        if (!node) return nullptr;
        if (node->vertex == vertex) return node;
        for (Node *child: node->children) {
            Node *found = searchNodeNoState(child, vertex);
            if (found) return found;
        }
        return nullptr;
    }

    void deleteSubTree(Node *node, unsigned int rootVertex) {
        if (!node) return;

        // remove the node from the parent's children list
        if (node->parent) {
            auto &siblings = node->parent->children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), node), siblings.end());
        } else {
            std::cout << "WARNING: Deleting from root node in a subtree deletion procedure" << std::endl;
        }

        deleteTreeIterative(node, rootVertex);
        // deleteTreeRecursive(node, rootVertex);
    }

    void deleteTreeIterative(Node* node, unsigned int rootVertex) {
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

    void deleteTreeRecursive(Node* node, unsigned int rootVertex) {
        if (!node) return;
        for (Node* child : node->children) {
            deleteTreeRecursive(child, rootVertex);
        }
        if (vertex_tree_map.find(node->vertex) != vertex_tree_map.end()) {
            auto it = vertex_tree_map[node->vertex].begin();
            while (it != vertex_tree_map[node->vertex].end()) {
                if (it->rootVertex == rootVertex) {
                    vertex_tree_map[node->vertex].erase(it);
                    break;
                }
                ++it;
            }
        }

        // if the vertex_tree_map at current->vertex is empty, remove the entry
        if (vertex_tree_map[node->vertex].empty()) {
            vertex_tree_map.erase(node->vertex);
        }
        delete node;
    }

};

#endif //RPQ_FOREST_H
