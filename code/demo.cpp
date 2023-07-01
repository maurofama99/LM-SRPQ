#include<vector>
#include<string>
#include<map>
#include<time.h>
#include<stdlib.h>
#include<algorithm>
#include<chrono>
#include<unordered_map>
#include "./source/S-PATH.h"
#include "./source/LM-SRPQ.h"
using namespace std;
bool compare(unsigned int a, unsigned int b)
{
return a>b;
}

int main(int argc, char* argv[])
{
	double candidate_rate = 0.2;
	double benefit_threshold = 1.5;
	string data_path = argv[1];
	ifstream fin(data_path.c_str());
	unsigned int days = atoi(argv[2]);
	unsigned int hour = atoi(argv[3]);
	unsigned int query_type = atoi(argv[4]);
	unsigned int scores[4];
	
	automaton* aut = new automaton;
	unsigned int state_num = 0;
	
	 if(query_type == 1) //Q1 a*
    {
    	state_num = 1;
        aut->set_final_state(0);
        aut->insert_edge(0, 0, atoi(argv[5]));
        scores[0] = 6;
    }
    else if (query_type == 2) // Q2 a?b*
    {
    	state_num = 2;
       	aut->set_final_state(1);  /// 0 is also a final state, namely empty string is acceptable, and (v, v) is a result pair, but I omit result like (v, v) in queries.
        aut->insert_edge(0, 1, atoi(argv[5]));
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(1, 1, atoi(argv[6]));
        scores[0] = 0; //score of s0 is set to 0 as it has no incoming edge
        scores[1] = 6;
    }
    else if (query_type == 3) // Q3 ab*
    {
    	state_num = 2;
        aut->set_final_state(1);
        aut->insert_edge(0, 1, atoi(argv[5]));
        aut->insert_edge(1, 1, atoi(argv[6]));
        scores[0] = 0;
        scores[1] = 6;
    }
    else if (query_type == 4)   /// Q4: abc
    {
    	state_num = 4;
    	aut->set_final_state(3);
        aut->insert_edge(0, 1, atoi(argv[5]));
        aut->insert_edge(1, 2, atoi(argv[6]));
        aut->insert_edge(2, 3, atoi(argv[7]));
        scores[0] = 0;
        scores[1] = 2;
        scores[2] = 1;
        scores[3] = 0;
    }
    else if (query_type == 5) // Q5 abc*
    {
        state_num = 3; 
		aut->set_final_state(2);
        aut->insert_edge(0, 1, atoi(argv[5]));
        aut->insert_edge(1, 2, atoi(argv[6]));
        aut->insert_edge(2, 2, atoi(argv[7]));
        scores[0] = 0;
        scores[1] = 7;
        scores[2] = 6;
    }
    else if (query_type == 6) // Q6 ab*c
    {
    	state_num = 3;
        aut->set_final_state(2);
        aut->insert_edge(0, 1, atoi(argv[5]));
        aut->insert_edge(1, 1, atoi(argv[6]));
        aut->insert_edge(1, 2, atoi(argv[7]));
        scores[0] = 0;
        scores[1] = 7;
        scores[2] = 0;
    }
    else if (query_type == 7) // Q7 (a1+a2+a3)b*
    {
    	state_num = 2;
        aut->set_final_state(1);
        aut->insert_edge(0, 1,  atoi(argv[5]));
        aut->insert_edge(0, 1,  atoi(argv[6]));
        aut->insert_edge(0, 1,  atoi(argv[7]));
        aut->insert_edge(1, 1,  atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 6;
    }
    else if (query_type ==8)  // Q7 a*b*
    {
    	state_num = 2;
        aut->set_final_state(0);
        aut->set_final_state(1);
        aut->insert_edge(0, 0,  atoi(argv[5]));
        aut->insert_edge(0, 1,  atoi(argv[6]));
        aut->insert_edge(1, 1,  atoi(argv[6]));
        scores[0] = 13;
        scores[1] = 6;
    }
    else if(query_type == 9)  // Q9 ab*c*
    {
    	state_num = 3;
        aut->set_final_state(1);
        aut->set_final_state(2);
        aut->insert_edge(0, 1,  atoi(argv[5]));
        aut->insert_edge(1, 1,  atoi(argv[6]));
        aut->insert_edge(1, 2,  atoi(argv[7]));
        aut->insert_edge(2, 2,  atoi(argv[7]));
        scores[0] = 0;
        scores[1] = 13;  /// 2 loops, 1 edge 1->2, thus 2*6+1 = 13
        scores[2] = 6;
    }
    else if(query_type == 10) /// Q10 (a1+a2+a3)*
    {
    	state_num = 1;
        aut->set_final_state(0);
        aut->insert_edge(0, 0,  atoi(argv[5]));
        aut->insert_edge(0, 0,  atoi(argv[6]));
        aut->insert_edge(0, 0,  atoi(argv[7]));
        scores[0] = 6;
    }
    else
    {
    	cout<<"wrong query type"<<endl;
    	return 0;
	}
	
	unsigned int w = 3600*24*days;
	streaming_graph* sg = new streaming_graph(w);
	RPQ_forest* f1 = new RPQ_forest(sg, aut);
	unsigned int s, d, l;
	unsigned long long t;
	string prefix = "./"; // path of the out put files
	string postfix = ".txt";
	ofstream fout1((prefix+ "S-memory"+postfix).c_str()); // memory of S-PATH, memory mearsued at each checkpoint will be output, including 
	ofstream fout2((prefix+"S-insertion-time"+postfix).c_str());
	ofstream fout3((prefix+"LM-memory"+postfix.c_str())); // memory of LM-SRPQ, memory mearsued at each checkpoint will be output, and 
	ofstream fout4((prefix+"LM-insertion"+postfix).c_str());
	ofstream fout5((prefix+"time"+postfix).c_str()); //time used by two algorithms
	clock_t start = clock();
	unsigned int slice = 0;
	unsigned int t0 = 0;
	vector<unsigned int> insertion_time;
	unsigned int edge_number = 0;
	while (fin >> s >> d >> l >> t)
	{
		edge_number++;
		if(t0==0)
			t0 = t;
		unsigned int time = t-t0+1;
		auto insertion_start = std::chrono::steady_clock::now();
		f1->insert_edge(s, d, l, time);
		auto insertion_end = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<chrono::microseconds>(insertion_end - insertion_start);
		insertion_time.push_back(duration.count());
		
		if(time/(3600*hour)>slice)
		{
			slice++;
			f1->expire(time);
			cout << slice << " slices have been inserted" << endl;
			if (slice>0&&slice%(days*24/hour) == 0)
				f1->count(fout1); // a checkpoint
		}
	}
	clock_t finish = clock();
	cout << "insert finished " << endl;
	cout << "time used " << (double)(finish - start) / CLOCKS_PER_SEC << endl;
	fout5<< " S-PATH time used " << (double)(finish - start) / CLOCKS_PER_SEC << endl;
	sort(insertion_time.begin(), insertion_time.end(), compare);
	if(insertion_time.size()>(edge_number/100))
		insertion_time.erase(insertion_time.begin()+(edge_number/100), insertion_time.end());
	for(int i=0;i<insertion_time.size();i++)
		fout2<<insertion_time[i]<<endl;
	insertion_time.clear();
	fout1.close();
	fout2.close();

	delete f1;
	delete sg;
	fin.clear();
	fin.seekg(0, ios::beg);

	sg = new streaming_graph(w);
	LM_forest* f2 = new LM_forest(sg, aut);
	for(int i=0;i<state_num;i++)
			f2->aut_scores[i] = scores[i];
	start = clock();
	slice = 0;
	t0 = 0;
	edge_number = 0;
	while (fin >> s >> d >> l >> t)
	{
		edge_number++;
		if(t0==0)
			t0 = t;
		unsigned int time = t-t0+1;
		auto insertion_start = std::chrono::steady_clock::now();
		f2->insert_edge(s, d, l, time);
		auto insertion_end = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<chrono::microseconds>(insertion_end - insertion_start);
		insertion_time.push_back(duration.count());
		if (time / (3600*hour) > slice) {
			slice++;
			f2->expire(time);
			f2->dynamic_lm_select(candidate_rate, benefit_threshold);

			cout << slice << " slices have been inserted" << endl;
			if (slice>0&&slice%(days*24/hour) == 0)
				f2->count(fout3);
		}
	}
	finish = clock();
	cout << "insert finished " << endl;
	cout << "time used " << (double)(finish - start) / CLOCKS_PER_SEC << endl;
	fout5<< " LM-SRPQ time used " << (double)(finish - start) / CLOCKS_PER_SEC << endl;
	
	sort(insertion_time.begin(), insertion_time.end(), compare);
	if(insertion_time.size()>(edge_number/100))
		insertion_time.erase(insertion_time.begin()+(edge_number/100), insertion_time.end());
	for(int i=0;i<insertion_time.size();i++)
		fout4<<insertion_time[i]<<endl;
		
	insertion_time.clear();
	fout3.close();
	fout4.close();
	delete sg;
	delete f2;


}
