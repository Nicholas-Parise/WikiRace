#include "graph.h"
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <cmath>

std::vector<std::string> graph::search(long start, long end) {
    time_t start_t;
    time(&start_t);


    std::vector<std::string> output = { };
    std::vector<long> output_long;
    output_long = bfs(start, end);

    if (output_long.size() == 0) { return output; }
   
    std::cout << "Found a path with " << output_long.size() << " steps:" << std::endl;

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
    std::reverse(path.begin(), path.end());
    return path;
}


std::vector<std::pair<long, double>> graph::pageRank(double threshold){
    static double d = 0.85; // damping factor
    
    std::cout<<"Allocating Vectors"<<std::endl;
    std::unordered_map<long, int> idMap = std::unordered_map<long, int>(links->size());
    std::vector<double> prevRanks = std::vector<double>(links->size(), 1 / links->size());
    std::vector<double> nextRanks = std::vector<double>(links->size(), 0.0);

    // Create map of id -> index
    int index = 0;
    for(auto kv : *links) {
        idMap[kv.first] = index;
        index++;
    }

    static int MAX_ITERATIONS = 500;
    int curr_interation = 1;

    std::cout<<"Starting Page Rank Algorithm"<<std::endl;
    while(curr_interation <= MAX_ITERATIONS) {
        std::cout<<"Page Rank Iteration: "<<curr_interation<<std::endl;
        
        std::fill(nextRanks.begin(), nextRanks.end(), (1.0 - d) / links->size());

        // disperse weights
        for(auto kv : *links) {
            index = idMap[kv.first];
            std::vector<long> neighbours = kv.second;

            // n is ID of neighbour
            // kv.first is ID of nocurrent node
            for(auto n : neighbours) {
                nextRanks[idMap[n]] += d * prevRanks[idMap[kv.first]] / neighbours.size();
            }
        }

        auto tmp = nextRanks;
        nextRanks = prevRanks;
        prevRanks = tmp;
        
        std::cout<<"Testing for Convergence"<<std::endl;
        // check convergence
        double diff = 0.0;
        for (int i = 0; i < nextRanks.size(); i++)
            diff += std::abs(nextRanks[i] - prevRanks[i]);
        
        if(diff < threshold){
            break;
        }
        std::cout<<"Diff: "<<diff<<std::endl;

        curr_interation++;
    }


    std::vector<std::pair<long, double>> ranks = std::vector<std::pair<long, double>>(links->size());
    index = 0;
    for(auto kv : *links) {
        ranks[index] = {kv.first, prevRanks[idMap[kv.first]]};
        index++;
    }

    return ranks;
}
