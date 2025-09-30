mkdir -p build
gcc -c src/sqlite3.c -DSQLITE_ENABLE_FTS5 -Iinclude -o build/sqlite3.o
g++ -Iinclude src/*.cpp build/sqlite3.o -o build/wikiRace.exe -Wall -O2