-- set links to point to actual destination 	
UPDATE OR IGNORE links
SET target_id = (
    SELECT target_id
    FROM redirects r
    WHERE r.source_id = links.target_id
)
WHERE target_id IN (SELECT source_id FROM redirects);


CREATE TABLE IF NOT EXISTS links_grouped (
    source_id INTEGER PRIMARY KEY,
    targets TEXT
);

INSERT INTO links_grouped (source_id, targets)
SELECT source_id,
       GROUP_CONCAT(target_id, ' ') AS targets
FROM links
GROUP BY source_id;
