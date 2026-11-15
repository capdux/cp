#!/usr/bin/env python3
"""
Task Queue Watcher for Antigravity Multi-AGY inter-agent communication (cp_agy <-> bun_agy & capsurf_agy).

Key Reliability Guarantees:
1. Per-Agent Isolation & Broadcast Fan-out:
   - Downstream AGY (`bun_agy` or `capsurf_agy`) only wakes up on tasks created by itself OR syntax updates
     broadcast by `cp_agy` (`broadcast: true` / `syntax-*`).
   - Upstream `cp_agy` (`--status pending`) automatically listens to BOTH `bun_agy` and `capsurf_agy`.
2. Persistent Disk Ack Watermark (`/data/dev/cp/.tasks/.ack_<agent>.json`):
   - Eliminates the old `initial_resolved = set(os.listdir(...))` startup race condition.
   - Any task resolved while `watch_task.py` was not running (e.g. while the agent was busy handling an earlier
     wakeup) remains unacked on disk and triggers an immediate wakeup (0s delay) the moment `watch_task.py` restarts.
3. Micro-Batch Coalescing (200ms) + Batch Drain:
   - When multiple tasks are resolved in the same second (e.g. `resolve t1 && resolve t2 && resolve t3 && resolve t4`),
     waits a short 200ms coalescing window, drains ALL ready tasks at once, updates `.ack_<agent>.json` atomically,
     and prints all N tasks in a single wakeup so nothing is ever lost or requires artificial `sleep`.
"""
import sys
import time
import os
import json
import argparse

BASE_DIR = "/data/dev/cp/.tasks"
PENDING_DIR = os.path.join(BASE_DIR, "pending")
RESOLVED_DIR = os.path.join(BASE_DIR, "resolved")


def infer_default_agent() -> str:
    hint = (os.path.abspath(__file__) + " " + os.getcwd()).lower()
    if "/capsurf" in hint:
        return "capsurf_agy"
    if "/bun" in hint:
        return "bun_agy"
    return "cp_agy"


DEFAULT_AGENT = infer_default_agent()


def get_ack_file(agent_name: str) -> str:
    return os.path.join(BASE_DIR, f".ack_{agent_name}.json")


def load_acks(agent_name: str) -> dict:
    ack_file = get_ack_file(agent_name)
    if not os.path.exists(ack_file):
        return None
    try:
        with open(ack_file, "r", encoding="utf-8") as f:
            data = json.load(f)
            return data if isinstance(data, dict) else {}
    except Exception:
        return {}


def save_acks(agent_name: str, acks: dict):
    os.makedirs(BASE_DIR, exist_ok=True)
    ack_file = get_ack_file(agent_name)
    tmp_file = f"{ack_file}.tmp.{os.getpid()}"
    try:
        with open(tmp_file, "w", encoding="utf-8") as f:
            json.dump(acks, f, ensure_ascii=False, indent=2)
            f.flush()
            os.fsync(f.fileno())
        os.rename(tmp_file, ack_file)
    except Exception:
        pass


def is_targeted_for_agent(task: dict, agent_name: str) -> bool:
    """
    Determine whether a resolved task or syntax broadcast belongs to `agent_name`:
    1. Broadcast notices (`broadcast: true` or `category == "syntax_update"` or `id` starting with `syntax-`)
       notify both `bun_agy` and `capsurf_agy`.
    2. Explicit `target_agy` list/string containing `agent_name` or `"all"`.
    3. Tasks created by `agent_name` (`created_by == agent_name`).
    4. Tasks with `capsurf` in ID when `agent_name == "capsurf_agy"`.
    """
    if not agent_name or agent_name in ("all", "any", "*"):
        return True
    if task.get("broadcast") is True or task.get("category") == "syntax_update":
        targets = task.get("target_agy")
        if not targets or targets == "all" or agent_name in targets:
            return True
    tid = task.get("id", "")
    if tid.startswith("syntax-"):
        return True
    targets = task.get("target_agy")
    if isinstance(targets, list):
        if "all" in targets or agent_name in targets:
            return True
        return False
    if isinstance(targets, str):
        if targets in ("all", agent_name):
            return True
        return False
    creator = task.get("created_by", "")
    if creator == agent_name:
        return True
    if agent_name == "capsurf_agy" and "capsurf" in tid:
        return True
    return False


