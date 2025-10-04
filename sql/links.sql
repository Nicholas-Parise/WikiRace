CREATE TABLE IF NOT EXISTS links (
	source_id INTEGER,
	target_id INTEGER,
	PRIMARY KEY (source_id, target_id)
);

INSERT INTO links (source_id, target_id)
SELECT tl.from_id, p.page_id
FROM temp_links tl
JOIN linktarget lt ON tl.target_id = lt.id
JOIN pages p ON lt.page_name = p.title;