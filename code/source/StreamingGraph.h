#pragma once
#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<map>
#include<unordered_map>
#include<unordered_set>
#include <cassert>
using namespace std;

// this streaming graph class suppose that there may be duplicate edges in the streaming graph. It means the same edge (s, d, label) may appear multiple times in the stream.

class sg_edge;

struct timed_edge // this is the structure to maintain the time sequence list. It stores the tuples in the stream with time order, each tuple is an edge; 
{
	timed_edge* next;
	timed_edge* prev; // the two pointers to maintain the list;
	sg_edge* edge_pt;	// information of the tuple, including src, dst, edge label and timestamp;
	explicit timed_edge(sg_edge* edge_pt_ = nullptr)
	{
		edge_pt = edge_pt_;
		next = nullptr;
		prev = nullptr;
	}
};

struct edge_info // the structure as query result, include all the information of an edge;
{
	unsigned int s, d;
	int label;
	unsigned int timestamp;
	edge_info(unsigned int src, unsigned int dst, unsigned int time, int label_)
	{
		s = src;
		d = dst;
		timestamp = time;
		label = label_;
	}
};

// TODO Add expiration timestamp
class sg_edge
{
public:
	unsigned int s, d;
	int label;
	unsigned int timestamp;
	timed_edge* time_pos;
	sg_edge* src_prev;
	sg_edge* src_next;	// cross list, maintaining the graph structure
	sg_edge* dst_next;
	sg_edge* dst_prev;
	sg_edge(unsigned int src, unsigned int dst, unsigned int label_, unsigned int time)
	{
		s = src;
		d = dst;
		timestamp = time;
		label = label_;
		src_next = nullptr;
		src_prev = nullptr;
		dst_next = nullptr;
		dst_prev = nullptr;
		time_pos = nullptr;
	}
	~sg_edge()
	= default;
};

struct neighbor_list  // this struct serves as the head pointed of the neighbor list of a vertex
{
	sg_edge* list_head;
	explicit neighbor_list(sg_edge* head = nullptr)
	{
		list_head = head;
	}
};


class streaming_graph
{
public:
	unordered_map<unsigned int, neighbor_list> g; // successor list
	unordered_map<unsigned int, neighbor_list> rg; // precursor list
	int window_size;  // length of the window, defined in time units.
	int edge_num; // number of edges in the window
	timed_edge* time_list_head; // head of the time sequence list;
	timed_edge* time_list_tail;// tial of the time sequence list

	explicit streaming_graph(int w) {
		edge_num = 0;
		window_size = w;
		time_list_head = nullptr;
		time_list_tail = nullptr;
	}
	~streaming_graph()
	{
		unordered_map<unsigned int, neighbor_list>::iterator it;
		for (it = g.begin(); it != g.end(); it++) // delete the edges
		{
			sg_edge* tmp = it->second.list_head;
			sg_edge* parent = tmp;
			while (tmp)
			{
				parent = tmp;
				tmp = tmp->src_next;
				delete parent;
			}
		}
		g.clear();
		rg.clear(); // each edge only has one copy, liked by cross lists. Thus we only need to scan the successor lists to delete all edges;
		timed_edge* tmp = time_list_head; // delete the time sequence list;
		while (tmp)
		{
			timed_edge* cur = tmp;
			tmp = tmp->next;
			delete cur;
		}
	}
	void add_timed_edge(timed_edge* cur) // add an edge to the time sequence list. As edges arrive in time order, it will be added to the tail.
	{
		cur->next = nullptr;
		cur->prev = time_list_tail;
		if (!time_list_head) // if the list is empty, then this is the head.
			time_list_head = cur;
		if (time_list_tail)
			time_list_tail->next = cur;
		time_list_tail = cur; // cur is the new tail
	}
	void delete_timed_edge(timed_edge* cur) // delete an edge from the time list;
	{
		if (time_list_head == cur)
			time_list_head = cur->next;
		if (time_list_tail == cur)
			time_list_tail = cur->prev;
		if (cur->prev)
			(cur->prev)->next = cur->next; // disconnet it with its precursor and successor
		if (cur->next)
			(cur->next)->prev = cur->prev;
	}

