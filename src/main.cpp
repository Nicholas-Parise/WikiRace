#include <iostream>
#include "sqlite3.h"

int main(){

    sqlite3 *db;
    int rc = sqlite3_open("wikipedia.sqlite", &db);
    
    if (rc){
        std::cerr<<"Can't open database: "<<sqlite3_errmsg(db)<<std::endl;
        return 1;
    }
    else{
        std::cout<<"Opened database successfully"<<std::endl;
    }






}