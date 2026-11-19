import json
import time
import os

JOURNAL_PATH = "/root/.agy/comm/cp-bun-capsurf.jsonl"

def main():
    print(f"[WATCHER] Starting journal watch on {JOURNAL_PATH}...")
    # Open and seek to end
    with open(JOURNAL_PATH, "r") as f:
        f.seek(0, os.SEEK_END)
        while True:
            line = f.readline()
            if not line:
                time.sleep(1)
                continue
            
            try:
                event = json.loads(line)
                print(f"[EVENT] {event}")
                # Trigger logic based on event type
                if event.get("status") == "resolved":
                    print(f"[ACTION] Task {event['id']} resolved. Notifying agent...")
            except json.JSONDecodeError:
                pass

if __name__ == "__main__":
    main()
