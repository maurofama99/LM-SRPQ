#pragma once
#include<iostream>
#include<fstream>
#include<map>
#include<unordered_map>
#include<unordered_set>
#include<string>
#include <queue>
#include<stack>
#include<stdlib.h> 
#include<algorithm>
#include "forest_struct.h"
#include "automaton.h"
#define merge_long_long(s, d) (((unsigned long long)s<<32)|d)
using namespace std;

// code for the S-PATH algorithm



class Brutal_Solver
{
public:
	streaming_graph* g; // pointer to the streaming graph .
	automaton* aut; // pointer to the DFA of the regular expression. 
	product_graph* pg;
	unordered_map<unsigned long long, unsigned int> result_pairs; // result set, maps from vertex paris to the largest timestamp of regular paths between them

	Brutal_Solver(streaming_graph* g_, automaton* automaton)
	{
		g = g_;
		aut = automaton;
		pg = new product_graph;
	}
	~Brutal_Solver()
	{
		delete pg;
		result_pairs.clear();
	}

	void update_result(unordered_map<unsigned long long, unsigned int>& updated_paths) // update the result set, first of a KV in the um is a vertex ID v, and second is a timestamp t, update timestamp of (root v) pair
	// to t
	{
		for (unordered_map<unsigned long long, unsigned int>::iterator it = updated_paths.begin(); it != updated_paths.end(); it++)
		{
			unsigned long long result_pair = it->first;
			unsigned int time = it->second;
			if (result_pairs.find(result_pair) != result_pairs.end())
				result_pairs[result_pair] = max(result_pairs[result_pair], time); // if the vertex pair exists, update its timestamp
			else
				result_pairs[result_pair] = time; // else add the vertex pair.
		}
	}

	void pg_bfs(unsigned int v, unsigned int state, unordered_map<unsigned int, unsigned int>& final_state_nodes)
	{
		unordered_map<unsigned long long, unsigned int> time_index;
		priority_queue<pair<unsigned long long, unsigned int>, vector<pair<unsigned long long, unsigned int> >, pair_compare> q;
		unsigned long long info = merge_long_long(v, state);
		q.push(make_pair(info, MAX_INT));
		time_index[merge_long_long(v, state)] = MAX_INT;
		while (!q.empty())
		{
			pair<unsigned long long, unsigned int> p = q.top();
			q.pop();
			unsigned long long info = p.first;
			unsigned int time = p.second;
			if (aut->check_final_state((info & 0xFFFFFFFF)))
				final_state_nodes[(info >> 32)] = max(time, final_state_nodes[(info >> 32)]);
			vector<pair<unsigned long long, unsigned int>> vec;
			pg->get_successor((info >> 32), (info & 0xFFFFFFFF), vec);
			for (int i = 0; i < vec.size(); i++)
			{
				unsigned int suc_time = min(time, vec[i].second);
				if (time_index.find(vec[i].first) != time_index.end() && time_index[vec[i].first] >= suc_time)
					continue;
				else
				{
					time_index[vec[i].first] = suc_time;
					q.push(make_pair(vec[i].first, suc_time));
				}
			}
		}
	}


	void pg_reverse_bfs(unsigned int v, unsigned int state, unordered_map<unsigned int, unsigned int>& initial_state_nodes)
	{
		unordered_map<unsigned long long, unsigned int> time_index;
		priority_queue<pair<unsigned long long, unsigned int>, vector<pair<unsigned long long, unsigned int> >, pair_compare> q;
		q.push(make_pair(merge_long_long(v, state), MAX_INT));
		time_index[merge_long_long(v, state)] = MAX_INT;
		while (!q.empty())
		{
			pair<unsigned long long, unsigned int> p = q.top();
			unsigned long long info = p.first;
			q.pop();
			unsigned int time = time_index[info];
			if ((info & 0xFFFFFFFF)==0)
				initial_state_nodes[(info >> 32)] = max(time, initial_state_nodes[(info >> 32)]);
			vector<pair<unsigned long long, unsigned int>> vec;
			pg->get_precursor((info >> 32), (info & 0xFFFFFFFF), vec);
			for (int i = 0; i < vec.size(); i++)
			{
				unsigned int pre_time = min(time, vec[i].second);
				if (time_index.find(vec[i].first) != time_index.end() && time_index[vec[i].first] >= pre_time)
					continue;
				else
				{
					time_index[vec[i].first] = pre_time;
					q.push(make_pair(vec[i].first, pre_time));
				}
			}
		}
	}


