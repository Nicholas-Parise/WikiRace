#include <unordered_map>
#include <vector>
#include <string>
#include "dbUtil.h"

class graph{

public:
    graph(std::unordered_map<long, std::vector<long>>* link, dbUtil &db) : links(link), databaseUtil(db) {};

    std::vector<std::string> search(long start, long end, bool parallel);
    std::vector<std::string> bidirectional_search(long start, long end, bool parallel, std::unordered_map<long, std::vector<long>>* incoming_link);
    std::vector<std::pair<long, double>> pageRank(double threshold);

private:
    std::unordered_map<long, std::vector<long>>* links;
    dbUtil &databaseUtil;
    std::vector<bool> visited;

    std::vector<long> bfs(long start, long end);
    std::vector<long> parallel_bfs(long start, long end);
    std::vector<long> bidirectional(long start, long end, std::unordered_map<long, std::vector<long>>* incoming_link);
    std::vector<long> parallel_bidirectional(long start, long end, std::unordered_map<long, std::vector<long>>* incoming_link);

    static const int MAX_DEPTH = 15;
    static const int NUM_THREADS = 16;
};