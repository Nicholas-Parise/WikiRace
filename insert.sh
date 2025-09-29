gzip -d links.csv.gz
gzip -d pages.tsv.gz
gzip -d redirects.tsv.gz
sqlite3 wikipedia.sqlite ".read ./sql/links.sql"
sqlite3 wikipedia.sqlite ".read ./sql/pages.sql"
sqlite3 wikipedia.sqlite ".read ./sql/temp_redirects.sql"
sqlite3 wikipedia.sqlite ".read ./sql/redirects.sql"
sqlite3 wikipedia.sqlite ".read ./sql/cleanup.sql"