def scan_resolved_candidates(agent_name: str) -> list:
    """Return sorted list of (tid, task_dict) from both resolved/<agent_name>/ and resolved/."""
    candidates = {}
    search_dirs = []
    if agent_name and agent_name not in ("all", "any", "*"):
        ag_dir = os.path.join(RESOLVED_DIR, agent_name)
        if os.path.exists(ag_dir):
            search_dirs.append(ag_dir)
    if os.path.exists(RESOLVED_DIR):
        search_dirs.append(RESOLVED_DIR)

    for d in search_dirs:
        try:
            files = sorted([f for f in os.listdir(d) if f.endswith(".json") and not f.startswith(".")])
        except Exception:
            continue
        for fname in files:
            tid = fname[:-5]
            if tid in candidates:
                continue
            fpath = os.path.join(d, fname)
            if not os.path.isfile(fpath):
                continue
            try:
                with open(fpath, "r", encoding="utf-8") as f:
                    task = json.load(f)
                if task.get("status") == "resolved":
                    candidates[tid] = task
            except (json.JSONDecodeError, OSError):
                continue
    return sorted(candidates.items(), key=lambda x: x[0])


def init_acks_if_needed(agent_name: str) -> dict:
    acks = load_acks(agent_name)
    if acks is not None:
        return acks
    # 首次初始化（仅当磁盘上尚无 .ack_<agent>.json 时执行一次）：
    # 将当前已存在的历史归档标记为已读，后续进程重启均直接复用磁盘游标，绝不丢失重启间隙的新结单
    acks = {}
    for tid, task in scan_resolved_candidates(agent_name):
        if is_targeted_for_agent(task, agent_name):
            acks[tid] = task.get("resolved_at") or task.get("timestamp") or "init"
    save_acks(agent_name, acks)
    return acks


def check_pending_batch(task_id_filter=None, creator_filter=None, strict_creator=False) -> list:
    if not os.path.exists(PENDING_DIR):
        return []
    allowed_creators = None
    if creator_filter and creator_filter.lower() not in ("all", "any", "*"):
        raw_set = set(c.strip() for c in creator_filter.split(",") if c.strip())
        # 兼容 cp_agy 历史命令行 `--creator bun_agy`：非严格模式下自动同时监听 bun_agy 与 capsurf_agy
        if not strict_creator and raw_set == {"bun_agy"}:
            allowed_creators = {"bun_agy", "capsurf_agy"}
        else:
            allowed_creators = raw_set

    matched = []
    try:
        files = sorted([f for f in os.listdir(PENDING_DIR) if f.endswith(".json") and not f.startswith(".")])
        for fname in files:
            tid = fname[:-5]
            if task_id_filter and tid != task_id_filter:
                continue
            fpath = os.path.join(PENDING_DIR, fname)
            try:
                with open(fpath, "r", encoding="utf-8") as f:
                    task = json.load(f)
                if task.get("status") == "pending":
                    creator = task.get("created_by", "")
                    if allowed_creators and creator not in allowed_creators:
                        continue
                    matched.append(task)
            except (json.JSONDecodeError, OSError):
                continue
    except Exception:
        pass
    return matched


def check_resolved_batch(task_id_filter: str, agent_name: str, acks: dict, commit_ack: bool = False) -> list:
    matched = []
    for tid, task in scan_resolved_candidates(agent_name):
        if task_id_filter:
            if tid == task_id_filter:
                matched.append(task)
            continue
        if not is_targeted_for_agent(task, agent_name):
            continue
        ver = task.get("resolved_at") or task.get("timestamp") or "resolved"
        if acks.get(tid) == ver:
            continue
        matched.append(task)
        if commit_ack:
            acks[tid] = ver

    if matched and commit_ack and not task_id_filter:
        save_acks(agent_name, acks)
    return matched


