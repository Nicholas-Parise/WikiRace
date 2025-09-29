CREATE TABLE IF NOT EXISTS redirects (
	source_id INTEGER PRIMARY KEY,
	target_id INTEGER NOT NULL
);

INSERT INTO redirects
SELECT t.source_id, p.page_id
FROM temp_redirects t
JOIN pages p ON t.target_name = p.title;

DROP TABLE temp_redirects;