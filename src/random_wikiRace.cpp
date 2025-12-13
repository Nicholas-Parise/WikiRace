#include <iostream>
#include <ctime>
#include <unordered_set>
#include <random>
#include "sqlite3.h"
#include "dbUtil.h"
#include "graph.h"

// this file was created to have the behaviour of randomly picking two articles but for the full graph (mostly for performance comparison)

// When given a links map, return a randomly selected first aritcle and second article from the map as a std::pair
std::pair<long, long> selectArticles(std::unordered_map<long, std::vector<long>>* links) {
    std::vector<long> keys;
    int firstIndex = rand() % links->size();
    int secondIndex = rand() % links->size();
    for (auto& p : *links) keys.push_back(p.first); // get a vector of map key names
    std::pair articles = {keys.at(firstIndex), keys.at(secondIndex)};
    return articles;
}


int main(){
    // sanity check that OMP is enabled
    #ifdef _OPENMP
        std::cout << "OpenMP is enabled." << std::endl;
    #else
        std::cout << "OpenMP is not enabled." << std::endl;
    #endif
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

    //std::unordered_map<long, std::vector<long>>* links = databaseUtil.loadLinks_grouped();
    std::unordered_map<long, std::vector<long>>* links = databaseUtil.loadLinks_grouped_Threaded();

    time_t end_t;
    time(&end_t);

    std::cout << "It took " << difftime(end_t, start_t) << " seconds." << std::endl;

    // if user wants a smaller graph

    graph wikiGraph(links, databaseUtil);

    bool done = false;
    while (!done) {
        // Pick two random articles
        auto articles = selectArticles(links);
        long firstArticle = articles.first;
        long secondArticle = articles.second;
        std::string input;
        std::string firstTitle = databaseUtil.getTitle(firstArticle);
        std::string secondTitle = databaseUtil.getTitle(secondArticle);
        std::cout << firstTitle << std::endl;
        std::cout << secondTitle << std::endl;

        // Use search algorithm to find the shortest path (if it exists)
        std::vector<std::string> output = wikiGraph.search(firstArticle, secondArticle, true);
        for (std::string str: output){
            std::cout << str << std::endl;
        }

        // Ask user if they want to continue or end the program
        std::cout << "Press Enter to search between a new pair (or type :exit to exit): ";
        std::getline(std::cin, input);
        if (input == ":exit") {
            done = true;
        }
    }
}