#include <algorithm>
#include <iostream>
#include <ctime>
#include <unordered_set>

#include "sqlite3.h"
#include "dbUtil.h"
#include "graph.h"
#include "mpi.h"

using namespace std;

long selectCandidate(const std::vector<std::pair<long, std::string>>& candidates) {
    std::string line;
    while (true){
        std::cout << "Which candidate matches the article the closest? Enter the number, or 0 to type a new name: ";
        std::getline(std::cin, line);

        int selection = -1;
        try {
            selection = std::stoi(line);
        } catch (...) {
            std::cout << "Invalid input, please enter a number." << std::endl;
            continue;
        }

        if (selection == 0) {
            return 0; // type new name
        }else if (selection >= 1 && selection <= static_cast<int>(candidates.size())) {
            return candidates[selection - 1].first; // return the page ID
        } else {
            std::cout << "Please enter a valid number from the list, or 0 to type a new name." << std::endl;
        }
    }
}


/* cin that prompts user to enter an article name and select one of the found articles
*
* Returns:
* ID of article selected
* or -1 if user enters ":exit" to quit
*/
long promptForArticle(dbUtil& databaseUtil, std::string first_second) {

    std::string input;

    while (true){
        std::cout << "Enter "<<first_second<<" article name (or :exit): ";

        std::getline(std::cin, input);

        if (input == ":exit") { return -1; }

        std::vector<std::pair<long, std::string>> candidates = databaseUtil.getTitleCandidates(input);

        std::cout << "which candidates matches the article the closest: " << std::endl;

        if (candidates.empty()){
            std::cout << "no articles could be found with a name similar to that"<<std::endl;
            continue;
        }

        for (size_t  i = 0; i < candidates.size(); i++){
            std::cout << i + 1 << ": " << candidates[i].second << std::endl;
        }

        int choice = selectCandidate(candidates);

        if (choice == 0) {
            continue;
        } else {
            return choice;
        }
    }
}
int owner(long nodeId, int size) {
    return nodeId % size;
}

