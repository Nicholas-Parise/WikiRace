CREATE TABLE IF NOT EXISTS pages (
    page_id INTEGER PRIMARY KEY,
    title TEXT,
    is_redirect INTEGER
);
.mode csv
.separator "\t"
.import pages.tsv pages