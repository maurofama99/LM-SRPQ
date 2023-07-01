# LM-SRPQ
Code and Technical Report of paper LM-SRPQ: Efficiently Answering Regular Path Query in Streaming Graphs.

## Data: 
StackOverflow: http://snap.stanford.edu/data/sx-stackoverflow.html  
LDBC: Erling O, Averbuch A, Larriba-Pey J, et al. The LDBC social network benchmark: Interactive workload[C]//Proceedings of the 2015 ACM SIGMOD International Conference on Management of Data. 2015: 619-630. The scale factor is set to 10.  
Yago: https://www.mpi-inf.mpg.de/departments/databases-and-information-systems/research/yago-naga/yago/  

Data should be coded and ordered according to timestamps, and each row is organized in formal "src_id, dst_id, label, timestamp". All the 4 variables should be in integer type. 
Timestamps should be in Unix time form. The specific labels we use for different queries in different datasets are listed in the file named lable.txt.  

## File:
source/automaton.h: code of DFA  
source/StreamingGraph.h: code of the streaming graph  
source/forest_struct.h：code for the basic structures used in S-PATH and LM-SRPQ.   
source/S-PATH: code for S-PATH.  
source/LM-SRPQ: code for LM-SRPQ.  
demo.cpp: demo for testing. The parameters of LM-SRPQ like candidate set rate (0.2 by default) and benefit threshold (1.5 by default) can be set in this file.   

## Compile and Run:
Compile: g++ -O2 -std=c++11 -o demo demo.cpp  
Run ./demo (path of data) (sliding window length, with days as unit) (sliding interval, with hours as unit) (query type (1-10, please refer to the paper to see the query each number denotes)) (label code for each variable in the RPQ)  
For example, ./demo ./data.txt 20 24 2 0 1 means to test query 2 (a?b*) with a=0, b=1, and window size = 20 days, sliding interval = 24 hours.
5 files will be output:   
S-memory.txt: memory of S-PATH at each checkpoint, including other information like result pair number and tree node number.  
S-insertion-time.txt: largest 99% insertion time of S-PATH, with microseconds as unit and sorted in descending order.  
LM-memory.txt:  memory of LM-SRPQ at each checkpoint, including other information like result pair number and tree node number.  
LM-insertion-time.txt: largest 99% insertion time of LM-SRPQ, with microseconds as unit and sorted in descending order.  
time.txt: total run time of S-PATH and LM-SRPQ, with seconds as unit. It can be used in computation of throughput.  
Note that the run time can be more than 1 week for some queries.  
