mkdir -p build
gcc -c src/sqlite3.c -Iinclude -o build/sqlite3.o
g++ -Iinclude src/*.cpp build/sqlite3.o -o build/wikiRace.exe -Wall -O2