#include "sqlite3.h"
#include <unordered_map>
#include <vector>
#include <string>

class graph
{

public:
    graph(std::unordered_map<long, std::vector<long>>* link) : links(link) {};

    std::vector<std::string> search(long start, long end);

private:
    std::unordered_map<long, std::vector<long>>* links;
    std::vector<bool> visited;

    std::vector<long> bfs(long start, long end);

    static const int MAX_DEPTH = 15;
};