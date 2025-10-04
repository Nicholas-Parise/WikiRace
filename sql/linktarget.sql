CREATE TABLE IF NOT EXISTS linktarget (
	id INTEGER PRIMARY KEY,
	page_name TEXT NOT NULL
);
.mode csv
.separator "\t"
.import linktarget.tsv linktarget