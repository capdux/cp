# Bench 1 (Python3): sqlite3 :memory: 100,000 batch inserts + 100,000 primary-key point lookups
import json
import sqlite3
import time

ROWS = 100000
LOOKUPS = 100000

conn = sqlite3.connect(":memory:")
cur = conn.cursor()
cur.execute("CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT, score INTEGER);")

payload = "agent_item_payload"

t0 = time.perf_counter_ns()
cur.execute("BEGIN;")
cur.executemany(
    "INSERT INTO items (id, name, score) VALUES (?, ?, ?);",
    ((i, payload, i * 7) for i in range(1, ROWS + 1)),
)
conn.commit()
t1 = time.perf_counter_ns()

checksum = 0
for k in range(LOOKUPS):
    target_id = ((k * 7919) % ROWS) + 1
    row = cur.execute("SELECT score, name FROM items WHERE id = ?;", (target_id,)).fetchone()
    if row is not None:
        checksum += row[0] + len(row[1])
t2 = time.perf_counter_ns()
conn.close()

write_ms = (t1 - t0) / 1e6
read_ms = (t2 - t1) / 1e6
total_ms = write_ms + read_ms

print(json.dumps({
    "write_ms": round(write_ms, 3),
    "read_ms": round(read_ms, 3),
    "total_ms": round(total_ms, 3),
    "checksum": checksum,
}))
