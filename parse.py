import gzip
import sqlite3
import re

page_sql_gz = "enwiki-latest-page.sql.gz"
pagelinks_sql_gz = "enwiki-latest-pagelinks.sql.gz"
db_file = "wikipedia.db"
batch_size = 500000

insert_re = re.compile(r"INSERT INTO `\w+` VALUES (.+);")

# Regex for: (number, number, 'string'
page_pattern = re.compile(r"\((\d+),(\d+),'((?:\\'|[^'])*)'")
# Regex for: (number, number, 'number'
pagelinks_pattern = re.compile(r"\((\d+),(\d+),(\d+)")

def escape(s: str) -> str:
    """Convert MySQL-escaped to sqlite"""
    return (
        s.replace(r"\\'", "'")   # \' -> '
         .replace(r"\\", "\\")   # \\ -> \
    )

def parse_page_values(values_str):
    """Extract only page_id, page_namespace, page_title"""
    records = []
    #print(values_str)
    for match in page_pattern.finditer(values_str):
        page_id = int(match.group(1))
        page_namespace = int(match.group(2))
        page_title = escape(match.group(3))
        #print(match)
        #print(page_id, page_namespace, page_title)
        records.append((page_id, page_namespace, page_title))
    return records

def parse_pagelinks_values(values_str):
    """Extract pl_from, pl_from_namespace, pl_target_id """
    records = []
    #print(values_str)
    for match in pagelinks_pattern.finditer(values_str):
        pl_from = int(match.group(1))
        pl_from_namespace = int(match.group(2))
        pl_target_id = int(match.group(3))
        # Only namespace 0 (articles)
        if pl_from_namespace != 0:
            continue
        #print(match)
        #print(pl_from, pl_from_namespace, pl_target_id)
        records.append((pl_from, pl_from_namespace, pl_target_id))
    return records

conn = sqlite3.connect(db_file)
c = conn.cursor()

# tables
c.execute("""
CREATE TABLE IF NOT EXISTS page (
    page_id INTEGER PRIMARY KEY,
    page_namespace INTEGER,
    page_title TEXT
)
""")
c.execute("""
CREATE TABLE IF NOT EXISTS pagelinks (
    pl_from INTEGER,
    pl_from_namespace INTEGER,
    pl_target_id INTEGER,
    PRIMARY KEY (pl_from, pl_target_id)
)
""")
conn.commit()

'''
print("Loading pages")
with gzip.open(page_sql_gz, "rt", encoding="utf-8", errors="ignore") as f:
    batch = []
    for line in f:
        
        m = insert_re.search(line)
        if m:
            values_str = m.group(1)
            batch.extend(parse_page_values(values_str))
            
            if len(batch) >= batch_size:
                c.executemany("INSERT OR IGNORE INTO page (page_id, page_namespace, page_title) VALUES (?, ?, ?)", batch)
                conn.commit()
                batch = []
                print("Inserted ",batch_size," rows")
    if batch:
        c.executemany("INSERT OR IGNORE INTO page (page_id, page_namespace, page_title) VALUES (?, ?, ?)", batch)
        conn.commit()
'''
print("Loading pagelinks...")
with gzip.open(pagelinks_sql_gz, "rt", encoding="utf-8", errors="ignore") as f:
    batch = []
    for line in f:
        print(line)
        m = insert_re.search(line)
        if m:
            values_str = m.group(1)
            #batch.extend(parse_pagelinks_values(values_str))
            if len(batch) >= batch_size:
                c.executemany("INSERT OR IGNORE INTO pagelinks (pl_from, pl_from_namespace, pl_target_id) VALUES (?, ?, ?)", batch)                
                conn.commit()
                batch = []
                print("Inserted ",batch_size," rows")
                
    if batch:
        c.executemany("INSERT OR IGNORE INTO pagelinks (pl_from, pl_from_namespace, pl_target_id) VALUES (?, ?, ?)", batch)
        conn.commit()

c.execute("CREATE INDEX IF NOT EXISTS idx_page_id ON page(page_id)")
c.execute("CREATE INDEX IF NOT EXISTS idx_pagelinks_from ON pagelinks(pl_from)")
conn.commit()

conn.close()
print("Done. SQLite DB ready:", db_file)
