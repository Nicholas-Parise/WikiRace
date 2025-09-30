#include "sqlite3.h"
#include <vector>
#include <string>

class dbUtil
{

public:
    dbUtil(sqlite3 *dbConnection) : db(dbConnection) {};
    std::string getTitle(long pageId);
    std::vector<std::string> getTitleCandidates(std::string title);
    long getId(std::string title);

private:
    sqlite3 *db;
};