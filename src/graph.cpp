#include "graph.h"
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <omp.h>
#include <random>

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

    q.push({start, 1});
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
    std::vector<double> prevRanks = std::vector<double>(links->size(), 1.0 / links->size());
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
            // kv.first is ID of current node
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
        for (long i = 0; i < nextRanks.size(); i++)
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

struct page {
    long id;
    int numberOfOutLinks;
};

// OpenMP parallel implementation of PageRank
std::vector<std::pair<long, double>> graph::pageRankSharedMemory(double threshold, int threads){
    static double d = 0.85; // damping factor

    std::vector<page> pages = std::vector<page>(links->size());
    std::unordered_map<long, int> idMap = std::unordered_map<long, int>(links->size());

    std::cout<<"Creating PageID list"<<std::endl;

    // Create vector of page IDs to iterate over and vector of # out links
    int tmp = 0;
    for(auto kv : *links) {
        pages[tmp] = {kv.first, 0};
        idMap[kv.first] = tmp;
        tmp++;
    }

    std::cout<<"Counting Outgoing Connections"<<std::endl;

    // count outgoing connections
    for (auto kv : *links){
        for(long in : kv.second){
            pages[idMap[in]].numberOfOutLinks = pages[idMap[in]].numberOfOutLinks + 1;
        }
    }
    
    std::cout<<"Allocating Vectors "<<links->size()<<std::endl;
    std::vector<double> prevRanks = std::vector<double>(links->size(), 1.0 / links->size());
    std::vector<double> nextRanks = std::vector<double>(links->size(), 0.0);

    static int MAX_ITERATIONS = 500;
    int curr_interation = 1;

    double randomSurferValue = (1.0 - d) / links->size();
    double diff = 0.0;

    std::cout<<"Starting Page Rank Algorithm"<<std::endl;

    omp_set_num_threads(threads); // declare thread pool

    while(curr_interation <= MAX_ITERATIONS) {
        std::cout<<"Page Rank Iteration: "<<curr_interation<<std::endl;

        // collect weights
        #pragma omp parallel for schedule(dynamic, 1)
        for(int i = 0; i < pages.size(); i++){
            nextRanks[i] = randomSurferValue;
            for (auto in : (*links)[pages[i].id]) {
                if(pages[idMap[in]].numberOfOutLinks == 0){
                    std::cout<<"DIVISION BY ZERO"<<std::endl;
                }
                nextRanks[i] = nextRanks[i] + (d * prevRanks[idMap[in]] / pages[idMap[in]].numberOfOutLinks);
            }
        }

        std::cout<<"Finished Collecting Weights"<<std::endl;

        auto temp = nextRanks;
        nextRanks = prevRanks;
        prevRanks = temp;
            
        std::cout<<"Testing for Convergence"<<std::endl;
        // check convergence
        diff = 0.0;
        #pragma omp parallel for reduction(+:diff)
        for (int i = 0; i < nextRanks.size(); i++){
            diff += std::abs(nextRanks[i] - prevRanks[i]);
        }
        
        std::cout<<"Diff: "<<diff<<std::endl;
        if(diff < threshold){
            break;
        }

        curr_interation++;
    }

    std::vector<std::pair<long, double>> ranks = std::vector<std::pair<long, double>>(links->size());
        
    #pragma omp parallel for
    for(int i = 0; i < pages.size(); i++){
        ranks[i] = {pages[i].id, prevRanks[i]};
    }
    

    delete links;
    
    return ranks;
    
}

/*  Reduce the size of a graph by reducing the number of nodes and links to a certain fraction of the original size.
    fractionalSize is the size of the graph you want to have after the function. 0.75 = 75% the size of the original graph
    When adding extra connections, duplicates are not checked. This is okay for the sake of PageRank calculations, but not for searching.
*/
void graph::reduceGraphSize(double fractionalSize, std::unordered_map<long, std::vector<long>>* links){
    printf("Reducing Graph Size. Fraction: %.2f\n", fractionalSize);

    int nodes = links->size();
    int originalEdges = 0;

    std::unordered_set<int> remove;
    int removeNodes = (int) (nodes * (1 - fractionalSize));

    std::vector<long> survivingNodes = std::vector<long>(nodes - removeNodes);

    int tmp = 0;
    int keepIndex = 0;
    for(auto kv : (*links)){
        originalEdges += kv.second.size();

        if (tmp < removeNodes)  // mark IDs to be removed 
            remove.insert(kv.first);
        else {
            survivingNodes[keepIndex] = kv.first;
            keepIndex++;
        }

        tmp++;
    }

    printf("Original Nodes: %d, Edges: %d\n", nodes, originalEdges);

    // remove the IDs in remove set from the nodes
    for(long id : remove){ 
        (*links).erase(id);
    }

    // remove the IDs in remove set from the adjacency lists
    for(auto kv : (*links)){
        int index = 0;
        int total = kv.second.size();
        for(int i = 0; i < total; i++){
            long edge = kv.second[index];
            if(remove.count(edge)) { // remove
                kv.second[index] = kv.second.back();
                kv.second.pop_back();
            }
            else {
                index++;
            }
        }
    }

    // count how many edges are in the reduced graph
    int edges = 0;
    for(auto kv : (*links)){
        edges += kv.second.size();
    }

    int reducedEdges = (int) (originalEdges * fractionalSize);

    std::mt19937 generator{12345};
    std::uniform_int_distribution<> range{0, survivingNodes.size()};

    printf("edges: %d, reducedEdges: %d\n", edges, reducedEdges);

    while(edges < reducedEdges) { // not enough edges, randomly add edges until there is enough
        int randomNode = range(generator);

        long randomEdge;
        while(randomEdge = range(generator) == randomNode);

        (*links)[survivingNodes[randomNode]].push_back(randomEdge);

        edges++;
    }

    while(edges > reducedEdges) { // too many edges, randomly remove until the desired amount is reached
        int randomNode = range(generator);
        while((*links)[survivingNodes[randomNode]].size() == 0){ // find a random node with at least 1 edge
            randomNode = range(generator);
        }

        (*links)[survivingNodes[randomNode]].pop_back();

        edges--;
    }

    // count how many edges are in the reduced graph
    edges = 0;
    for(auto kv : (*links)){
        edges += kv.second.size();
    }

    printf("Reduced Nodes: %d, Edges: %d\n", links->size(), edges);
}


