//
// Created by maurofama on 03/12/24.
//
#ifndef WINDOW_OPERATOR_H_INCLUDED
#define WINDOW_OPERATOR_H_INCLUDED

#include <memory>

using namespace std;

enum WINDOW_TYPE {
    TRIANGLE,
    TRANSITIVE,
    LABEL,
    CENTRALITY
};

class window_operator {

public:

    virtual int assignWindow(unsigned int s, unsigned int d, unsigned int l, unsigned int timestamp, streaming_graph *sg) = 0;
    virtual ~window_operator() = default;

    static string toString(WINDOW_TYPE type) {
        switch (type) {
            case TRIANGLE:
                return "TRIANGLE";
            case TRANSITIVE:
                return "TRANSITIVE";
            case LABEL:
                return "LABEL";
            case CENTRALITY:
                return "CENTRALITY";
            default:
                return "UNKNOWN";
        }
    }

};

class window_triangle : public window_operator {
public:
    int assignWindow(unsigned int s, unsigned int d, unsigned int l, unsigned int timestamp, streaming_graph *sg) override{
        printf("triangle");
        return 0;
    }
};

class window_transitivity : public window_operator {
public:
    int assignWindow(unsigned int s, unsigned int d, unsigned int l, unsigned int timestamp, streaming_graph *sg) override{
        printf("transitivity");
        return 0;
    }
};

class window_label : public window_operator {
public:
    int assignWindow(unsigned int s, unsigned int d, unsigned int l, unsigned int timestamp, streaming_graph *sg) override{
        printf("label");
        return 0;
    }
};

class window_centrality : public window_operator {
public:
    int assignWindow(unsigned int s, unsigned int d, unsigned int l, unsigned int timestamp, streaming_graph *sg) override{
        printf("centrality");
        return 0;
    }
};

#endif