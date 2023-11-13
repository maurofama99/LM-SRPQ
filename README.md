# LM-SRPQ
Code and Technical Report of paper LM-SRPQ: Efficiently Answering Regular Path Query in Streaming Graphs.

## Data: 
**StackOverflow**: http://snap.stanford.edu/data/sx-stackoverflow.html  
**LDBC**: Erling O, Averbuch A, Larriba-Pey J, et al. The LDBC social network benchmark: Interactive workload[C]//Proceedings of the 2015 ACM SIGMOD International Conference on Management of Data. 2015: 619-630. The scale factor is set to 10.  
**Yago**: https://www.mpi-inf.mpg.de/departments/databases-and-information-systems/research/yago-naga/yago/  

Data should be coded and ordered according to timestamps, and each row is organized in formal "src_id, dst_id, label, timestamp". All the 4 variables should be in integer type. 
Timestamps should be in Unix time form. The specific labels we use for different queries in different datasets are listed in the file named lable.txt.  

## File:
**source/automaton.h**: code of DFA  
**source/StreamingGraph.h**: code of the streaming graph  
**source/forest_struct.h**：code for the basic structures used in S-PATH and LM-SRPQ.   
**source/S-PATH**: code for S-PATH.  
**source/LM-SRPQ**: code for LM-SRPQ.  
**source/LM-NT.h** variant of LM-SRPQ, where no TI maps exist.  
**source/LM-DF.h** variant of LM-SRPQ, where we use dependency trees rather than TI maps to aid dependency graph traversal.  
**source/LM-random.h** variant of LM-SRPQ, where we randomly select landmarks.  
**source/Brutal-Search.h**: code for the brutal search algorithm, where we only store the product graph, and traverse in the product graph to build new paths from scratch upon each tuple update.    
**demo.cpp**: demo for testing. The parameters of LM-SRPQ like the candidate set rate (0.2 by default) and benefit threshold (1.5 by default) can be set in this file.   

## Compile and Run:
**Compile**: g++ -O2 -std=c++11 -o demo demo.cpp    
**Run**: ./demo (algorithm type) (path of data) (sliding window length, with days as unit) (sliding interval, with hours as unit) (query type (1-10, please refer to the paper to see the query each number denotes)) (label code for each variable in the RPQ)  
Algorithm type:  
1 S-PATH  
2 LM-SRPQ  
3 LM-DF  
4 LM-NT  
5 LM-random  
6 Brutal search  
For example, ./demo 2 ./data.txt 20 24 2 0 1 means to test LM-SRPQ in query 2 (a?b*) with a=0, b=1, and window size = 20 days, sliding interval = 24 hours.  
**Output**: 3 files will be output:     
S-PATH-memory.txt: memory of S-PATH at each checkpoint (there is a checkpoint when the sliding window slides forward 1/10 of the window size), including other information like result pair number and tree node number.  
S-PATH-insertion-latency.txt: largest 1% insertion time of S-PATH, with microseconds as unit and sorted in descending order.  
S-PATH-speed.txt: total processing time (second) and throughput (edge per second) of S-PATH.  

When using different algorithms, the names of the output files will be different, but the content is similar.  

LM-memory.txt LM-insertion-latency.txt LM-time.txt: Similar output files as discussed above, but for LM-SRPQ.

DF-memory.txt DF-insertion-latency.txt DF-time.txt: Similar output files as discussed above, but for LM-DF.

NT-memory.txt NT-insertion-latency.txt NT-time.txt: Similar output files as discussed above, but for LM-NT.

random-memory.txt random-insertion-latency.txt random-time.txt: Similar output files as discussed above, but for LM-random.

brutal-memory.txt brutal-insertion-latency.txt brutal-time.txt: Similar output files as discussed above, but for brutal search.

Note that the run time can be more than 1 week for some queries, especially when the algorithm is LM-NT and brutal search.
