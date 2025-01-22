//
// Created by maurofama on 03/12/24.
//
#ifndef WINDOW_OPERATOR_H_INCLUDED
#define WINDOW_OPERATOR_H_INCLUDED

#include <cmath>

using namespace std;

enum WINDOW_TYPE {
    TRIANGLE,
    TRANSITIVE,
    LABEL,
    CENTRALITY
};

class window {
public:
    unsigned int opening_time;
    unsigned int closing_time{};
    unsigned int last_update;
    unsigned int last_timestamp = 0;
    unsigned int window_lambda;
    unsigned int size = 1;
    vector<edge_info> edges;

    explicit window(unsigned int opening_time, int lambda, int last_update)
        : opening_time(opening_time), window_lambda(lambda), last_update(last_update) {
    }


    void set_expiration_time(unsigned int time);

    void add_edge(edge_info edge) {
        if (edges.empty()) {
            opening_time = edge.timestamp;
        }
        edges.push_back(edge);
    };
};


class window_operator {
public:

    int reshaping = 0;
    int max_window_size = 0;

    int unique_vertexes = 0;
    int window_extended = 0;
    int window_opened = 0;

    std::unordered_map<int, window> windows;

    virtual unsigned int assignWindow(unsigned int s, unsigned int d, unsigned int l, unsigned int timestamp,
                                      streaming_graph *sg) = 0;

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
    unsigned int window_size = 100000;

    unsigned assignWindow(unsigned int s, unsigned int d, unsigned int l, unsigned int timestamp,
                          streaming_graph *sg) override {
        return timestamp + window_size;
    }
};

class window_transitivity : public window_operator {
public:
    unsigned assignWindow(unsigned int s, unsigned int d, unsigned int l, unsigned int timestamp,
                          streaming_graph *sg) override {
        printf("transitivity");
        return 0;
    }
};

class window_label : public window_operator {
public:
    unsigned assignWindow(unsigned int s, unsigned int d, unsigned int l, unsigned int timestamp,
                          streaming_graph *sg) override {
        printf("label");
        return 0;
    }
};

class window_centrality : public window_operator {
public:
    unsigned assignWindow(unsigned int s, unsigned int d, unsigned int l, unsigned int timestamp,
                          streaming_graph *sg) override {
        /// Windowing parameters
        float alpha = 2;
        unsigned lambda = 109000;
        float gamma = 8000;
        int step = 500;
        double beta = 0.0005;
        int t_start = 20000;

        unsigned int n;
        unsigned int t_open;
        unsigned int exp;

        int window_id;

        // Compute degrees of inserted vertexes and pass it to the window operator
        unsigned int degree_s = sg->get_total_degree(s);
        unsigned int degree_d = -sg->get_total_degree(d);

        float theta_s = pow(degree_s, alpha) / (pow(degree_s, alpha) + pow(degree_d, alpha));
        float theta_d = 1 - theta_s;

        if (theta_s > theta_d) {
            window_id = s;
        } else {
            window_id = d;
        }

        if (windows.find(window_id) == windows.end()) {
            windows.emplace(window_id, window(timestamp, lambda, t_start));
            unique_vertexes++;
        } else {
            window_extended++;
            windows.at(window_id).size++;

            n = windows.at(window_id).last_update + step;

            // se l'n-esimo elemento arriva con un ritardo superiore alla tangente, chiudo la finestra e ne apro una nuova
            // TODO Add interpolation instead of fixed point
            double fn0 = windows.at(window_id).window_lambda * std::exp(-gamma / windows.at(window_id).window_lambda);
            double slope = windows.at(window_id).window_lambda * (
                               gamma / (windows.at(window_id).window_lambda * windows.at(window_id).window_lambda)) *
                           std::exp(-gamma / windows.at(window_id).window_lambda);
            double admitted_delay = slope * (n - windows.at(window_id).window_lambda) + fn0;
            if (beta * admitted_delay < timestamp - windows.at(window_id).last_timestamp) {
                windows.at(window_id).last_update = t_start;
                windows.at(window_id).opening_time = timestamp;
                window_opened++;
                // cout << "scaled admitted delay: " << beta * admitted_delay << ", delay :" << timestamp - windows.at(window_id).last_timestamp << endl;
            }
        }

        n = windows.at(window_id).last_update + step;
        exp = windows.at(window_id).window_lambda * std::exp(-gamma / n);

        windows.at(window_id).last_timestamp = timestamp;
        windows.at(window_id).last_update = n;

        return timestamp + exp;
    }
};

#endif
