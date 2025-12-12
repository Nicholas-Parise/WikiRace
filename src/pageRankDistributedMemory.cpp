#include <iostream>
#include <ctime>
#include "sqlite3.h"
#include "dbUtil.h"
#include "graph.h"
#include <unordered_map>
#include <chrono>
#include <mpi.h>

int main(int argc, char* argv[]){
    if (argc < 2) { 
        std::cerr << "Usage: " << argv[0] << " <fractional_problem_size (0,1])>" << std::endl;
        return 1;
    }

    double fractional_problem_size = std::stod(argv[1]);
    static double d = 0.85; // damping factor
    static double threshold = 0.000001;
    
    int comm_sz;
    int my_rank;
    int local_n;
    int N;
    std::vector<std::pair<int, std::vector<int>>> local_graph; 
    std::unordered_map<long, std::vector<long>>* links; // only used by master
    std::unordered_map<long, int> idMap; // only used by master

    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point end;
    std::vector<std::pair<std::string, int>> timeline;

    time_t start_t;
    time(&start_t);

    start = std::chrono::high_resolution_clock::now();
    
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // Partition Graph
    if (my_rank == 0) { // master
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

        dbUtil databaseUtil(db);

        links = databaseUtil.loadLinks_grouped_Threaded(); // read in data

        if(fractional_problem_size != 1){ // reduce probelm size if neccessary
            std::cout<<"Reducing Graph Size"<<std::endl;
            graph::reduceGraphSize(fractional_problem_size, links);
        }

        N = links->size();

        // build idMap
        idMap.reserve(N);
        int tmp = 0;
        for (auto kv : (*links)){
            idMap[kv.first] = tmp;
            tmp++;
        }

        int min_nodes = N / comm_sz;
        int remainder = N % comm_sz;
        int breakpoint = min_nodes;
        if (remainder >= 1) breakpoint++; // extra node
        bool send_length = false;
        local_n = breakpoint; // master local_n

        std::cout<<"Dividing Data Domain"<<std::endl;

        // divide data domain
        int node = 0;
        tmp = 0;
        for (auto kv : (*links)){
            if (node == 0){ // master partition
                std::vector<int> indices;

                for (long out : kv.second)
                    indices.push_back(idMap[out]);

                local_graph.push_back({idMap[kv.first], indices});
                kv.second.clear();
                kv.second.shrink_to_fit();
            }
            else { // worker partition
                if (send_length){
                    int length = min_nodes;
                    if (remainder >= node+1) length++;
                    MPI_Send(&N, 1, MPI_INT, node, NULL, MPI_COMM_WORLD); // MPI_Send(global N)
                    MPI_Send(&length, 1, MPI_INT, node, NULL, MPI_COMM_WORLD); // MPI_Send(node's local_n)

                    send_length = false;
                }

                int index = idMap[kv.first];
                MPI_Send(&index, 1, MPI_INT, node, NULL, MPI_COMM_WORLD); // MPI_Send(int)

                int l = kv.second.size();
                MPI_Send(&l, 1, MPI_INT, node, NULL, MPI_COMM_WORLD); // MPI_Send(length of out nodes)

                // for each out: MPI_Send(int)
                for (long out : kv.second){
                    int o = idMap[out];
                    MPI_Send(&o, 1, MPI_INT, node, NULL, MPI_COMM_WORLD);
                }
            }

            // check for when to switch nodes
            tmp++;
            if (tmp >= breakpoint){
                if (node == 0){
                    end = std::chrono::high_resolution_clock::now();
                    auto duration = end - start;
                    int milliseconds = (int) std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                    timeline.push_back({"computation", milliseconds});

                    start = std::chrono::high_resolution_clock::now();
                }

                std::cout<<"Finished node: "<<node<<std::endl;
                node++;
                send_length = true;
                std::cout<<"Old breakpoint: "<<breakpoint<<std::endl;
                breakpoint += min_nodes;
                if (remainder >= node+1) breakpoint++;
                std::cout<<"New breakpoint: "<<breakpoint<<std::endl;
            }
        }
    }
    else { // worker 
        // receive global N from master
        MPI_Recv(&N, 1, MPI_INT, 0, NULL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // receive local_n from master
        MPI_Recv(&local_n, 1, MPI_INT, 0, NULL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (int i = 0; i < local_n; i++){
            int index;
            MPI_Recv(&index, 1, MPI_INT, 0, NULL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int l;
            MPI_Recv(&l, 1, MPI_INT, 0, NULL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            std::vector<int> indices;
            int edge;
            for (int j = 0; j < l; j++){
                MPI_Recv(&edge, 1, MPI_INT, 0, NULL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                indices.push_back(edge);
            }

            local_graph.push_back({index, indices});
        }
    }

    end = std::chrono::high_resolution_clock::now();
    auto duration = end - start;
    int milliseconds = (int) std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    timeline.push_back({"communication", milliseconds});

    start = std::chrono::high_resolution_clock::now();

    if (my_rank == 0)
        std::cout<<"Starting PageRank Algorithm"<<std::endl;

    double randomSurferValue = (1.0 - d) / N;
    double diff = 1;
    if (my_rank == 0)
        std::cout<<"Allocating Vectors "<<N<<std::endl;
    std::vector<double> prevRanks = std::vector<double>(N, 1.0 / N);
    std::vector<double> nextRanks = std::vector<double>(N);

    int iteration = 1;

    // Main PageRank algorithm
    while (diff >= threshold){
        if (my_rank == 0){
            std::cout<<"Iteration: "<<iteration<<std::endl;
        }
        iteration++;

        if (my_rank == 0)
            std::fill(nextRanks.begin(), nextRanks.end(), randomSurferValue); // add random surfer value on master
        else
            std::fill(nextRanks.begin(), nextRanks.end(), 0);

        // compute local page ranks
        for (auto pair : local_graph){
            for (int edge : pair.second){
                nextRanks[edge] += d * prevRanks[pair.first] / pair.second.size();
            }
        }

        end = std::chrono::high_resolution_clock::now();
        duration = end - start;
        milliseconds = (int) std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        timeline.push_back({"computation", milliseconds});
        start = std::chrono::high_resolution_clock::now();

        // Allreduce nextRanks
        MPI_Allreduce(MPI_IN_PLACE, nextRanks.data(), N, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        end = std::chrono::high_resolution_clock::now();
        duration = end - start;
        milliseconds = (int) std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        timeline.push_back({"communication", milliseconds});
        start = std::chrono::high_resolution_clock::now();

        // calculate diffs
        diff = 0;
        for (auto pair : local_graph){
            diff += std::abs(nextRanks[pair.first] - prevRanks[pair.first]);
        }

        end = std::chrono::high_resolution_clock::now();
        duration = end - start;
        milliseconds = (int) std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        timeline.push_back({"computation", milliseconds});
        start = std::chrono::high_resolution_clock::now();

        // Allreduce the diff
        MPI_Allreduce(MPI_IN_PLACE, &diff, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        end = std::chrono::high_resolution_clock::now();
        duration = end - start;
        milliseconds = (int) std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        timeline.push_back({"communication", milliseconds});
        start = std::chrono::high_resolution_clock::now();

        if (my_rank == 0)
            std::cout<<"Diff: "<<diff<<std::endl;

        // swap
        auto temp = nextRanks;
        nextRanks = prevRanks;
        prevRanks = temp;
    }

    if (my_rank == 0) {
        time_t end_t;
        time(&end_t);

        std::cout<<"Finished"<<std::endl;
        std::cout << "It took " << difftime(end_t, start_t) << " seconds." << std::endl;

        for (auto item : timeline){
            std::cout<<item.first<<","<<item.second<<std::endl;
        }
    }

    MPI_Finalize();
}