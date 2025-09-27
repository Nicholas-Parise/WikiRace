#!/bin/bash

LINKS_FILENAME="enwiki-latest-pagelinks.sql.gz"

#python3 parse.py

echo "Trimming Links File"
# remove lines not starting with INSERT INTO
# split multiple inserts into single inserts with one line each ),( -> )\n(
# only keep links with namespace 0 ..,0,..
# now delete ,0, -> ,
# so went from INSERT INTO .. (151,0,1535) - > 151,1535\n
time pigz  -dc $LINKS_FILENAME \
	| sed -E 's/^INSERT INTO `pagelinks` VALUES //' \
    | sed -E 's/^\(//; s/\);?$//' \
    | sed 's/),(/\'$'\n/g' \
    | awk -F',' '$2 == 0 {print $1 "," $3}' \
    | pigz --fast > links.csv.gz.tmp
mv links.csv.gz.tmp links.csv.gz


#gzip -dc enwiki-latest-pagelinks.sql.gz | grep "INSERT INTO" | sed 's/INSERT INTO `pagelinks`/INSERT OR IGNORE INTO pagelinks/g' | sqlite3 wikipedia.db