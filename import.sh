#!/bin/bash
python3 parse.py

gzip -dc enwiki-latest-pagelinks.sql.gz | grep "INSERT INTO" | sed 's/INSERT INTO `pagelinks`/INSERT OR IGNORE INTO pagelinks/g' | sqlite3 wikipedia.db
