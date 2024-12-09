//
// Created by maurofama on 03/12/24.
//

#ifndef LM_SRPQ_WINDOW_FACTORY_H
#define LM_SRPQ_WINDOW_FACTORY_H

#include "window_operator.h"

class window_factory {
public:
    virtual window_operator* createWindowOperator() = 0;
    virtual ~window_factory() = default; // Virtual destructor for polymorphism
};

class triangle_factory : public window_factory {
public:
    window_operator* createWindowOperator() override {
        return new window_triangle();
    }
};

class transitivity_factory : public window_factory {
public:
    window_operator* createWindowOperator() override {
        return new window_transitivity();
    }
};

class label_factory : public window_factory {
public:
    window_operator* createWindowOperator() override {
        return new window_label();
    }
};

class centrality_factory : public window_factory {
public:
    window_operator* createWindowOperator() override {
        return new window_centrality();
    }
};

#endif //LM_SRPQ_WINDOW_FACTORY_H
