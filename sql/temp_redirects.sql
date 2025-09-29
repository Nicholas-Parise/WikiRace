CREATE TABLE IF NOT EXISTS temp_redirects (
	source_id INTEGER PRIMARY KEY,
	target_name TEXT NOT NULL
);
.mode csv
.separator "\t"
.import redirects.tsv temp_redirects