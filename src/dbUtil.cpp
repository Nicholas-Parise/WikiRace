#include "dbUtil.h"
#include <iostream>

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

// spinner loading bar
void dbUtil::spinner(int &state) {
    const char symbols[] = {'|', '/', '-', '\\'};
    std::cout << "\rLoading links " << symbols[state % 4] << std::flush;
    state++;
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