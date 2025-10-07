#pragma once
#include "sqlite3.h"
#include <unordered_map>
#include <vector>
#include <string>

class dbUtil
{

public:
    dbUtil(sqlite3 *dbConnection) : db(dbConnection) {};
    std::string getTitle(long pageId);
    std::vector<std::pair<long, std::string>> getTitleCandidates(std::string title);
    std::unordered_map<long, std::vector<long>>* loadLinks(void); // legacy, not used anymore it's way to slow
    std::unordered_map<long, std::vector<long>>* loadLinks_grouped(void);
    long getId(std::string title);

private:
    sqlite3 *db;

    static void spinner(int &state);
    static void parseTargets(const std::string& s, std::vector<long>& out);

    static const long NUM_PAGES = 18657926;
    static const int AVG_LINKS = 22;
};