	bool insert_edge(int s, int d, int label, int timestamp) // insert an edge in the streaming graph, bool indicates if it is a new edge (not appear before)
	{
		auto* cur = new timed_edge; // create a new timed edge and insert it to the tail of the time sequence list, as in a streaming graph the inserted edge is always the latest
		add_timed_edge(cur);

		auto it = g.find(s);

		if (it != g.end())
		{
			sg_edge* tmp = it->second.list_head;
			while (tmp)
			{
				if (tmp->label == label && tmp->d == d) // If we find the edge in the cross list, this is not a new edge, and we only update its timestamp and pointer to the time sequence list;
				{
				    delete_timed_edge(tmp->time_pos);
				    tmp->time_pos = cur; 
				    cur->edge_pt = tmp;
					tmp->timestamp = timestamp;
					return false;
				}
				tmp = tmp->src_next;
			}
			edge_num++; // if not appears before
			tmp = new sg_edge(s, d, label, timestamp); // create a new sg_edge
			tmp->time_pos = cur;
			cur->edge_pt = tmp;
			tmp->src_next = it->second.list_head; // add it to the front of the successor list of the src node 
			if(it->second.list_head)
				it->second.list_head->src_prev = tmp;
			it->second.list_head = tmp;
			auto it2 = rg.find(d);
			if (it2 != rg.end())
			{
				tmp->dst_next = it2->second.list_head;  // add it to the front of the precursor list of the dst node
				if(it2->second.list_head)
					it2->second.list_head->dst_prev = tmp;
				it2->second.list_head = tmp;
			}
			else {
				rg[d] = neighbor_list(tmp);
			}
			return true;
		}
		else // else the source vertex does not show up in the graph, we need to add a neighbor list, and add the new edge to the neighbor list.
		{
			edge_num++;
			auto* tmp = new sg_edge(s, d, label,timestamp);
			tmp->time_pos = cur;
			cur->edge_pt = tmp;
			g[s] = neighbor_list(tmp);
			auto it2 = rg.find(d);
			if (it2 != rg.end())
			{
				tmp->dst_next = it2->second.list_head;
				if(it2->second.list_head)
					it2->second.list_head->dst_prev = tmp;
				it2->second.list_head = tmp;
			}
			else
				rg[d] = neighbor_list(tmp);
			return true;
		}
	}

	void expire_edge(sg_edge* edge_to_delete) // this function deletes an expired edge from the streaming graph.
	{
		edge_num--;
		if(edge_to_delete->src_prev)	// If this edge is not the head of the neighbor list, we split it from the neighbor list.
		{
			edge_to_delete->src_prev->src_next = edge_to_delete->src_next;
			if(edge_to_delete->src_next)
				edge_to_delete->src_next->src_prev = edge_to_delete->src_prev;
		}
		else  // otherwise we need to change the head of the nighbor list to the next edge
		{
			auto it = g.find(edge_to_delete->s);
			if(edge_to_delete->src_next){
				it->second.list_head = edge_to_delete->src_next;
				edge_to_delete->src_next->src_prev = nullptr;
			}
			else  // if there is no next edge, this neighbor list is empty, and we can delete it.
			g.erase(it);
		}
		
		if(edge_to_delete->dst_prev)  // update the neighbor list of the destination vertex, similar to the update in for the source vertex.
		{
			edge_to_delete->dst_prev->dst_next = edge_to_delete->dst_next;
			if(edge_to_delete->dst_next)
				edge_to_delete->dst_next->dst_prev = edge_to_delete->dst_prev;
		}
		else
		{
			auto it = rg.find(edge_to_delete->d);
			if(edge_to_delete->dst_next){
				it->second.list_head = edge_to_delete->dst_next;
				edge_to_delete->dst_next->dst_prev = nullptr;
			}
			else
				rg.erase(it);
		}
    }

	void expire(unsigned int timestamp, vector<edge_info >& deleted_edges) // this function is used to find all expired edges and remove them from the graph.
	{
		while (time_list_head)
		{
			sg_edge* cur = time_list_head->edge_pt;
			if (cur->timestamp + window_size >= timestamp) // The later edges are still in the sliding window, and we can stop the expiration.
				break;
			expire_edge(cur);
			deleted_edges.emplace_back(cur->s, cur->d, cur->timestamp, cur->label); // we record the information of expired edges. This information will be used to find expired tree nodes in S-PATH or LM-SRPQ.
			timed_edge* tmp = time_list_head;
			time_list_head = time_list_head->next;	// delete the time sequence list unit
			if (time_list_head)
				time_list_head->prev = nullptr;
			delete tmp;
			delete cur;
		}
		if (!time_list_head) // if the list is empty, the tail pointer should also be NULL;
			time_list_tail = nullptr;
	}
	void get_suc(unsigned int s, int label, vector<unsigned int>& sucs) // get the successors of s, connected by edges with given label 
	{
		auto it = g.find(s);
		if (it != g.end())
		{
			sg_edge* tmp = it->second.list_head;
			while (tmp)
			{
				if (tmp->label == label)
					sucs.push_back(tmp->d);
				tmp = tmp->src_next;
			}
		}
    }

	void get_prev(unsigned int d, int label, vector<unsigned int>& prevs) // get the precursors of d, connected by edges with given label 
	{
		auto it = rg.find(d);
		if (it != rg.end())
		{
			sg_edge* tmp = it->second.list_head;
			while (tmp)
			{
				if (tmp->label == label)
					prevs.push_back(tmp->s);
				tmp = tmp->dst_next;
			}
		}
    }

