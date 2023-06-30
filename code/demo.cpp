#include<vector>
#include<string>
#include<map>
#include<time.h>
#include<iostream>
#include<stdlib.h>
#include<algorithm> 
#include<unordered_map>
#include<chrono>
#include<algorithm>
#include"./source/S-PATH.h"
#include"./source/LM-SRPQ.h"
//#include "Golden-compute.h"
using namespace std;

int main()
{
	ifstream fin("D://学习相关文档//论文研究//RegularPathQuery//code//SCC-merge//data//StackOverflow.txt");
	double gap = 1;
	//double gap = 3.775;
	unsigned int w = 40*gap;
	streaming_graph* sg = new streaming_graph(w);
	automat* aut = new automat;
	aut->set_final_state(0);
//	aut->set_final_state(1);
//	aut->insert_edge(0, 1, 2);
	aut->insert_edge(0, 0, 0);
	//aut->insert_edge(1, 1, 1);
//	aut->insert_edge(2, 2, 2); 
//	aut->insert_edge(0, 0, 1);
//	aut->insert_edge(0, 1, 2);
//	aut->insert_edge(1, 1, 2);
	RPQ_forest* f1 = new RPQ_forest(sg, aut);
	unsigned int s, d, t, l;
	int index = 1;
	ofstream fout1("match.txt");
	ofstream fout2("memory.txt");
	ofstream fout3("insertion-time.txt");
	ofstream fout4("expiration-time.txt");
	ofstream fout5("HP-match.txt");
	ofstream fout6("HP-memory.txt");
	ofstream fout7("HP-insertion-time.txt");
	ofstream fout8("HP-expiration-time.txt");
	ofstream fout9("HP-selection-time.txt");
	ofstream fout10("total_time.txt");
	clock_t start = clock();
	unsigned int slice = 0;
	unsigned int t0 = 0;
	unsigned int total_edge_number = 5 * w;// 63497050;
	vector<unsigned int> edge_processing_time;
	vector<unsigned int> expire_time;
	vector<unsigned int> hp_selection_time;
		while (fin >> s >> d >> l >> t)
		{
		//	break;
			auto insertion_start = std::chrono::steady_clock::now();
			f1->insert_edge(s, d, l, index++);
			auto insertion_finish = std::chrono::steady_clock::now();
			auto duration = std::chrono::duration_cast<chrono::microseconds>(insertion_finish - insertion_start);
			edge_processing_time.push_back(duration.count());
			if(index%(w/10)==0) 
			{
			auto expiration_start = std::chrono::steady_clock::now();
			f1->expire(index);
			auto expiration_finish = std::chrono::steady_clock::now();
			duration = std::chrono::duration_cast<chrono::microseconds>(expiration_finish - expiration_start);
			expire_time.push_back(duration.count());
			if (edge_processing_time.size() > total_edge_number / 100)
			{
				sort(edge_processing_time.begin(), edge_processing_time.end(), compare);
				edge_processing_time.erase(edge_processing_time.begin() + (total_edge_number / 100), edge_processing_time.end());
			}
			cout<<index<<" edges has been inserted"<<endl; 
			//	if(index%w==0)
		//	{
				f1->count(fout2);
		//	}
			if(index==w)
				break;
			}
		}
	//	f1->print_all("info.txt");
	clock_t finish = clock();
	//f->print_path(263, 0, 117, 0);
	cout << "insert finished " << endl;
		unordered_map<unsigned long long, unsigned int> standard_result;
	//	 golden_compute(aut, sg, standard_result);
		for(unordered_map<unsigned long long, unsigned int>::iterator iter = f1->result_pairs.begin();iter!=f1->result_pairs.end();iter++)
			standard_result[iter->first] = iter->second;
		cout << "time used " << (double)(finish - start) / CLOCKS_PER_SEC << endl;
		fout10 << " naive time used " << (double)(finish - start) / CLOCKS_PER_SEC << endl;
		sort(edge_processing_time.begin(), edge_processing_time.end(), compare);
		if (edge_processing_time.size() > (total_edge_number / 100))
			edge_processing_time.erase(edge_processing_time.begin() + (total_edge_number / 100), edge_processing_time.end());
	//	sort(expire_time.begin(), expire_time.end(), compare);
	//	for (int i = 0; i < edge_processing_time.size(); i++)
	//		fout3 << edge_processing_time[i] << endl;
	//	for (int i = 0; i < expire_time.size(); i++)
	//		fout4 << expire_time[i] << endl;

		fout2.close();
		fout3.close();
		fout4.close();

	delete f1;
	delete sg;
	fin.clear();
	fin.seekg(0, ios::beg);
	edge_processing_time.clear();
	expire_time.clear();

	sg = new streaming_graph(w);
	RPQ_forest_hp* f2 = new RPQ_forest_hp(sg, aut);
	f2->aut_scores[0] = 6;
//	f2->aut_scores[1] = 6;
//	f->aut_scores[2] = 6;

	index = 1;
	start = clock();
	slice = 0;
	t0 = 0;
	while (fin >> s >> d >> l >> t)
	{
		auto insertion_start = std::chrono::steady_clock::now();
		f2->insert_edge(s, d, l, index++);
		auto insertion_finish = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<chrono::microseconds>(insertion_finish - insertion_start);
		edge_processing_time.push_back(duration.count());


			if(index%(w/10)==0) {
		//	slice++;
			auto expiration_start = std::chrono::steady_clock::now();
			f2->expire(index);
			auto expiration_finish = std::chrono::steady_clock::now();
			duration = std::chrono::duration_cast<chrono::microseconds>(expiration_finish - expiration_start);
			expire_time.push_back(duration.count());
			if (edge_processing_time.size() > total_edge_number / 100)
			{
				sort(edge_processing_time.begin(), edge_processing_time.end(), compare);
				edge_processing_time.erase(edge_processing_time.begin() + (total_edge_number / 100), edge_processing_time.end());
			}
		//	cout << "expiration finished" << endl;
			auto selection_start = std::chrono::steady_clock::now();
			f2->dynamic_hp_select(0.2, 1.5, fout9);
		//	f2->dynamic_hp_select(0.2, 0.2,2);
			auto selection_finish = std::chrono::steady_clock::now();
			duration = std::chrono::duration_cast<chrono::microseconds>(selection_finish - selection_start);
			hp_selection_time.push_back(duration.count());
			//cout << time/3.775 << " time units has been inserted" << endl;
			cout<<index<<" edges has been inserted"<<endl; 
			cout << "current hot point number " << f2->hotpoints.size() << endl;
		//	if (slice>0&&slice%10 == 0)
		//	if(index%w==0)
		//	cout << index - f2->g->window_size << endl;
				f2->count(fout6);
		/*	if (slice>0&&slice % 100 == 0) {
				fout3 << "slice " << slice << endl;
				f->output_match(fout1);
				fout3 << endl;
			}
			if (slice == 50)
				break;*/
			if(index==w){
				sg->check_correctness();
				break;
			}
		}
	}
	//f2->print_all("HPinfo.txt");
	finish = clock();
	cout << "insert finished " << endl;
	cout << "time used " << (double)(finish - start) / CLOCKS_PER_SEC << endl;
	fout10<< " HP time used " << (double)(finish - start) / CLOCKS_PER_SEC << endl;
	sort(edge_processing_time.begin(), edge_processing_time.end(), compare);
	if (edge_processing_time.size() > (total_edge_number / 100))
		edge_processing_time.erase(edge_processing_time.begin() + (total_edge_number / 100), edge_processing_time.end());
	for (int i = 0; i < edge_processing_time.size(); i++)
		fout7 << edge_processing_time[i] << endl;
	for (int i = 0; i < expire_time.size(); i++)
		fout8 << expire_time[i] << endl;
	//for (int i = 0; i < hp_selection_time.size(); i++)
		//fout9 << hp_selection_time[i] << endl;

	    cout << "result pairs not in hp solution " << endl;
		int wrong_num = 0;
		for(unordered_map<unsigned long long, unsigned int>::iterator iter = standard_result.begin();iter!=standard_result.end();iter++)
		{
			if(f2->result_pairs.find(iter->first)==f2->result_pairs.end())
				cout<<(iter->first>>32)<<' '<<(iter->first&0xFFFFFFFF)<<endl;
			else if(f2->result_pairs[iter->first]!=iter->second)
				cout<<"wrong timestamp "<<(iter->first>>32)<<' '<<(iter->first&0xFFFFFFFF)<<' '<<f2->result_pairs[iter->first]<<' '<<iter->second<<endl;
		}
		cout<<wrong_num<<endl;
			cout<<"result pairs not in standard solution "<<endl;
		for(unordered_map<unsigned long long, unsigned int>::iterator iter = f2->result_pairs.begin();iter!=f2->result_pairs.end();iter++)
		{
			if(standard_result.find(iter->first)==standard_result.end())
				cout<<(iter->first>>32)<<' '<<(iter->first&0xFFFFFFFF)<<endl;
		}


}
