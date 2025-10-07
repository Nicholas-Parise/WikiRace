#include "graph.h"
#include "dbUtil.h"
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <ctime>

std::vector<std::string> graph::search(long start, long end) {
    time_t start_t;
    time(&start_t);


    std::vector<std::string> output = { };
    std::vector<long> output_long;
    output_long = bfs(start, end);

    if (output_long.size() == 0) { return output; }
    sqlite3* db;
    int rc = sqlite3_open("../../wikipedia.sqlite", &db);
    if (rc)
    {
        std::cerr << "Can't open database (for fetching page titles): " << sqlite3_errmsg(db) << std::endl;
        return output;
    }
    std::cout << "Found a path with " << output_long.size() << " steps:" << std::endl;

    dbUtil databaseUtil(db);
    for (long l : output_long) {
        std::cout << databaseUtil.getTitle(l) << std::endl;
    }

    time_t end_t;
    time(&end_t);
    std::cout << "It took " << difftime(end_t, start_t) << " seconds to find a path." << std::endl;
    return output;
}

struct NodeState {
    long id;
    int depth;
};

std::vector<long> graph::bfs(long start, long end) {
    std::unordered_map<long, long> parent;
    std::vector<long> empty;
    std::queue<NodeState> q;

    int depth = 0;

    q.push({start, -1});
    parent[start] = -1;
    
    while (!q.empty()) {
        NodeState currentNode = q.front();
        q.pop();

        if (currentNode.depth > MAX_DEPTH) {
            std::cerr << "Over max depth of " << MAX_DEPTH << "." << std::endl;
            break;
        }

        if (depth < currentNode.depth) {
            std::cout << "Current Depth: " << currentNode.depth << std::endl;
            depth = currentNode.depth;
        }

        if (currentNode.id == end) break;

        auto it = links->find(currentNode.id);
        if (it == links->end()) continue; // no outgoing links

        for (long neighbour : it->second) {
            if (!parent.count(neighbour)) {
                parent[neighbour] = currentNode.id;
                q.push({neighbour, currentNode.depth+1});
            }
        }
    }

    // reconstruct path
    std::vector<long> path;
    if (!parent.count(end)) return path; // key not found

    for (int v = end; v != -1; v = parent[v])
        path.push_back(v);
    reverse(path.begin(), path.end());
    return path;
}
