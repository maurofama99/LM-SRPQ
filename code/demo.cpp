#include<vector>
#include<string>
#include<ctime>
#include<cstdlib>
#include<algorithm>
#include<chrono>
#include "./source/S-PATH.h"
#include "./source/LM-SRPQ.h"
#include "./source/LM-DF.h"
#include "./source/LM-NT.h"
#include "./source/LM-random.h"
#include "./source/Brutal-Search.h"
#include "./source/window_factory.h"

using namespace std;
bool compare(unsigned int a, unsigned int b)
{
return a>b;
}

int main(int argc, char* argv[])
{
	double candidate_rate = 0.2;
	double benefit_threshold = 1.5;
	unsigned int algorithm = atoi(argv[1]); 
	string data_path = argv[2];
	ifstream fin(data_path.c_str());
	unsigned int days = atoi(argv[3]);
	unsigned int hour = atoi(argv[4]);
	unsigned int query_type = atoi(argv[5]);
    unsigned int window_type = 3;
	unsigned int scores[4];

	if (fin.fail())
	{
		cerr << "Error opening file" << endl;
		exit(1);
	}

	auto* aut = new automaton;

    // Factory method pattern to create window operator
    window_factory* window_factory;
    window_operator* windowOperator;
    switch (window_type) {
        case 0: // TRIANGLE
            window_factory = new class triangle_factory();
            windowOperator = window_factory->createWindowOperator();
            break;
        case 1: // TRANSITIVITY
            window_factory = new class transitivity_factory();
            windowOperator = window_factory->createWindowOperator();
            break;
        case 2: // LABEL
            window_factory = new class label_factory();
            windowOperator = window_factory->createWindowOperator();
            break;
        case 3: // CENTRALITY
            window_factory = new class centrality_factory();
            windowOperator = window_factory->createWindowOperator();
            break;
        default:
            // throw std::invalid_argument(&"Unknown window operator type: " [ window_type]);
    }

	unsigned int state_num = 0;
	
	 if(query_type == 1) //Q1 a*
    {
    	state_num = 1;
        aut->set_final_state(0);
        aut->insert_edge(0, 0, atoi(argv[6]));
        scores[0] = 6;
    }
    else if (query_type == 2) // Q2 a?b*
    {
    	state_num = 2;
       	aut->set_final_state(1);  /// 0 is also a final state, namely empty string is acceptable, and (v, v) is a result pair, but I omit result like (v, v) in queries.
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(0, 1, atoi(argv[7]));
        aut->insert_edge(1, 1, atoi(argv[7]));
        scores[0] = 0; //score of s0 is set to 0 as it has no incoming edge
        scores[1] = 6;
    }
    else if (query_type == 3) // Q3 ab*
    {
    	state_num = 2;
        aut->set_final_state(1);
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(1, 1, atoi(argv[7]));
        scores[0] = 0;
        scores[1] = 6;
    }
    else if (query_type == 4)   /// Q4: abc
    {
    	state_num = 4;
    	aut->set_final_state(3);
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(1, 2, atoi(argv[7]));
        aut->insert_edge(2, 3, atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 2;
        scores[2] = 1;
        scores[3] = 0;
    }
    else if (query_type == 5) // Q5 abc*
    {
        state_num = 3; 
		aut->set_final_state(2);
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(1, 2, atoi(argv[7]));
        aut->insert_edge(2, 2, atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 7;
        scores[2] = 6;
    }
    else if (query_type == 6) // Q6 ab*c
    {
    	state_num = 3;
        aut->set_final_state(2);
        aut->insert_edge(0, 1, atoi(argv[6]));
        aut->insert_edge(1, 1, atoi(argv[7]));
        aut->insert_edge(1, 2, atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 7;
        scores[2] = 0;
    }
    else if (query_type == 7) // Q7 (a1+a2+a3)b*
    {
    	state_num = 2;
        aut->set_final_state(1);
        aut->insert_edge(0, 1,  atoi(argv[6]));
        aut->insert_edge(0, 1,  atoi(argv[7]));
        aut->insert_edge(0, 1,  atoi(argv[8]));
        aut->insert_edge(1, 1,  atoi(argv[9]));
        scores[0] = 0;
        scores[1] = 6;
    }
    else if (query_type ==8)  // Q7 a*b*
    {
    	state_num = 2;
        aut->set_final_state(0);
        aut->set_final_state(1);
        aut->insert_edge(0, 0,  atoi(argv[6]));
        aut->insert_edge(0, 1,  atoi(argv[7]));
        aut->insert_edge(1, 1,  atoi(argv[7]));
        scores[0] = 13;
        scores[1] = 6;
    }
    else if(query_type == 9)  // Q9 ab*c*
    {
    	state_num = 3;
        aut->set_final_state(1);
        aut->set_final_state(2);
        aut->insert_edge(0, 1,  atoi(argv[6]));
        aut->insert_edge(1, 1,  atoi(argv[7]));
        aut->insert_edge(1, 2,  atoi(argv[8]));
        aut->insert_edge(2, 2,  atoi(argv[8]));
        scores[0] = 0;
        scores[1] = 13;  /// 2 loops, 1 edge 1->2, thus 2*6+1 = 13
        scores[2] = 6;
    }
    else if(query_type == 10) /// Q10 (a1+a2+a3)*
    {
    	state_num = 1;
        aut->set_final_state(0);
        aut->insert_edge(0, 0,  atoi(argv[6]));
        aut->insert_edge(0, 0,  atoi(argv[7]));
        aut->insert_edge(0, 0,  atoi(argv[8]));
        scores[0] = 6;
    }
    else
    {
    	cout<<"wrong query type"<<endl;
    	return 0;
	}
	
	unsigned int w = 3600*24*days;
	auto* sg = new streaming_graph(w);
	unsigned long s, d, l;
	unsigned long long t;
	string prefix = "./"; // path of the output files
	string postfix = ".txt";
	unsigned int slice = 0;
	unsigned int t0 = 0;
	vector<unsigned int> insertion_time;
	unsigned int edge_number = 0;
	
	if(algorithm==1){
	auto* f1 = new RPQ_forest(sg, aut);
	ofstream fout1((prefix+ "S-PATH-memory"+postfix).c_str()); // memory of S-PATH, memory mearsued at each checkpoint will be output, including 
	ofstream fout2((prefix+"S-PATH-insertion-latency"+postfix).c_str());
	ofstream fout3((prefix+"S-PATH-speed"+postfix).c_str()); //time used by two algorithms
	clock_t start = clock();
	while (fin >> s >> d >> l >> t)
	{
		edge_number++;
		if(t0==0)
			t0 = t;
		unsigned int time = t-t0+1;

		auto insertion_start = std::chrono::steady_clock::now();
		f1->insert_edge(s, d, l, time);  // TODO modify edge structure to save expiration time
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
	unsigned int time_used = (double)(finish - start) / CLOCKS_PER_SEC;
	cout << "S-PATH time used " << time_used << endl;
	cout<< "S-PATH speed "<< double(edge_number)/time_used<<endl;
	fout3<< " S-PATH time used " << time_used << endl;
	fout3<< "S-PATH speed "<< double(edge_number)/time_used<<endl;
	sort(insertion_time.begin(), insertion_time.end(), compare);
	if(insertion_time.size()>(edge_number/100))
		insertion_time.erase(insertion_time.begin()+(edge_number/100), insertion_time.end());
	for(int i=0;i<insertion_time.size();i++)
		fout2<<insertion_time[i]<<endl;
		
	insertion_time.clear();
	fout1.close();
	fout2.close();
	fout3.close();
	delete f1;
}

if(algorithm==2){
	ofstream fout1((prefix+ "LM-memory_" + std::to_string(days) + "_" + std::to_string(hour) + "_" + to_string(query_type) + postfix).c_str()); // memory of S-PATH, memory measured at each checkpoint will be output, including
	ofstream fout2((prefix+"LM-insertion-latency"+postfix).c_str());
	ofstream fout3((prefix+"LM-speed"+postfix).c_str()); //time used by two algorithms
	ofstream fout4((prefix+"LM-output"+postfix).c_str());
	
	auto* f2 = new LM_forest(sg, aut);
	for(int i=0;i<state_num;i++)
			f2->aut_scores[i] = scores[i];
	clock_t	start = clock();
	while (fin >> s >> d >> l >> t)
	{
		edge_number++;
		if(t0==0)
			t0 = t;
		unsigned int time = t-t0+1;

		// TODO: Add the edge only after assigning window
		// Decouple insertion in the snapshot graph (g->insert_edge) from the query operator
		f2->update_snapshot_graph(l, t, s, d);

		// Compute degrees of inserted vertexes and pass it to the window operator
		unsigned int degree_s = f2->g->get_total_degree(s);
		unsigned int degree_d = f2->g->get_total_degree(d);

		// Apply window operator and return expiration time
		int expiration_time = windowOperator->assignWindow(s, d, l, time, f2->g);

		// TODO 4: extend element addition in the forest with custom expiration time


		auto insertion_start = std::chrono::steady_clock::now();
		f2->insert_edge(s, d, l, time);
		auto insertion_end = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<chrono::microseconds>(insertion_end - insertion_start);
		insertion_time.push_back(duration.count());
		// f2->output_match(fout4);

		// TODO: customize expired nodes deletions and results output
		if (time / (3600*hour) > slice) {
			slice++;
			f2->expire(time);
			f2->dynamic_lm_select(candidate_rate, benefit_threshold);

			cout << slice << " slices have been inserted" << endl;
			if (slice>0&&slice%(days*24/hour) == 0)
				f2->count(fout1);
		}
	}
	clock_t	finish = clock();
	cout << "insert finished " << endl;
	unsigned int time_used = (double)(finish - start) / CLOCKS_PER_SEC;
	cout << "LM-SRPQ time used " << time_used << endl;
	cout << "LM-SRPQ speed " << (double)edge_number / time_used << endl;
	fout3<< " LM-SRPQ time used " << time_used << endl;
	fout3 << "LM-SRPQ speed " << (double)edge_number / time_used << endl;
	
	sort(insertion_time.begin(), insertion_time.end(), compare);
	if(insertion_time.size()>(edge_number/100))
		insertion_time.erase(insertion_time.begin()+(edge_number/100), insertion_time.end());
	for(int i=0;i<insertion_time.size();i++)
		fout2<<insertion_time[i]<<endl;
		
	insertion_time.clear();
	fout1.close();
	fout2.close();
	fout3.close();
	delete f2;
}

if(algorithm==3){	
	ofstream fout1((prefix+ "DF-memory"+postfix).c_str()); // memory of S-PATH, memory measured at each checkpoint will be output, including
	ofstream fout2((prefix+"DF-insertion-latency"+postfix).c_str());
	ofstream fout3((prefix+"DF-speed"+postfix).c_str()); //time used by two algorithms
	
	LM_DF* f2 = new LM_DF(sg, aut);
	for(int i=0;i<state_num;i++)
			f2->aut_scores[i] = scores[i];
	clock_t	start = clock();
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
				f2->count(fout1);
		}
	}
	clock_t	finish = clock();
	cout << "insert finished " << endl;
	unsigned int time_used = (double)(finish - start) / CLOCKS_PER_SEC;
	cout << "LM-DF time used " << time_used << endl;
	cout << "LM-DF speed " << (double)edge_number / time_used << endl;
	fout3<< " LM-DF time used " << time_used << endl;
	fout3 << "LM-DF speed " << (double)edge_number / time_used << endl;
	
	sort(insertion_time.begin(), insertion_time.end(), compare);
	if(insertion_time.size()>(edge_number/100))
		insertion_time.erase(insertion_time.begin()+(edge_number/100), insertion_time.end());
	for(int i=0;i<insertion_time.size();i++)
		fout2<<insertion_time[i]<<endl;
		
	insertion_time.clear();
	fout1.close();
	fout2.close();
	fout3.close();
	delete f2;
}

if(algorithm==4){	
	ofstream fout1((prefix+ "NT-memory"+postfix).c_str()); // memory of S-PATH, memory mearsued at each checkpoint will be output, including 
	ofstream fout2((prefix+"NT-insertion-latency"+postfix).c_str());
	ofstream fout3((prefix+"NT-speed"+postfix).c_str()); //time used by two algorithms
	
	LM_NT* f2 = new LM_NT(sg, aut);
	for(int i=0;i<state_num;i++)
			f2->aut_scores[i] = scores[i];
	clock_t	start = clock();
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
				f2->count(fout1);
		}
	}
	clock_t	finish = clock();
	cout << "insert finished " << endl;
	unsigned int time_used = (double)(finish - start) / CLOCKS_PER_SEC;
	cout << "LM-NT time used " << time_used << endl;
	cout << "LM-NT speed " << (double)edge_number / time_used << endl;
	fout3<< " LM-NT time used " << time_used << endl;
	fout3 << "LM-NT speed " << (double)edge_number / time_used << endl;
	
	sort(insertion_time.begin(), insertion_time.end(), compare);
	if(insertion_time.size()>(edge_number/100))
		insertion_time.erase(insertion_time.begin()+(edge_number/100), insertion_time.end());
	for(int i=0;i<insertion_time.size();i++)
		fout2<<insertion_time[i]<<endl;
		
	insertion_time.clear();
	fout1.close();
	fout2.close();
	fout3.close();
	delete f2;
}

if(algorithm==5){	
	ofstream fout1((prefix+ "random-memory"+postfix).c_str()); // memory of S-PATH, memory mearsued at each checkpoint will be output, including 
	ofstream fout2((prefix+"random-insertion-latency"+postfix).c_str());
	ofstream fout3((prefix+"random-speed"+postfix).c_str()); //time used by two algorithms
	
	LM_random* f2 = new LM_random(sg, aut);
	for(int i=0;i<state_num;i++)
			f2->aut_scores[i] = scores[i];
	clock_t	start = clock();
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
				f2->count(fout1);
		}
	}
	clock_t	finish = clock();
	cout << "insert finished " << endl;
	unsigned int time_used = (double)(finish - start) / CLOCKS_PER_SEC;
	cout << "LM-random time used " << time_used << endl;
	cout << "LM-random speed " << (double)edge_number / time_used << endl;
	fout3<< " LM-random time used " << time_used << endl;
	fout3 << "LM-random speed " << (double)edge_number / time_used << endl;
	
	sort(insertion_time.begin(), insertion_time.end(), compare);
	if(insertion_time.size()>(edge_number/100))
		insertion_time.erase(insertion_time.begin()+(edge_number/100), insertion_time.end());
	for(int i=0;i<insertion_time.size();i++)
		fout2<<insertion_time[i]<<endl;
		
	insertion_time.clear();
	fout1.close();
	fout2.close();
	fout3.close();
	delete f2;
}


