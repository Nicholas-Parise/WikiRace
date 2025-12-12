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

std::vector<std::string> graph::search(long start, long end, bool parallel) {
    time_t start_t;
    time(&start_t);


    std::vector<std::string> output = { };
    std::vector<long> output_long;
    if (parallel) output_long = parallel_bfs(start, end);
    else output_long = bfs(start, end);

    if (output_long.size() == 0) {
        time_t end_t;
        time(&end_t);
        std::cout << "Could not find a path after " << difftime(end_t, start_t) << " seconds." << std::endl;
        return output; 
    }
   
    std::cout << "Found a path with " << output_long.size() << " steps:" << std::endl;

    for (long l : output_long) {
        std::cout << databaseUtil.getTitle(l) << std::endl;
    }

    time_t end_t;
    time(&end_t);
    std::cout << "It took " << difftime(end_t, start_t) << " seconds to find a path." << std::endl;
    return output;
}

std::vector<std::string> graph::bidirectional_search(long start, long end, bool parallel, std::unordered_map<long, std::vector<long>>* incominglink) {
    time_t start_t;
    time(&start_t);


    std::vector<std::string> output = { };
    std::vector<long> output_long;
    if (parallel) output_long = parallel_bidirectional(start, end, incominglink);
    output_long = bidirectional(start, end, incominglink);

    if (output_long.size() == 0) {
        time_t end_t;
        time(&end_t);
        std::cout << "Could not find a path after " << difftime(end_t, start_t) << " seconds." << std::endl;
        return output;
    }

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

std::vector<long> graph::parallel_bfs(long start, long end) {
    std::unordered_map<long, long> parent;
    std::vector<NodeState> frontier;
    bool found = false;
    std::cout << "Now searching with " << NUM_THREADS << " threads." << std::endl;
    int depth = 0;

    frontier.push_back({start, 1});
    parent[start] = -1;

    while (!frontier.empty()) {
        depth++;
        if (found) break;
        if (depth > MAX_DEPTH) {
            std::cerr << "Over max depth of " << MAX_DEPTH << std::endl;
            break;
        }
        int levelSize = frontier.size();
        std::vector<NodeState> next_frontier; // stores nodes on next level
        #pragma omp parallel for schedule(dynamic) num_threads(NUM_THREADS)// uses multiple threads to get a node from the current frontier and add them to the next frontier queue
        for (int i = 0; i < levelSize; i++) {
            if (found) continue; // makes loop end early when a path has already been found.
            NodeState node;
            node = frontier[i];

            if (node.id == end) found = true;
            auto it = links->find(node.id);
            if (it == links->end()) continue; // no outgoing links

            for (long neighbour : it->second) {
                if (!parent.count(neighbour)) {
                    #pragma omp critical (parent_update) // parent acts as the visited queue which is a critical region, next_q is also critical as push_back is not thread safe
                    {
                        parent[neighbour] = node.id;
                        next_frontier.push_back({neighbour, node.depth+1});
                    }
                }
            }
        }
        frontier = next_frontier; //we've finished this level, advance to the next one
        std::cout << "Current depth is " << depth << std::endl;
    }

    // reconstruct path
    std::vector<long> path;
    if (!parent.count(end)) {
        std::cout << "Explored all possible nodes" << std::endl;
        return path; // key not found
    }

    for (int v = end; v != -1; v = parent[v])
        path.push_back(v);
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<long> graph::bfs(long start, long end) {
    std::unordered_map<long, long> parent;
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
    if (!parent.count(end)) {
        std::cout << "Explored all possible nodes" << std::endl;
        return path; // key not found
    }

    for (int v = end; v != -1; v = parent[v])
        path.push_back(v);
    std::reverse(path.begin(), path.end());
    return path;
}

// Sequential version of PageRank
std::vector<long> graph::bidirectional(
        long start, long end,
        std::unordered_map<long, std::vector<long>>* incoming_link)
{
    if (start == end) return {start};

    long maxNode = std::max(start, end);
    for (auto &p : *links) maxNode = std::max(maxNode, p.first);
    for (auto &p : *incoming_link) maxNode = std::max(maxNode, p.first); //find the largest possible id to help size the visited vectors

    // Resize visited arrays
    std::vector<bool> front_visited(maxNode + 1, false);
    std::vector<bool> back_visited(maxNode + 1, false);

    std::unordered_map<long, long> front_parent;
    std::unordered_map<long, long> back_parent;

    std::queue<NodeState> front_q;
    std::queue<NodeState> back_q;

    front_q.push({start, 1});
    back_q.push({end, 1});

    front_visited[start] = true;
    back_visited[end] = true;

    front_parent[start] = -1;
    back_parent[end] = -1;

    long meeting_node = -1;

    bool done = false;

    int front_depth = 0;
    int back_depth = 0;

    while (!front_q.empty() && !back_q.empty()) {

        // Expand from front side
        while (front_q.front().depth == front_depth && !done){
            long u = front_q.front().id; front_q.pop();

            auto it = links->find(u);
            if (it != links->end()) {
                for (long v : it->second) {
                    if (!front_visited[v]) {
                        front_visited[v] = true;
                        front_parent[v] = u;
                        front_q.push({v, front_depth+1});

                        // Check meeting
                        if (back_visited[v]) {
                            meeting_node = v;
                            done = true;
                            break;
                        }
                    }
                }
            }
        }
        front_depth++;

        // Expand from back side
        while (back_q.front().depth == back_depth && !done) {
            long u = back_q.front().id; back_q.pop();

            auto it = incoming_link->find(u);
            if (it != incoming_link->end()) {
                for (long v : it->second) {
                    if (!back_visited[v]) {
                        back_visited[v] = true;
                        back_parent[v] = u;
                        back_q.push({v, back_depth+1});

                        // Check meeting
                        if (front_visited[v]) {
                            meeting_node = v;
                            done = true;
                            break;
                        }
                    }
                }
            }
        }
        back_depth++;
        if (done) break;
    }

    if (!done || meeting_node == -1) {
        return {}; // no path
    }
    std::cout << "Found a path using intermediate node " << meeting_node << std::endl;
    // Reconstruct full path
    std::vector<long> path;

    // start → meeting_node
    {
        long v = meeting_node;
        while (v != -1) {
            path.push_back(v);
            v = front_parent[v];
        }
        std::reverse(path.begin(), path.end());
    }

    // meeting_node → end
    {
        long v = back_parent[meeting_node];
        while (v != -1) {
            path.push_back(v);
            v = back_parent[v];
        }
    }

    return path;
}

std::vector<long> graph::parallel_bidirectional(long start, long end, std::unordered_map<long, std::vector<long> > *incoming_link){
    if (start == end) return {start};

    long maxNode = std::max(start, end);
    for (auto &p : *links) maxNode = std::max(maxNode, p.first);
    for (auto &p : *incoming_link) maxNode = std::max(maxNode, p.first);

    std::vector<bool> front_visited(maxNode + 1, false);
    std::vector<bool> back_visited (maxNode + 1, false);

    std::unordered_map<long,long> front_parent;
    std::unordered_map<long,long> back_parent;

    std::vector<NodeState> front_frontier;
    std::vector<NodeState> back_frontier;

    front_frontier.push_back({start,1});
    back_frontier.push_back({end,1});
    front_visited[start] = true;
    back_visited[end] = true;
    front_parent[start] = -1;
    back_parent[end] = -1;

    int front_depth = 0;
    int back_depth = 0;

    long meeting = -1;
    bool done = false;

    while (!front_frontier.empty() && !front_frontier.empty() && !done)
    {
        std::vector<NodeState> next_front;
        std::vector<NodeState> next_back;

        #pragma omp parallel sections shared(done, meeting)
        {
            // front bfs expansion
            #pragma omp section
            {
                #pragma omp parallel for
                for (long long unsigned int i=0; i<front_frontier.size(); i++)
                {
                    if (done) continue; //makes loop end early if the solution was already found
                    NodeState cur = front_frontier[i]; // does not need critical region because we are only reading

                    auto it = links->find(cur.id);
                    if (it == links->end()) continue;

                    for (long v : it->second) {
                        if (done) break;
                        #pragma omp critical (front) //used name critical section to allow the back searcher to work at the same time
                        if (!front_visited[v]) {
                            front_visited[v] = true;
                            front_parent[v] = cur.id;
                            next_front.push_back({v, front_depth + 1});

                            if (back_visited[v]) {
                                meeting = v;
                                done = true;
                            }
                        }
                    }
                }
            }

            // back bfs expansion
            #pragma omp section
            {
                #pragma omp parallel for
                for (long long unsigned int i=0; i<back_frontier.size(); i++)
                {
                    if (done) continue;
                    NodeState cur = back_frontier[i]; // does not need critical region because we are only reading

                    auto it = incoming_link->find(cur.id);
                    if (it == incoming_link->end()) continue;

                    for (long v : it->second) {
                        if (done) break;
                        #pragma omp critical (back) // used named critical region to allow the front searcher to search simultaneously
                        if (!back_visited[v]) {
                            back_visited[v] = true;
                            back_parent[v] = cur.id;
                            next_back.push_back({v, back_depth + 1});

                            if (front_visited[v]) {
                                meeting = v;
                                done = true;
                            }
                        }
                    }
                }
            }
        } // end sections

        if (done) break;

        // Move to next level after both sides expanded
        front_frontier.swap(next_front);
        back_frontier.swap(next_back);
        front_depth++;
        back_depth++;
    }

    if (!done || meeting == -1) return {};

    // ------------------------------------------------------------
    // PATH RECONSTRUCTION
    // ------------------------------------------------------------
    std::vector<long> path;

    long v = meeting;
    while (v != -1) {
        path.push_back(v);
        v = front_parent[v];
    }
    std::reverse(path.begin(), path.end());

    v = back_parent[meeting];
    while (v != -1) {
        path.push_back(v);
        v = back_parent[v];
    }

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
        for (long long unsigned int i = 0; i < nextRanks.size(); i++)
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

    time_t start_t;
    time(&start_t);

    std::vector<page> pages = std::vector<page>(links->size());
    std::unordered_map<long, int> idMap = std::unordered_map<long, int>(links->size());

    std::vector<std::pair<int, std::vector<int>>> mapped_graph;

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

    // Build mapped_graph
    for (auto kv : (*links)){
        std::vector<int> indices;

        for (long out : kv.second)
            indices.push_back(idMap[out]);
        mapped_graph.push_back({idMap[kv.first], indices});
    }
    
    std::cout<<"Allocating Vectors "<<links->size()<<std::endl;
    std::vector<double> prevRanks = std::vector<double>(links->size());
    std::vector<double> nextRanks = std::vector<double>(links->size());

    omp_set_num_threads(threads); // declare thread pool

    double init_val = 1.0 / links->size();
    #pragma omp parallel for
    for (long long unsigned int i = 0; i < nextRanks.size(); i++){
        prevRanks[i] = init_val;
        nextRanks[i] = 0;
    }

    static int MAX_ITERATIONS = 500;
    int curr_interation = 1;

    double randomSurferValue = (1.0 - d) / links->size();
    double diff = 0.0;

    std::cout<<"Starting Page Rank Algorithm"<<std::endl;

    while(curr_interation <= MAX_ITERATIONS) {
        std::cout<<"Page Rank Iteration: "<<curr_interation<<std::endl;

        // collect weights
        #pragma omp parallel for schedule(dynamic, 1)
        for(long long unsigned int i = 0; i < mapped_graph.size(); i++){
            nextRanks[mapped_graph[i].first] = randomSurferValue;
            for (auto in : mapped_graph[i].second) {
                nextRanks[mapped_graph[i].first] += d * prevRanks[in] / pages[in].numberOfOutLinks;
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

    time_t end_t;
    time(&end_t);

    std::cout << "It took " << difftime(end_t, start_t) << " seconds." << std::endl;

    // create output list, not timed
    #pragma omp parallel for
    for(int i = 0; i < pages.size(); i++){
        ranks[i] = {pages[i].id, prevRanks[i]};
    }
    
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


