CREATE TABLE IF NOT EXISTS links (
    from_id INTEGER,
    target_id INTEGER,
    PRIMARY KEY (from_id, target_id)
);
.mode csv
.separator ,
.import links.csv links