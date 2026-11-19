#!/bin/bash
while true; do
    python3 /data/dev/cp/scripts/simple_poll.py
    echo "[POLLER] Poller exited, restarting..."
    sleep 1
done
