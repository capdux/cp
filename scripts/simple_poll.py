import os
import time

JOURNAL_PATH = "/root/.agy/comm/cp-bun-capsurf.jsonl"
last_mtime = 0

print(f"[POLLER] Started polling {JOURNAL_PATH}")

while True:
    try:
        current_mtime = os.path.getmtime(JOURNAL_PATH)
        if current_mtime > last_mtime:
            with open(JOURNAL_PATH, "r") as f:
                lines = f.readlines()
                if lines:
                    print(f"[NEW_MSG] {lines[-1].strip()}")
            last_mtime = current_mtime
    except Exception:
        pass
    time.sleep(1)