if(algorithm==6){	
	ofstream fout1((prefix+ "brutal-memory"+postfix).c_str()); // memory of S-PATH, memory mearsued at each checkpoint will be output, including 
	ofstream fout2((prefix+"brutal-insertion-latency"+postfix).c_str());
	ofstream fout3((prefix+"brutal-speed"+postfix).c_str()); //time used by two algorithms
	
	Brutal_Solver* f2 = new Brutal_Solver(sg, aut);
	clock_t	start = clock();
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

			cout << slice << " slices have been inserted" << endl;
			if (slice>0&&slice%(days*24/hour) == 0)
				f2->count(fout1);
		}
	}
	clock_t	finish = clock();
	cout << "insert finished " << endl;
	unsigned int time_used = (double)(finish - start) / CLOCKS_PER_SEC;
	cout << "Brutal Search time used " << time_used << endl;
	cout << "Brutal Search speed " << (double)edge_number / time_used << endl;
	fout3<< " Brutal Search time used " << time_used << endl;
	fout3 << "Brutal Search speed " << (double)edge_number / time_used << endl;
	
	sort(insertion_time.begin(), insertion_time.end(), compare);
	if(insertion_time.size()>(edge_number/100))
		insertion_time.erase(insertion_time.begin()+(edge_number/100), insertion_time.end());
	for(int i=0;i<insertion_time.size();i++)
		fout2<<insertion_time[i]<<endl;
		
	insertion_time.clear();
	fout1.close();
	fout2.close();
	fout3.close();
	delete f2;
}


	delete sg;
	return 0;
}

