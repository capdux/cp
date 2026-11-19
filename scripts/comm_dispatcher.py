import time
import os
import subprocess
import json

JOURNAL_PATH = "/root/.agy/comm/cp-bun-capsurf.jsonl"

def main():
    print("[DISPATCHER] Communication Bus initialized. Monitoring journal...")
    # 使用 inotifywait 阻塞式监听
    cmd = ["inotifywait", "-e", "modify", "-m", JOURNAL_PATH]
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    with open(JOURNAL_PATH, "r") as f:
        f.seek(0, 2)
        while True:
            line = f.readline()
            if not line:
                time.sleep(0.1)
                continue
            
            try:
                event = json.loads(line)
                recipient = event.get("recipient")
                
                # 直接将消息推送至对应 Agent 的信箱
                if recipient in ["bun_agy", "capsurf_agy"]:
                    print(f"[DISPATCHER] Forwarding to {recipient}: {event}")
                    # 使用系统级命令发送消息，绕过 Agent 内部轮询
                    subprocess.run(["agy", "send", "--target", recipient, "--message", json.dumps(event)])
                elif event.get("broadcast", False):
                    subprocess.run(["agy", "send", "--target", "bun_agy", "--message", json.dumps(event)])
                    subprocess.run(["agy", "send", "--target", "capsurf_agy", "--message", json.dumps(event)])
            except:
                continue

if __name__ == "__main__":
    main()
