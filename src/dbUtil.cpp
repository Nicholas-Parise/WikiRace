#include "dbUtil.h"
#include <iostream>
#include <sstream>
#include <charconv>

#include <omp.h>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

// spinner loading bar
void dbUtil::spinner(int &state) {
    const char symbols[] = {'|', '/', '-', '\\'};
    std::cout << "\rLoading links " << symbols[state % 4] << std::flush;
    state++;
}


/*void dbUtil::parseTargets(const std::string& s, std::vector<long>& out) {
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ' ')) {
        if (!token.empty()) {
            out.push_back(std::stol(token));
        }
    }
}*/


void dbUtil::parseTargets(const std::string& s, std::vector<long>& out) {
    const char* p = s.data();
    const char* end = s.data() + s.size();
    out.clear();

    while (p < end) {
        // skip spaces
        while (p < end && *p == ' ') ++p;
        if (p == end) break;

        long val;
        auto [ptr, ec] = std::from_chars(p, end, val);
        if (ec == std::errc()) {
            out.push_back(val);
            p = ptr;
        }
        else {
            break; // stop if bad data
        }
    }
}


// returns a pages name from it's ID (useful for printing)
std::string dbUtil::getTitle(long pageId)
{
    std::string sql= "SELECT title FROM pages WHERE page_id = "+std::to_string(pageId)+";";
    sqlite3_stmt *stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK){
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return "ERROR";
    }

    std::string result;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW){
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        if (text)
            result = reinterpret_cast<const char *>(text);
    }

    sqlite3_finalize(stmt);
    return result;
}



std::vector<std::pair<long, std::string>> dbUtil::getTitleCandidates(std::string title)
{
    const char *sql = "SELECT page_id, title FROM page_titles WHERE page_titles MATCH ? ORDER BY rank LIMIT 10;";
        
    sqlite3_stmt *stmt = nullptr;
    std::vector<std::pair<long, std::string>> results;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK){
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl; 
        return results;
    }

    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) { 
        long id = sqlite3_column_int64(stmt, 0);
        const unsigned char *text = sqlite3_column_text(stmt, 1);

        if (text) 
        results.emplace_back(id, reinterpret_cast<const char *>(text));
    } 
    if (rc != SQLITE_DONE) { 
        std::cerr << "Error stepping through results: " << sqlite3_errmsg(db) << std::endl; 
    } 
    sqlite3_finalize(stmt); 
    return results;
}


// returns an id given a pages name (useful for input)
long dbUtil::getId(std::string title){
    const char *sql  = "SELECT page_id FROM pages WHERE title = ?";
    sqlite3_stmt *stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK){
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);

    long result = -1;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW){
        result = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return result;
}




// create map of all links, and return pointer
std::unordered_map<long, std::vector<long>>* dbUtil::loadLinks(void){

    std::unordered_map<long, std::vector<long>>* links = new std::unordered_map<long, std::vector<long>>;
    links->reserve(NUM_PAGES);
    
    const char *sql = "SELECT source_id, target_id FROM links;";
    sqlite3_stmt *stmt = nullptr;

    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to begin transaction: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return links;
    }


    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return links;
    }

    int spinnerState = 0;
    long rowCount = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        long source = sqlite3_column_int64(stmt, 0);
        long target = sqlite3_column_int64(stmt, 1);

        if ((*links)[source].empty())
            (*links)[source].reserve(AVG_LINKS);

        (*links)[source].push_back(target);


        rowCount++;
        if (rowCount % 1000000 == 0) {
            spinner(spinnerState);
        }

    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Error reading rows: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);

    // commit
    rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to commit transaction: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    }

    std::cout<<"\rFinished Loading links"<<std::endl;

    return links;
}



// create map of all links, and return pointer
std::unordered_map<long, std::vector<long>>* dbUtil::loadLinks_grouped(void){

    std::unordered_map<long, std::vector<long>>* links = new std::unordered_map<long, std::vector<long>>;
    links->reserve(NUM_PAGES);
    
    const char *sql = "SELECT source_id, targets FROM links_grouped;";
    sqlite3_stmt *stmt = nullptr;

    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to begin transaction: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return links;
    }


    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return links;
    }

    int spinnerState = 0;
    long rowCount = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        long source = sqlite3_column_int64(stmt, 0);

        const unsigned char* targetsText = sqlite3_column_text(stmt, 1);
        if (!targetsText) continue;

        std::vector<long> targets;
        parseTargets(reinterpret_cast<const char*>(targetsText),targets);

        (*links)[source] = std::move(targets);

        rowCount++;
        if (rowCount % 100000 == 0) {
            spinner(spinnerState);
        }

    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Error reading rows: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);

    // commit
    rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to commit transaction: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    }

    std::cout<<"\rFinished Loading links"<<std::endl;

    return links;
}



struct Row {
    long source;
    std::string text;
};

