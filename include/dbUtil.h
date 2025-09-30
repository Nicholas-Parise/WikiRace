#include "sqlite3.h"
#include <string>

class dbUtil
{

public:
    dbUtil(sqlite3 *dbConnection) : db(dbConnection) {};
    std::string getTitle(long pageId);
    long getId(std::string title);

private:
    sqlite3 *db;
};