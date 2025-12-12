#include <iostream>
#include <ctime>
#include "sqlite3.h"
#include "dbUtil.h"
#include "graph.h"
#include <omp.h>
#include <algorithm>

int main(int argc, char* argv[]){
    if (argc < 3) { 
        std::cerr << "Usage: " << argv[0] << " <num_threads> <fractional_problem_size (0,1])>" << std::endl;
        return 1;
    }

    int threads = std::stoi(argv[1]); 
    double fractional_problem_size = std::stod(argv[2]);

    time_t start_t;
    time(&start_t);

    sqlite3 *db;
    int rc = sqlite3_open("../wikipedia.sqlite", &db);

    if (rc)
    {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    else
    {
        std::cout << "Opened database successfully" << std::endl;
    }

    dbUtil databaseUtil(db);

    std::unordered_map<long, std::vector<long>>* links_inward = databaseUtil.loadLinks_grouped_Threaded();
    std::unordered_map<long, std::vector<long>>* links = databaseUtil.loadInwardLinks_fromOutward(links_inward);
    //std::unordered_map<long, std::vector<long>>* links = databaseUtil.loadInwardLinks_grouped();

    if(fractional_problem_size != 1) // reduce probelm size if neccessary
        graph::reduceGraphSize(fractional_problem_size, links);

    graph wikiGraph(links, databaseUtil);

    time_t end_t;
    time(&end_t);

    std::cout << "It took " << difftime(end_t, start_t) << " seconds." << std::endl;

    std::vector<std::pair<long, double>> res = wikiGraph.pageRankSharedMemory(0.000001, threads);

    /* Output top 100 ranks
    std::sort(res.begin(), res.end(), [](auto &left, auto &right) {
        return left.second > right.second;
    });

    for (int i = 0; i < 100; i++){
        std::cout<<databaseUtil.getTitle(res[i].first)<<":"<<res[i].second<<std::endl;
    }
        */
}