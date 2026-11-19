import os
import time
import json

JOURNAL_PATH = "/root/.agy/comm/cp-bun-capsurf.jsonl"
# 每个项目下该脚本硬编码自己的身份
MY_ID = "cp_agy" 
last_mtime = 0
last_size = 0

print(f"[WATCHER] Starting polling on {JOURNAL_PATH} for {MY_ID}...")

while True:
    try:
        if os.path.exists(JOURNAL_PATH):
            stat = os.stat(JOURNAL_PATH)
            if stat.st_mtime > last_mtime or stat.st_size != last_size:
                with open(JOURNAL_PATH, "r") as f:
                    f.seek(last_size)
                    new_data = f.read()
                    if new_data:
                        for line in new_data.strip().split('\n'):
                            if line.strip():
                                try:
                                    msg = json.loads(line)
                                    recipient = msg.get("recipient")
                                    broadcast = msg.get("broadcast", False)
                                    if recipient == MY_ID or broadcast:
                                        print(f"[NEW_MSG] {msg}")
                                except json.JSONDecodeError:
                                    pass
                last_mtime = stat.st_mtime
                last_size = stat.st_size
    except Exception:
        pass
    time.sleep(5)
