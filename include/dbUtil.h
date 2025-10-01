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
    std::unordered_map<long, std::vector<long>>* loadLinks(void);
    long getId(std::string title);

private:
    sqlite3 *db;
};