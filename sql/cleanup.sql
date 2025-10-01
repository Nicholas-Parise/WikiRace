CREATE INDEX IF NOT EXISTS idx_pages_id ON pages(page_id);

-- delete pages with redirects but no actual redirect 
BEGIN TRANSACTION;
DELETE FROM pages
WHERE is_redirect = 1
AND NOT EXISTS (
	SELECT 1 FROM redirects r WHERE r.source_id = pages.page_id);
COMMIT;
	
-- set links to point to actual destination 	
-- first run
UPDATE OR IGNORE links
SET target_id = (
    SELECT target_id
    FROM redirects r
    WHERE r.source_id = links.target_id
)
WHERE target_id IN (SELECT source_id FROM redirects);

-- second run
UPDATE OR IGNORE links
SET target_id = (
    SELECT target_id
    FROM redirects r
    WHERE r.source_id = links.target_id
)
WHERE target_id IN (SELECT source_id FROM redirects);


-- clean links with no page
BEGIN TRANSACTION;
DELETE FROM links
WHERE NOT EXISTS (SELECT 1 FROM pages p WHERE p.page_id = links.from_id)
   OR NOT EXISTS (SELECT 1 FROM pages p WHERE p.page_id = links.target_id);
COMMIT;

VACUUM;

CREATE INDEX IF NOT EXISTS idx_links_from ON links(from_id);
CREATE INDEX IF NOT EXISTS idx_links_target ON links(target_id);

-- create virtual table for searching
-- CREATE VIRTUAL TABLE page_titles USING fts5(title);
-- INSERT INTO page_titles (title) SELECT title FROM pages;
CREATE VIRTUAL TABLE page_titles USING fts5(page_id UNINDEXED, title);
INSERT INTO page_titles(page_id, title) SELECT page_id, title FROM pages;