	void insert_edge(unsigned int s, unsigned int d, unsigned int label, unsigned int timestamp)
	{
		vector<pair<int, int>> vec;
		aut->get_possible_state(label, vec);
		if(vec.empty())
			return;
		g->insert_edge(s, d, label, timestamp);
		unordered_map<unsigned long long, unsigned int> updated_paths;
		for (int i = 0; i < vec.size(); i++)
		{
			pg->insert_edge(s, vec[i].first, d, vec[i].second, timestamp);
			unordered_map<unsigned int, unsigned int> initial;
			unordered_map<unsigned int, unsigned int> final;
			pg_bfs(d, vec[i].second, final);
			pg_reverse_bfs(s, vec[i].first, initial);
			for (unordered_map<unsigned int, unsigned int>::iterator iter = initial.begin(); iter != initial.end(); iter++)
			{
				for (unordered_map<unsigned int, unsigned int>::iterator iter2 = final.begin(); iter2 != final.end(); iter2++)
				{
					if (iter->first == iter2->first)
						continue;
					unsigned int time = min(iter->second, iter2->second);
					time = min(time, timestamp);
					unsigned long long pair = merge_long_long(iter->first, iter2->first);
					if (updated_paths.find(pair) != updated_paths.end())
						updated_paths[pair] = max(updated_paths[pair], time);
					else
						updated_paths[pair] = time;
				}
			}
			initial.clear();
			final.clear();
		}
		update_result(updated_paths);
	}
	void results_update(int time) // given the threshold of expiration, delete all the expired result pairs.
	{
		if (time <= 0)
			return;
		for (unordered_map<unsigned long long, unsigned int>::iterator iter = result_pairs.begin(); iter != result_pairs.end();)
		{
			if (iter->second < time) {
				iter = result_pairs.erase(iter);
			}
			else
				iter++;
		}
		shrink(result_pairs);
	}


	void expire(int current_time) // given current time, delete the expired tree nodes and results.
	{
		int expire_time = current_time - g->window_size; // compute the threshold of expiration.
		results_update(expire_time); // delete expired results
		vector<edge_info> deleted_edges;
		g->expire(current_time, deleted_edges); // delete expired graph edges.
		for (int i = 0; i < deleted_edges.size(); i++)
		{
			unsigned int src = deleted_edges[i].s;
			unsigned int dst = deleted_edges[i].d;
			unsigned int label = deleted_edges[i].label; // for each expired edge, find its dst node. All the expired nodes in the spanning forest must be in a subtree of such dst node.
			vector<pair<int, int>> vec;
			aut->get_possible_state(label, vec); // get possible states of the dst node.
			for (int j = 0; j < vec.size(); j++) {
				int dst_state = vec[j].second;
				int src_state = vec[j].first;
				pg->expire_edge(src, src_state, dst, dst_state, expire_time);
			}
		}

	}



	void output_match(ofstream& fout) // output the recorded result pairs, used to 
	{
		for (unordered_map<unsigned long long, unsigned int>::iterator iter = result_pairs.begin(); iter != result_pairs.end(); iter++)
			fout << (iter->first >> 32) << " " << (iter->first & 0xFFFFFFFF) << " " << iter->second << endl;
	}
	void count(ofstream& fout) // count the memory used in the algorithm, but exclude the memory of automaton and streaming graph, because they are essential for any algorithm.
	{
		unsigned int um_size = sizeof(unordered_map<unsigned int, unsigned int>); // size of statistics in a map, which does not change with the key-value type, usually is 56, but may change with the system.
		unsigned int m_size = sizeof(map<unsigned int, unsigned int>); // size of pointers and statistics in a map, which does not change with the key-value type, usually is 48, but may change with the system.
		cout << "result pair size: " << result_pairs.size() << ", memory: " << ((double)(result_pairs.size() * 24 + result_pairs.bucket_count() * 8 + um_size) / (1024 * 1024)) << endl;  // number of result vertex pairs, and the memory used to store these results.
		fout << "result pair size: " << result_pairs.size() << ", memory: " << ((double)(result_pairs.size() * 24 + result_pairs.bucket_count() * 8 + um_size) / (1024 * 1024)) << endl;
		
		unsigned int pg_size = 0;
		pg_size += um_size;
		pg_size += pg->g.size() * 40 + pg->g.bucket_count() * 8;
		pg_size += pg->edge_cnt * 56;
		cout << "product graph size " << pg->edge_cnt << ' ' << pg->g.size() << ", memory: " << ((double)pg_size) / (1024 * 1024) << endl;
		fout << "product graph size " << pg->edge_cnt << ' ' << pg->g.size() << ", memory: " << ((double)pg_size) / (1024 * 1024) << endl;
		
		cout<<endl;
		fout<<endl;

	}


};