// create map of all links, and return pointer
std::unordered_map<long, std::vector<long>>* dbUtil::loadLinks_grouped_Threaded(void){

    std::unordered_map<long, std::vector<long>>* links = new std::unordered_map<long, std::vector<long>>;
    links->reserve(NUM_PAGES);
    
    std::queue<Row> q;
    std::mutex qtex;
    std::condition_variable cv;
    std::atomic<bool> done{false};
    
    const char *sql = "SELECT source_id, targets FROM links_grouped;";
    sqlite3_stmt *stmt = nullptr;

    char* errmsg = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return links;
    }

    std::thread producer([&](){
        int local_rc;
        int spinnerState = 0;
        long rowCount = 0;
        while ((local_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            long source = sqlite3_column_int64(stmt, 0);

            const unsigned char* targetsText = sqlite3_column_text(stmt, 1);
            if (!targetsText) continue;

            Row row{source, reinterpret_cast<const char*>(targetsText)};
            // add to queue
            {
                std::unique_lock<std::mutex> lock(qtex);
                bool should_notify = q.size() < BATCH_SIZE;
                q.push(std::move(row));
                // now only notify IF the batch is large enough
                if (should_notify && q.size() >= BATCH_SIZE) {
                    cv.notify_all();
                }
            }

            rowCount++;         
            if (rowCount % 100000 == 0) {
                spinner(spinnerState);
            }
        }

        if (local_rc != SQLITE_DONE) {
            std::cerr << "Error reading rows: " << sqlite3_errmsg(db) << std::endl;
        }

        done.store(true, std::memory_order_release);
        cv.notify_all();
    });

    // set number of threads to amount hardware is capable of using 
    int cores = std::thread::hardware_concurrency();
    int use_threads = cores > MAX_THREADS ? MAX_THREADS : cores;
    omp_set_num_threads(use_threads);

    // give each omp thread it's own local map to write data into
    int num_threads = omp_get_max_threads();
    std::vector<std::unordered_map<long, std::vector<long>>> thread_maps;
    thread_maps.resize(num_threads);

    std::cout<<"consumer threads: "<<num_threads<<std::endl;

    #pragma omp parallel
    {
        // get the local map
        const int map_id = omp_get_thread_num();
        auto &local_map = thread_maps[map_id];
    
        std::vector<Row> rows;
        std::vector<long> local;

        rows.reserve(BATCH_SIZE);
        while(true){

            {
                std::unique_lock<std::mutex> lock(qtex);
                cv.wait(lock, [&]{ 
                    // if there is enough work we will wake up this thread
                    return q.size() >= BATCH_SIZE || done.load(std::memory_order_acquire); 
                });

                if(done.load(std::memory_order_acquire) && q.empty()){
                    //std::cout<<"Thread "<<map_id<<" Done"<<std::endl;
                    break;
                } 

                //std::cout << "Current queue size: " << q.size() << std::endl;

                if(!q.empty()){
                    // get some work to do. Lock queue and take from it.
                    int n = 0;
                    while (!q.empty() && n < BATCH_SIZE) {
                        rows.push_back(std::move(q.front()));
                        q.pop();
                        n++;
                        //std::cout<<"Thread "<<map_id<<" poped"<<std::endl;
                    }
                }else{
                    continue;
                }
            //std::cout << "Current queue size: " << q.size() <<" after poping"<<std::endl;
            }

            

            // process entire batch    
            for (auto &row : rows) {
                // clear local copy and parse data.
                local.clear();
                parseTargets(row.text, local);
                // with our finished local copy move to local map
                local_map.emplace(row.source, std::move(local));
                row.text.clear();
            }
            // reset the rows vector
            rows.clear();
            rows.reserve(BATCH_SIZE);
        
        }
    }


    producer.join();
    sqlite3_finalize(stmt);

/*
    for (auto &tmap : thread_maps) {
        for (auto &p : tmap) {
            // Move thread map vectors into result map
            //(*links)[p.first] = std::move(p.second);
            links->emplace(p.first, std::move(p.second));
        }
    }
*/
    // merge all the local maps into one large map.
    for (auto &tmap : thread_maps) {
        links->merge(tmap);
    }

    std::cout<<"\rFinished Loading links"<<std::endl;

    return links;
}


std::unordered_map<long, std::vector<long>>* dbUtil::loadInwardLinks_grouped(void){

    std::unordered_map<long, std::vector<long>>* links = new std::unordered_map<long, std::vector<long>>;
    links->reserve(NUM_PAGES);

    const char *sql = "SELECT source_id, targets FROM links_grouped;";
    sqlite3_stmt *stmt = nullptr;

    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to begin transaction: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return links;
    }


    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return links;
    }

    int spinnerState = 0;
    long rowCount = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int source = sqlite3_column_int64(stmt, 0);
        (*links).try_emplace(source);

        const unsigned char* targetsText = sqlite3_column_text(stmt, 1);
        if (!targetsText) continue;

        std::vector<long> targets;
        parseTargets(reinterpret_cast<const char*>(targetsText),targets);

        for(auto t : targets) {
            (*links)[t].push_back(source);
        }
        
        rowCount++;
        if (rowCount % 100000 == 0) {
            spinner(spinnerState);
        }

    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Error reading rows: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);

    // commit
    rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to commit transaction: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    }

    std::cout<<"\rFinished Loading links"<<std::endl;

    return links;
}


