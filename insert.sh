time pigz -dc links.csv.gz | sqlite3 wikipedia.sqlite ".read ./sql/links.sql"
time pigz -dc pages.tsv.gz | sqlite3 wikipedia.sqlite ".read ./sql/pages.sql"
