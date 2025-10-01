#include <iostream>
#include "sqlite3.h"
#include "dbUtil.h"


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


long promptForArticle(dbUtil& databaseUtil, std::string first_second) {

    std::string input;
    long start;

    while (true){
        std::cout << "Enter "<<first_second<<" article name: ";

        std::getline(std::cin, input);

        std::vector<std::pair<long, std::string>> candidates = databaseUtil.getTitleCandidates(input);

        std::cout << "which candidates matches the article the closest: " << std::endl;

        if (candidates.empty()){
            std::cout << "no articles could be found with a name similar to that"<<std::endl;
            continue;
        }
        
        for (int i = 0; i < candidates.size(); i++){
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


int main(){

    sqlite3 *db;
    int rc = sqlite3_open("wikipedia.sqlite", &db);

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

    databaseUtil.loadLinks();

    long first_article = promptForArticle(databaseUtil, "first");

    std::cout<<first_article<<std::endl;

    long second_article = promptForArticle(databaseUtil, "second");

    std::cout<<second_article<<std::endl;

}