	void get_all_suc(unsigned int s, vector<pair<unsigned int, unsigned int> >& sucs) // get all the successors, each pair is successor node ID + edge label
	{
		auto it = g.find(s);
		if (it != g.end())
		{
			sg_edge* tmp = it->second.list_head;
			while (tmp)
			{
				sucs.emplace_back(tmp->d, tmp->label);
				tmp = tmp->src_next;
			}
		}
	}
	void get_all_prev(unsigned int d, vector<pair<unsigned int, unsigned int> >& prevs)// get all the precursors, each pair is precursor node ID + edge label
	{
		auto it = rg.find(d);
		if (it != rg.end())
		{
			sg_edge* tmp = it->second.list_head;
			while (tmp)
			{
				prevs.emplace_back(tmp->s, tmp->label);
				tmp = tmp->dst_next;
			}
		}
	}
	// the following functions are variant of the former 4 functions, except that timestamp information is also included. The reported result is stored with edge_info structure;
	void get_timed_suc(unsigned int s, int label, vector<edge_info>& sucs)
	{
		auto it = g.find(s);
		if (it != g.end())
		{
			sg_edge* tmp = it->second.list_head;
			while (tmp)
			{
				if (tmp->label == label)
					sucs.emplace_back(tmp->s, tmp->d, tmp->timestamp, tmp->label);
				tmp = tmp->src_next;
			}
		}
    }
	void get_timed_prev(unsigned int d, int label, vector<edge_info>& prevs)
	{
		auto it = rg.find(d);
		if (it != rg.end())
		{
			sg_edge* tmp = it->second.list_head;
			while (tmp)
			{
				if (tmp->label == label)
					prevs.emplace_back(tmp->s, tmp->d, tmp->timestamp, tmp->label);
				tmp = tmp->dst_next;
			}
		}
    }
	void get_timed_all_suc(unsigned int s, vector<edge_info >& sucs)
	{
		auto it = g.find(s);
		if (it != g.end())
		{
			sg_edge* tmp = it->second.list_head;
			while (tmp)
			{
				sucs.emplace_back(tmp->s, tmp->d, tmp->timestamp, tmp->label);
				tmp = tmp->src_next;
			}
		}
	}
	void get_timed_all_prev(unsigned int d, vector<edge_info >& prevs)
	{
		auto it = rg.find(d);
		if (it != rg.end())
		{
			sg_edge* tmp = it->second.list_head;
			while (tmp)
			{
				prevs.emplace_back(tmp->s, tmp->d, tmp->timestamp, tmp->label);
				tmp = tmp->dst_next;
			}
		}
	}

	void get_src_degree(unsigned int s, map<unsigned int, unsigned int>& degree_map) // count the degree of out edges of a vertex s.
	{
		auto it = g.find(s);
		if (it != g.end())
		{
			sg_edge* tmp = it->second.list_head;
			while (tmp)
			{
				if (degree_map.find(tmp->label) != degree_map.end())
					degree_map[tmp->label]++;
				else
					degree_map[tmp->label] = 1;
				tmp = tmp->src_next;
			}
		}
	}

	void get_dst_degree(unsigned int d, map<unsigned int, unsigned int>& degree_map)// count the degree of in edges of a vertex d.
	{
		auto it = rg.find(d);
		if (it != g.end())
		{
			sg_edge* tmp = it->second.list_head;
			while (tmp)
			{
				if (degree_map.find(tmp->label) != degree_map.end())
					degree_map[tmp->label]++;
				else
					degree_map[tmp->label] = 1;
				tmp = tmp->dst_next;
			}
		}
	}


	unsigned int compute_memory() // compute the memory of the streaming graph.
	{
		unsigned int total_memory = sizeof(unordered_map<unsigned long long, neighbor_list>)*2 + 4*2 + 8*2; // memory of the two unordered_map, two integer (window size and edge num), two pointer (head and tail pointer of the time sequence list)
		total_memory += g.size() * 24 + g.bucket_count() * 8; // each KV has 16 byte, plus a pointer of 8 byte. Each bucket has another pointer, points to the KV pair list.
		total_memory += rg.size() * 24 + rg.bucket_count() * 8;
		total_memory += edge_num * (56 + 24); // each edge has two structure, 56 byte for the sg_edge and 24 byte for the timed_edge;
		return total_memory;
	}

	unsigned int get_vertice_num() // count the number of vertices in the streaming graph
	{
		unordered_set<unsigned int> S;
		for (auto & it : g)
		{
			S.insert(it.first);
			sg_edge* tmp = it.second.list_head;
			while (tmp)
			{
				S.insert(tmp->d);
				tmp = tmp->src_next;
			}
		}
		return S.size();
	}

	unsigned int get_edge_num() // return the number of edges in the streaming graph.
	{
		return edge_num;
	}

};