def main():
    parser = argparse.ArgumentParser(description="Watch task queue for pending or resolved tasks (with Batch Drain)")
    parser.add_argument(
        "--status",
        default="pending",
        choices=["pending", "resolved"],
        help="Task status to wait for (default: pending)",
    )
    parser.add_argument("--id", default=None, help="Specific task ID to wait for")
    parser.add_argument(
        "--agent",
        default=DEFAULT_AGENT,
        help=f"Downstream agent identity when watching resolved tasks (default auto-detected: {DEFAULT_AGENT})",
    )
    parser.add_argument(
        "--creator",
        default="all",
        help="Filter pending tasks by creator ID(s), comma-separated or 'all' (default: all -> bun_agy,capsurf_agy)",
    )
    parser.add_argument(
        "--strict-creator",
        action="store_true",
        help="Strictly filter by --creator without auto-including capsurf_agy when --creator bun_agy is passed",
    )
    parser.add_argument(
        "--coalesce",
        type=float,
        default=0.2,
        help="Micro-batch coalescing window in seconds to group simultaneous multi-task resolves (default: 0.2s)",
    )
    parser.add_argument("--interval", type=float, default=1.0, help="Poll interval in seconds (default: 1.0)")
    parser.add_argument("--timeout", type=int, default=0, help="Timeout in seconds (0 = infinite)")
    args = parser.parse_args()

    acks = {}
    if args.status == "resolved":
        acks = init_acks_if_needed(args.agent)
        print(
            f"[WATCHER] Listening for RESOLVED tasks & syntax broadcasts (agent={args.agent}, id={args.id or 'ANY'})...",
            flush=True,
        )
    else:
        print(
            f"[WATCHER] Listening for PENDING tasks (creator={args.creator}, id={args.id or 'ANY'})...",
            flush=True,
        )

    start_time = time.time()
    while True:
        if args.status == "pending":
            probe = check_pending_batch(args.id, args.creator, args.strict_creator)
            if probe:
                if args.coalesce > 0 and not args.id:
                    time.sleep(args.coalesce)
                tasks = check_pending_batch(args.id, args.creator, args.strict_creator)
            else:
                tasks = []
        else:
            probe = check_resolved_batch(args.id, args.agent, acks, commit_ack=False)
            if probe:
                if args.coalesce > 0 and not args.id:
                    time.sleep(args.coalesce)
                tasks = check_resolved_batch(args.id, args.agent, acks, commit_ack=True)
            else:
                tasks = []

        if tasks:
            if len(tasks) > 1:
                print(
                    f"[FOUND_TASKS_BATCH] Count: {len(tasks)} tasks ready simultaneously:",
                    flush=True,
                )
            for idx, task in enumerate(tasks, 1):
                task_id = task.get("id", "unknown")
                creator = task.get("created_by", "unknown")
                category = task.get("category", "compiler_bug")
                is_bcast = task.get("broadcast", False)
                repro = task.get("repro", "")
                tag = "SYNTAX_BROADCAST" if (is_bcast or category == "syntax_update") else "FOUND_TASK"
                prefix = f"[{idx}/{len(tasks)}] " if len(tasks) > 1 else ""
                print(
                    f"{prefix}[{tag}] [FROM: {creator}] [TO: {task.get('target_agy', args.agent)}] ID: {task_id} | Repro: {repro}",
                    flush=True,
                )
                print(json.dumps(task, ensure_ascii=False), flush=True)
            sys.exit(0)  # 一次性排空全部就绪任务后退出，触发 Antigravity CLI Reactive Wakeup！

        if args.timeout > 0 and (time.time() - start_time) > args.timeout:
            print(f"[WATCHER_TIMEOUT] Timed out waiting after {args.timeout}s", file=sys.stderr)
            sys.exit(1)

        time.sleep(args.interval)


if __name__ == "__main__":
    main()
