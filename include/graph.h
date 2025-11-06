#include <unordered_map>
#include <vector>
#include <string>
#include "dbUtil.h"

class graph{

public:
    graph(std::unordered_map<long, std::vector<long>>* link, dbUtil &db) : links(link), databaseUtil(db) {};

    std::vector<std::string> search(long start, long end);
    std::vector<std::pair<long, double>> pageRank(double threshold);

private:
    std::unordered_map<long, std::vector<long>>* links;
    dbUtil &databaseUtil;
    std::vector<bool> visited;

    std::vector<long> bfs(long start, long end);

    static const int MAX_DEPTH = 15;
};