-- delete pages with redirects but no actual redirect 
BEGIN TRANSACTION;
DELETE FROM pages
WHERE is_redirect = 1
AND NOT EXISTS (
	SELECT 1 FROM redirects r WHERE r.source_id = pages.page_id);
COMMIT;

-- delete unneeded tables
DROP TABLE temp_links;
DROP TABLE redirects;
DROP TABLE linktarget;

-- reduce the size now that tables are deleted
VACUUM;

CREATE INDEX IF NOT EXISTS idx_pages_id ON pages(page_id);
CREATE INDEX IF NOT EXISTS idx_links_grouped_source ON links_grouped(source_id);

-- create virtual table for searching
CREATE VIRTUAL TABLE page_titles USING fts5(page_id UNINDEXED, title);
INSERT INTO page_titles(page_id, title) SELECT page_id, title FROM pages;
