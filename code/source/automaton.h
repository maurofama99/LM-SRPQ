#pragma once
#include<iostream>
#include<fstream>
#include<map>
#include<unordered_map>
#include<vector>
#include<string>
using namespace std;

// this file defines the DFA struture 
struct automat_edge // structure for neighbor list in the DFA 
{
	int state; // state of the neighbor
	int label;	// each edge only has one label. If there are multiple labels between two state, like state 0 can reach state 1 when receiving label 1 or 2 or 3, there will be multiple edges with different labels between 0 and 1
	automat_edge* next;
	automat_edge(int s, int l)
	{
		state = s;
		label = l;
		next = NULL;
	}
};
class automaton
{
public:
	unordered_set<int> final_state; // set of the final states;
	unordered_map<int, automat_edge*> g; // the successor edge list of each state 
	unordered_map<int, automat_edge*> rg;	// the precessor edge list of eeach state;
	unordered_set<unsigned int> acceptable_labels; // record set of edge labels acceptable to the DFA, represented in number
	automaton(int fs = 0) { final_state.insert(fs); }
	~automaton()
	{
		unordered_map<int, automat_edge*>::iterator it;
		for (it = g.begin(); it != g.end(); it++)
		{
			automat_edge* tmp = it->second;
			automat_edge* parent = tmp;
			while (tmp)
			{
				parent = tmp;
				tmp = tmp->next;
				delete parent;
			}
		}
		g.clear();
		for (it = rg.begin(); it != rg.end(); it++)
		{
			automat_edge* tmp = it->second;
			automat_edge* parent = tmp;
			while (tmp)
			{
				parent = tmp;
				tmp = tmp->next;
				delete parent;
			}
		}
		rg.clear();
	}
	void set_final_state(int fs) // set state fs as a final state
	{
		final_state.insert(fs);
	}
	bool check_final_state(int fs) // check if fs is a final state
	{
		return (final_state.find(fs)!=final_state.end());
	}
	void insert_edge(int s, int d, int label) // insert an edge, indicating state s transfers to state d after receiving label
	{
		acceptable_labels.insert(label);
		unordered_map<int, automat_edge*>::iterator it = g.find(s);
		if (it != g.end())
		{
			automat_edge* tmp = it->second;
			bool find = false;
			while (tmp)
			{
				if (tmp->label == label && tmp->state == d) // if the edge already exits, we do nothing
				{
					find = true;
					break;
				}
				tmp = tmp->next;
			}
			if (!find)
			{
				automat_edge* tmp = new automat_edge(d, label); // else we add a new edge to the neighbor list of the source state
				tmp->next = it->second;
				it->second = tmp;
			}
		}
		else
		{
			automat_edge* tmp = new automat_edge(d, label); // if there is no neighbor list for the source list in the DFA, we add it;
			g[s] = tmp;
		}

		it = rg.find(d);  // add edge to the neighbor list of the destination node, similar to above.
		if (it != rg.end())
		{
			automat_edge* tmp = it->second;
			bool find = false;
			while (tmp)
			{
				if (tmp->label == label && tmp->state == s)
				{
					find = true;
					break;
				}
				tmp = tmp->next;
			}
			if (!find)
			{
				automat_edge* tmp = new automat_edge(s, label);
				tmp->next = it->second;
				it->second = tmp;
			}
		}
		else
		{
			automat_edge* tmp = new automat_edge(s, label);
			rg[d] = tmp;
		}
	}
	bool delete_edge(int s, int d, int label) // delete a given edge; 
	{
		unordered_map<int, automat_edge*>::iterator it = g.find(s);
		if (it != g.end())
		{
			automat_edge* tmp = it->second;
			automat_edge* parent = tmp;
			bool find = false;
			while (tmp)
			{
				if (tmp->label == label && tmp->state == d)
				{
					find = true;
					if (it->second == tmp)
					{
						it->second = tmp->next;
						delete tmp;
						if (it->second == NULL)
							g.erase(it);
					}
					else
					{
						parent->next = tmp->next;
						delete tmp;
					}
					break;
				}
				parent = tmp;
				tmp = tmp->next;
			}
			if (!find)
				return false;
		}
		else
			return false;

		it = rg.find(d);
		if (it != rg.end())
		{
			automat_edge* tmp = it->second;
			automat_edge* parent = tmp;
			bool find = false;
			while (tmp)
			{
				if (tmp->label == label && tmp->state == s)
				{
					find = true;
					if (it->second == tmp)
					{
						it->second = tmp->next;
						delete tmp;
						if (it->second == NULL)
							rg.erase(it);
					}
					else
					{
						parent->next = tmp->next;
						delete tmp;
					}
					break;
				}
				parent = tmp;
				tmp = tmp->next;
			}
			if (!find)
				return false;
		}
		else
			return false;

		return true;

	}
	void get_possible_state(int label, vector<pair<int, int>>& vec) // get all the state that can has out-edge with the given label, and their dst state
	{
		for (unordered_map<int, automat_edge*>::iterator iter = g.begin(); iter != g.end(); iter++)
		{
			automat_edge* tmp = iter->second;
			while (tmp)
			{
				if (tmp->label == label)
				{
					vec.push_back(make_pair(iter->first, tmp->state));
					break;
				}
				tmp = tmp->next;
			}
		}
	}
	int get_suc(int s, int label) // get the destination state of s when receiving label. For a DFA, there will be only one such state
	{
		unordered_map<int, automat_edge*>::iterator it = g.find(s);
		if (it != g.end())
		{
			automat_edge* tmp = it->second;
			while (tmp)
			{
				if (tmp->label == label)
					return tmp->state;
				tmp = tmp->next;
			}
		}
		return -1;
	}
	void get_prev(int d, int label, vector<int>& prevs) // get the source states that can reach state d through the given label. Note that such states can be more than one, for example. regular expression a+
		// is expressed as state 0 reach final state 1 through lable a, and state 1 can reach itself through lable a. Thus state 1 can be reached through label a from both state 0 and state 1.
	{
		unordered_map<int, automat_edge*>::iterator it = rg.find(d);
		if (it != rg.end())
		{
			automat_edge* tmp = it->second;
			while (tmp)
			{
				if (tmp->label == label)
					prevs.push_back(tmp->state);
				tmp = tmp->next;
			}
		}
		return;
	}
	void get_label(int s, int d, vector<int> labels) // get the labels that can lead state s to state d. there may be more than 1 such labels;
	{
		unordered_map<int, automat_edge*>::iterator it = g.find(s);
		if (it != g.end())
		{
			automat_edge* tmp = it->second;
			while (tmp)
			{
				if (tmp->state == d)
					labels.push_back(tmp->label);
				tmp = tmp->next;
			}
		}
		return;

	}
	void get_all_suc(int s, map<int, int>& sucs) // sucs stores map from an edge label L to the destination state of s receiving L. It stores all the successors of s in the automat 
	{
		unordered_map<int, automat_edge*>::iterator it = g.find(s);
		if (it != g.end())
		{
			automat_edge* tmp = it->second;
			while (tmp)
			{
				sucs[tmp->label] = tmp->state;
				tmp = tmp->next;
			}
		}
	}
	void get_all_prev(int d, map<int, vector<int> >& prevs)// gets all the precursors of state d in the automat. Note that the result is a map from int to vector because there can be multiple labels bewteen state s and d
	{
		unordered_map<int, automat_edge*>::iterator it = rg.find(d);
		if (it != rg.end())
		{
			automat_edge* tmp = it->second;
			while (tmp)
			{
				prevs[tmp->label].push_back(tmp->state);
				tmp = tmp->next;
			}
		}
	}
};