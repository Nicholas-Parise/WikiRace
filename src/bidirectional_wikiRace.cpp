#include <iostream>
#include <ctime>
#include <unordered_set>
#include <random>
#include "sqlite3.h"
#include "dbUtil.h"
#include "graph.h"

/* Used for creating smaller 'subgraphs' of the graph given (creates a smaller hashmap from the wiki hashmap).
*  It randomly samples n pages from the master hashmap.
*/
std::unordered_map<long, std::vector<long>>* sampleNPages(int nOfPages, std::unordered_map<long, std::vector<long>>* links) {
    std::unordered_map<long, std::vector<long>>* new_links = new std::unordered_map<long, std::vector<long>>;
    std::unordered_set<long> new_link_set;
    std::vector<long> keys;

    std::cout << "Trimming wikigraph..." << std::flush;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, links->size() - 1);

    for (auto& p : *links) keys.push_back(p.first); // get a vector of map key names
    while (new_link_set.size() < nOfPages) {    // while set of new pages is not full, randomly pick a key to add to it
        long key = dist(gen);
        new_link_set.insert(keys.at(key));
    }

    // copy selected pages into the new map
    for (long key : new_link_set) {
        (*new_links)[key] = links->at(key);
    }

    std::cout << "\rFinished Trimming wikigraph" << std::endl;

    return new_links;
}

long selectCandidate(const std::vector<std::pair<long, std::string>>& candidates) {
    std::string line;
    while (true){
        std::cout << "Which candidate matches the article the closest? Enter the number, or 0 to type a new name: ";
        std::getline(std::cin, line);
            
        int selection = -1;
        try {
            selection = std::stoi(line);
        } catch (...) {
            std::cout << "Invalid input, please enter a number." << std::endl;
            continue;
        }

        if (selection == 0) {
            return 0; // type new name
        }else if (selection >= 1 && selection <= static_cast<int>(candidates.size())) {
            return candidates[selection - 1].first; // return the page ID
        } else {
            std::cout << "Please enter a valid number from the list, or 0 to type a new name." << std::endl;
        }
    }
}


/* cin that prompts user to enter an article name and select one of the found articles
* 
* Returns:
* ID of article selected
* or -1 if user enters ":exit" to quit
*/
long promptForArticle(dbUtil& databaseUtil, std::string first_second) {

    std::string input;

    while (true){
        std::cout << "Enter "<<first_second<<" article name (or :exit): ";

        std::getline(std::cin, input);

        if (input == ":exit") { return -1; }

        std::vector<std::pair<long, std::string>> candidates = databaseUtil.getTitleCandidates(input);

        std::cout << "which candidates matches the article the closest: " << std::endl;

        if (candidates.empty()){
            std::cout << "no articles could be found with a name similar to that"<<std::endl;
            continue;
        }
        
        for (size_t  i = 0; i < candidates.size(); i++){
            std::cout << i + 1 << ": " << candidates[i].second << std::endl;
        }

        int choice = selectCandidate(candidates);

        if (choice == 0) {
            continue;
        } else {
            return choice;
        }
    }
}

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

    std::unordered_map<long, std::vector<long>>* links = databaseUtil.loadLinks_grouped_Threaded();
    //std::unordered_map<long, std::vector<long>>* inverted_links = databaseUtil.loadInwardLinks_grouped();
    std::unordered_map<long, std::vector<long>>* inverted_links = databaseUtil.loadInwardLinks_fromOutward(links);

    time_t end_t;
    time(&end_t);

    std::cout << "It took " << difftime(end_t, start_t) << " seconds." << std::endl;

    // if user wants a smaller graph
    bool smallMode = false;
    while (!smallMode) {
        std::string input;
        std::cout << "Enter number of pages (recommended at least 1000) or nothing to use entire wikigraph." << std::endl;
        std::getline(std::cin, input);

        if (!input.empty()) {
            int selection = -1;
            try {
                selection = std::stoi(input);
                if (selection >= links->size()) {   //user wants a graph LARGER than wikipedia!? No.
                    std::cout << "That number is too large. The number of pages must be less than " << links->size() << std::endl;
                    continue;
                }
                auto* new_links = sampleNPages(selection, links);
                delete links;
                links = new_links;
                std::cout << links->size() << std::endl;
                smallMode = true;
            }
            catch (const std::exception& e) {
                std::cout << e.what() << std::endl;
                std::cout << "Invalid input, please enter a number or nothing." << std::endl;
            }
            catch (...) {}
        }
        else {
            break; // user entered nothing, use entire wikigraph
        }
    }

    graph wikiGraph(links, databaseUtil);

    if (smallMode) {    // Do to smaller graphs having an uncontrollable list of pages, we don't let user choose articles
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
            std::vector<std::string> output = wikiGraph.bidirectional_search(firstArticle, secondArticle, false, inverted_links);
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
    else {  // Default behaviour enabled if user is using the full wikigraph
        while (true) {
            long firstArticle = promptForArticle(databaseUtil, "first");

            if (firstArticle == -1) { break; }

            std::cout << firstArticle << std::endl;

            long secondArticle = promptForArticle(databaseUtil, "second");

            if (secondArticle == -1) { break; }

            std::cout << secondArticle << std::endl;

            std::string mode;
            std::cout << "Enter 0 for sequential and 1 for parallel: " << std::endl;
            std::getline(std::cin, mode);
            std::vector<std::string> output;
            if (mode == "0") {
                output = wikiGraph.bidirectional_search(firstArticle, secondArticle, false, inverted_links);
            } else {
                output = wikiGraph.bidirectional_search(firstArticle, secondArticle, true, inverted_links);
            }
            for (std::string str: output){
                std::cout << str << std::endl;
            }

        }
    }

}