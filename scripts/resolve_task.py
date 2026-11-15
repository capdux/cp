#!/usr/bin/env python3
"""
Convenience wrapper to mark one or more tasks as resolved or broadcast a syntax update.
Calls task_queue.py resolve with atomic Maildir-style movement and per-AGY routing.
"""
import sys
import os

script_dir = os.path.dirname(os.path.abspath(__file__))
task_queue_script = os.path.join(script_dir, "task_queue.py")

args = sys.argv[1:]
if not args:
    print(
        "Usage:\n"
        "  python3 scripts/resolve_task.py <task_id> [task_id2 ...] [--note 'note'] [--broadcast]\n"
        "  python3 scripts/task_queue.py broadcast --id <syntax-id> --note 'syntax update note'",
        file=sys.stderr,
    )
    sys.exit(1)

cmd = [sys.executable, task_queue_script, "resolve"] + args
os.execv(sys.executable, cmd)
