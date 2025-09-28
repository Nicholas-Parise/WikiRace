time gzip -dc links.csv.gz
time gzip -dc pages.tsv.gz
sqlite3 wikipedia.sqlite ".read ./sql/links.sql"
sqlite3 wikipedia.sqlite ".read ./sql/pages.sql"