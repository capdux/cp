# Bench 4 (Python3): json.loads — 10,000 parses of medium JSON payload
import json
import os
import time

DIR = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(DIR, "payload.json"), "r", encoding="utf-8") as f:
    payload = f.read()

ITERATIONS = 10_000
checksum = 0

t0 = time.perf_counter_ns()
for _ in range(ITERATIONS):
    root = json.loads(payload)
    checksum += len(root["meta"]["request_id"])
    for item in root["items"]:
        checksum += item["score"]
t1 = time.perf_counter_ns()

parse_ms = (t1 - t0) / 1e6
print(json.dumps({
    "parse_ms": round(parse_ms, 3),
    "iterations": ITERATIONS,
    "payload_bytes": len(payload.encode("utf-8")),
    "checksum": checksum,
}))
