#include <iostream>
#include <ctime>
#include "sqlite3.h"
#include "dbUtil.h"
#include "graph.h"
#include <omp.h>

int main(int argc, char* argv[]){
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

    std::unordered_map<long, std::vector<long>>* links = databaseUtil.loadLinks_grouped();

    graph wikiGraph(links, databaseUtil);

    time_t end_t;
    time(&end_t);

    std::cout << "It took " << difftime(end_t, start_t) << " seconds." << std::endl;

    time(&start_t);

    wikiGraph.pageRank(0.000001);

    time(&end_t);

    std::cout << "It took " << difftime(end_t, start_t) << " seconds." << std::endl;
}