int main(int argc, char *argv[]) {
    int rank, size;
    std::unordered_map<long, std::vector<long>>* links;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    dbUtil* databaseUtil; // made it a pointer because this is the only way I could think to declare it outside of the block scope
    long first, second;
    if (rank == 0 ) {   //load database into graph
        cout << "Warning: There is a high chance this program will run out of memory and crash if you select two articles more than 3 layers apart, and are using 2 or more processors." << endl;
        time_t start_t;
        time(&start_t);

        sqlite3 *db;
        int rc = sqlite3_open("../wikipedia.sqlite", &db);

        if (rc)
        {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
            return 1;
        }
        else
        {
            std::cout << "Opened database successfully" << std::endl;
        }

        databaseUtil = new dbUtil(db);

        links = databaseUtil->loadLinks_grouped_Threaded();

        time_t end_t;
        time(&end_t);

        std::cout << "It took " << difftime(end_t, start_t) << " seconds." << std::endl;

        graph wikiGraph(links, *(databaseUtil));

        long firstArticle = promptForArticle(*(databaseUtil), "first");

        if (firstArticle == -1) {
            return 1;
        }

        std::cout << firstArticle << std::endl;

        long secondArticle = promptForArticle(*(databaseUtil), "second");

        if (secondArticle == -1) {
            return 1;
        }

        std::cout << secondArticle << std::endl;
        first = firstArticle;
        second = secondArticle;
    }
    MPI_Bcast(&first, 1, MPI_LONG, 0, MPI_COMM_WORLD);  //broadcast the first and goal article to the other processes
    MPI_Bcast(&second, 1, MPI_LONG, 0, MPI_COMM_WORLD);
    vector<vector<int>> sendKeys(size);
    vector<vector<int>> sendOffsets(size);
    vector<vector<int>> sendAdj(size);
    double total_start = MPI_Wtime();
    // partitioning and sending the adjacency list to other processes: this process dominates the run time before searching can even begin
    if (rank == 0) {
        // first create the buffers needed for sending the graph partitions
        cout << "Partitioning Graph" << endl;
        for (auto it : *links) {
            int node = it.first;
            int owner_rank = owner(node, size); // returns the rank we need to fill the send buffer for

            sendKeys[owner_rank].push_back(node);
            sendOffsets[owner_rank].push_back(sendAdj[owner_rank].size());

            // append adjacency list
            sendAdj[owner_rank].insert(sendAdj[owner_rank].end(), it.second.begin(), it.second.end());
        }
    }
    if (rank == 0) {    // then load the buffers with the adjacency list and send the buffers/sizes
        for (int r = 1; r < size; r++) {
            int numNodes = sendKeys[r].size();
            int numAdj   = sendAdj[r].size();

            MPI_Send(&numNodes, 1, MPI_INT, r, 0, MPI_COMM_WORLD);
            MPI_Send(&numAdj,   1, MPI_INT, r, 0, MPI_COMM_WORLD);

            MPI_Send(sendKeys[r].data(),    numNodes, MPI_INT, r, 0, MPI_COMM_WORLD);
            MPI_Send(sendOffsets[r].data(), numNodes, MPI_INT, r, 0, MPI_COMM_WORLD);
            MPI_Send(sendAdj[r].data(),     numAdj,   MPI_INT, r, 0, MPI_COMM_WORLD);
        }
    }
    int numNodes = 0, numAdj = 0;

    vector<int> keys;
    vector<int> offsets;
    vector<int> adj;

    if (rank != 0) {    // all worker nodes receive adj list and sizes from 0
        MPI_Recv(&numNodes, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&numAdj,   1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        keys.resize(numNodes);
        offsets.resize(numNodes);
        adj.resize(numAdj);

        MPI_Recv(keys.data(),    numNodes, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(offsets.data(), numNodes, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(adj.data(),     numAdj,   MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    else { // master process also needs its own partition
        numNodes = sendKeys[0].size();
        numAdj   = sendAdj[0].size();

        keys     = sendKeys[0];
        offsets  = sendOffsets[0];
        adj      = sendAdj[0];
    }
    std::unordered_map<int, std::pair<int,int>> localRanges;

    for (int i = 0; i < numNodes; i++) { // rebuild adjacency list using CSR representation
        int node = keys[i];
        int start = offsets[i];
        int end = (i + 1 < numNodes) ? offsets[i+1] : numAdj;
        localRanges[node] = {start, end}; // used for finding which part of adj stores the neighbours for node
    }
    std::cout << "Rank " << rank << " owns "
          << numNodes << " nodes and "
          << numAdj   << " adjacency entries."
          << std::endl;
    MPI_Barrier(MPI_COMM_WORLD); // this barrier is technically unnecessary but makes std output a bit cleaner with low time cost
    if (rank == 0) {
        cout << "Now starting search..." << endl;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    double compute_start = MPI_Wtime();
    vector<long> next_frontier;
    vector<long> frontier;
    unordered_map<long, long> parents;
    if (owner(first, size) == rank) {
        frontier.push_back(first);
    }
    bool done = false;
    int depth = 1;
    while (!done){
        // frontier expansion
        vector<vector<long>> buckets(size);
        vector<int> sendcounts(size);
        vector<int> sdispls(size);
        vector<vector<long>> parent_buckets(size);
        sdispls[0] = 0;
        {
            unordered_set<long> seen;
            for (auto u : frontier) {
                if (seen.count(u)) {
                    std::cerr << "DUPLICATE in frontier: " << u << std::endl;
                }
                seen.insert(u);
            }
        }

        for (auto u: frontier) { //u = node being explored
            int s = localRanges[u].first; int e = localRanges[u].second;
            for (int i = s; i < e; i++) {
                auto v = adj[i]; //v = neighbor node
                if (parents.find(v) != parents.end()) // check if node was visited
                    continue;
                int k = owner(v, size);
                parent_buckets[k].push_back(u);
                buckets[k].push_back(v);
            }
        }
        for(int r = 0; r < size; r++)
            sendcounts[r] = buckets[r].size();
        for(int r = 1; r < size; r++)
            sdispls[r] = sdispls[r-1] + sendcounts[r-1];
        int total_send = sdispls[size-1] + sendcounts[size-1];
        std::vector<long> sendbuf(total_send);
        vector<long> parent_sendbuf(total_send);
        for (int r = 0; r < size; r++)
            std::copy(buckets[r].begin(), buckets[r].end(), sendbuf.begin() + sdispls[r]);
        for (int r = 0; r < size; r++)
            std::copy(parent_buckets[r].begin(), parent_buckets[r].end(), parent_sendbuf.begin() + sdispls[r]);
        std::vector<int> recvcounts(size);
        MPI_Alltoall(sendcounts.data(), 1, MPI_INT,
                     recvcounts.data(), 1, MPI_INT,
                     MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        std::vector<int> rdispls(size);
        rdispls[0] = 0;
        for (int r = 1; r < size; r++)
            rdispls[r] = rdispls[r-1] + recvcounts[r-1];

        int recv_total = rdispls[size-1] + recvcounts[size-1];
        std::vector<long> recvbuf(recv_total);
        std::vector<long> parent_recvbuf(recv_total);
        MPI_Alltoallv(
            sendbuf.data(), sendcounts.data(), sdispls.data(), MPI_LONG,
            recvbuf.data(), recvcounts.data(), rdispls.data(), MPI_LONG,
            MPI_COMM_WORLD
        );
        MPI_Alltoallv(
        parent_sendbuf.data(), sendcounts.data(), sdispls.data(), MPI_LONG,
        parent_recvbuf.data(), recvcounts.data(), rdispls.data(), MPI_LONG,
            MPI_COMM_WORLD
        );
        unordered_set<long> seen;
        for (int x = 0; x < recvbuf.size(); x++) {
            int node = recvbuf[x];
            if (not seen.count(node)) {
                if (parents.find(node) != parents.end()) continue; //ignore nodes that we have already seen
                seen.insert(node);
                next_frontier.push_back(node);
            }
            if (parents.find(node) == parents.end()) //avoids some race conditions caused by some articles within a small depth ratio having the same neighbour
                parents[node] = parent_recvbuf[x];
        }
        MPI_Barrier(MPI_COMM_WORLD);
        bool local_found = false;
        bool local_empty = false;
        // Check frontier for the goal article
        for (int x : next_frontier) {
            if (x == second)
                local_found = true;
        }
        // Check if frontier is empty
        local_empty = next_frontier.empty();

        int local_flag = local_found ? rank : -1; // int will equal the rank of who found it
        int local_empty_flag = local_empty ? 1 : -1;
        int global_flag = -1;
        int global_empty = -1;

        // One rank will broadcast its answer
        MPI_Allreduce(&local_flag, &global_flag, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);  //bcast for if end was found
        MPI_Allreduce(&local_empty_flag, &global_empty, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD); //bcast for if all frontiers are empty

        // If any rank found it, global_flag != -1
        done = (global_flag != -1);
        int winner = global_flag;  // global_flag contains the winning rank

        if (done) { //print final result
            double end = MPI_Wtime();
            if (rank == 0)
                cout << "Found by rank " << winner << endl;
            if (rank == 0) {
                cout << "Including partitioning it took " << end - total_start << " seconds" << endl;
                cout << "BFS took " << end - compute_start << " seconds" << endl;
            }
            // path construction
            long cur = second;
            vector<long> path;

            while (cur != first) {
                int owner_rank = owner(cur, size);

                long parent;

                if (rank == owner_rank) {
                    parent = parents[cur];
                }

                // Broadcast parent to all
                MPI_Bcast(&parent, 1, MPI_LONG, owner_rank, MPI_COMM_WORLD);

                if (rank == 0) {
                    path.push_back(cur);
                }
                cur = parent;
            }
            // path output
            if (rank == 0) {
                path.push_back(first);
                std::reverse(path.begin(), path.end());
                for (auto p : path) {
                    if (p != second)
                        cout << databaseUtil->getTitle(p) << " -> ";
                    else
                        cout << databaseUtil->getTitle(p) << endl;
                }
            }
        }
        if (!done) {    //if we're going again, advance the frontiers
            {
                unordered_set<long> seen;
                for (auto u : next_frontier) {
                    if (seen.count(u)) {
                        std::cerr << "DUPLICATE in NEXT frontier: " << u << std::endl;
                    }
                    seen.insert(u);
                }
            }
            frontier = std::move(next_frontier);
            if (rank == 0)
                cout << "Depth " << depth << endl;
            depth++;
        }
        if (global_empty == size) { //all frontiers are empty
            if (rank == 0) {
                cout << "Couldn't find a path" << endl;
                return 1;
            }
        }
        if (depth >= 15 && !done)
            if (rank == 0) {
                cout << "Over Max Depth" << endl;
                return 1;
            }


    }
    MPI_Finalize();
}