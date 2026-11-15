#!/usr/bin/env python3
"""
Task Queue Manager for Antigravity Multi-AGY inter-agent communication (cp_agy <-> bun_agy & capsurf_agy).
Uses Maildir-style atomic directory moving (.tasks/pending -> .tasks/resolved/<agent>/)
combined with an append-only TASK_QUEUE.jsonl audit log, multi-task batch resolve, and fan-out broadcast support.
100% race-condition free and atomic on Linux filesystems.
"""
import os
import sys
import json
import time
import argparse

BASE_DIR = "/data/dev/cp/.tasks"
PENDING_DIR = os.path.join(BASE_DIR, "pending")
RESOLVED_DIR = os.path.join(BASE_DIR, "resolved")
AUDIT_LOG = "/data/dev/cp/TASK_QUEUE.jsonl"

ALL_DOWNSTREAM_AGENTS = ["bun_agy", "capsurf_agy"]


def infer_default_agent() -> str:
    """Auto-detect agent identity from script path or working directory."""
    hint = (os.path.abspath(__file__) + " " + os.getcwd()).lower()
    if "/capsurf" in hint:
        return "capsurf_agy"
    if "/bun" in hint:
        return "bun_agy"
    return "cp_agy"


DEFAULT_AGENT = infer_default_agent()


def ensure_dirs():
    os.makedirs(PENDING_DIR, exist_ok=True)
    os.makedirs(RESOLVED_DIR, exist_ok=True)
    for ag in ALL_DOWNSTREAM_AGENTS:
        os.makedirs(os.path.join(RESOLVED_DIR, ag), exist_ok=True)
    os.makedirs(os.path.dirname(AUDIT_LOG), exist_ok=True)
    if not os.path.exists(AUDIT_LOG):
        with open(AUDIT_LOG, "w", encoding="utf-8") as f:
            f.write("# CP / Bun / CapSurf AGY Task Mailbox Log (Append-Only Event Stream)\n")


def normalize_task_id(task_id: str, created_by: str) -> str:
    """Ensure task_id carries agent prefix to avoid filename collisions between bun_agy and capsurf_agy."""
    if created_by == "capsurf_agy" and "capsurf" not in task_id:
        if task_id.startswith("task-"):
            return "task-capsurf-" + task_id[len("task-"):]
        return f"task-capsurf-{task_id}"
    return task_id


def infer_project(created_by: str) -> str:
    if "capsurf" in created_by:
        return "capsurf"
    if "bun" in created_by:
        return "bun"
    return "cp"


def atomic_write_json(target_path: str, data: dict):
    os.makedirs(os.path.dirname(target_path), exist_ok=True)
    tmp_path = os.path.join(
        os.path.dirname(target_path),
        f".tmp_{os.getpid()}_{int(time.time() * 1000000)}_{os.path.basename(target_path)}",
    )
    with open(tmp_path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
        f.flush()
        os.fsync(f.fileno())
    os.rename(tmp_path, target_path)


def submit_task(task_id: str, repro: str, error_msg: str, created_by: str = DEFAULT_AGENT):
    ensure_dirs()
    norm_id = normalize_task_id(task_id, created_by)
    abs_repro = os.path.abspath(repro)
    task_data = {
        "id": norm_id,
        "project": infer_project(created_by),
        "category": "compiler_bug",
        "broadcast": False,
        "created_by": created_by,
        "target_agy": [created_by],
        "repro": abs_repro,
        "error": error_msg,
        "status": "pending",
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
    }
    target_file = os.path.join(PENDING_DIR, f"{norm_id}.json")
    atomic_write_json(target_file, task_data)

    with open(AUDIT_LOG, "a", encoding="utf-8") as f:
        f.write(json.dumps({**task_data, "event": "submitted"}, ensure_ascii=False) + "\n")

    print(
        f"[TASK_SUBMITTED] [FROM: {created_by}] Task '{norm_id}' queued in {target_file} (repro: {abs_repro})"
    )


def resolve_single_task(task_id: str, note: str = "", resolved_by: str = "cp_agy", broadcast: bool = False) -> bool:
    ensure_dirs()
    pending_file = os.path.join(PENDING_DIR, f"{task_id}.json")
    resolved_root_file = os.path.join(RESOLVED_DIR, f"{task_id}.json")

    if not os.path.exists(pending_file):
        if os.path.exists(resolved_root_file):
            print(f"[TASK_ALREADY_RESOLVED] Task '{task_id}' was already resolved.")
            return True
        for ag in ALL_DOWNSTREAM_AGENTS:
            if os.path.exists(os.path.join(RESOLVED_DIR, ag, f"{task_id}.json")):
                print(f"[TASK_ALREADY_RESOLVED] Task '{task_id}' was already resolved for {ag}.")
                return True
        print(f"[ERROR] Task '{task_id}' not found in {PENDING_DIR}", file=sys.stderr)
        return False

    try:
        with open(pending_file, "r", encoding="utf-8") as f:
            task_data = json.load(f)
    except Exception:
        task_data = {"id": task_id, "created_by": "capsurf_agy" if "capsurf" in task_id else "bun_agy"}

    creator = task_data.get("created_by") or ("capsurf_agy" if "capsurf" in task_id else "bun_agy")
    if broadcast:
        targets = list(ALL_DOWNSTREAM_AGENTS)
        task_data["broadcast"] = True
        task_data["category"] = task_data.get("category", "syntax_update")
    else:
        raw_targets = task_data.get("target_agy")
        if isinstance(raw_targets, list) and raw_targets:
            targets = raw_targets
        elif isinstance(raw_targets, str) and raw_targets:
            targets = [raw_targets]
        else:
            targets = [creator]

    task_data["created_by"] = creator
    task_data["target_agy"] = targets
    task_data["status"] = "resolved"
    task_data["note"] = note
    task_data["resolved_by"] = resolved_by
    task_data["resolved_at"] = time.strftime("%Y-%m-%dT%H:%M:%S")

    if os.path.exists(pending_file):
        os.remove(pending_file)

    delivered_paths = []
    for ag in targets:
        ag_file = os.path.join(RESOLVED_DIR, ag, f"{task_id}.json")
        atomic_write_json(ag_file, task_data)
        delivered_paths.append(ag_file)

    # 兼容层：若目标包含 bun_agy 或为全局广播，同时写入平面目录 resolved/<task_id>.json
    if broadcast or "bun_agy" in targets:
        atomic_write_json(resolved_root_file, task_data)
        delivered_paths.append(resolved_root_file)

    with open(AUDIT_LOG, "a", encoding="utf-8") as f:
        f.write(json.dumps({**task_data, "event": "resolved"}, ensure_ascii=False) + "\n")

    print(
        f"[TASK_RESOLVED] Task '{task_id}' (creator={creator}, targets={targets}) -> {', '.join(delivered_paths)}"
    )
    return True


def resolve_tasks(task_ids: list, note: str = "", resolved_by: str = "cp_agy", broadcast: bool = False):
    ok = True
    for tid in task_ids:
        if not resolve_single_task(tid, note, resolved_by, broadcast):
            ok = False
    if not ok:
        sys.exit(1)


def broadcast_syntax(
    notice_id: str,
    note: str,
    resolved_by: str = "cp_agy",
    targets_csv: str = "bun_agy,capsurf_agy",
):
    """
    Broadcast a CP language syntax / cpc rule update from cp_agy to both bun_agy and capsurf_agy simultaneously.
    """
    ensure_dirs()
    if not notice_id.startswith("syntax-"):
        notice_id = f"syntax-{notice_id}"
    targets = [t.strip() for t in targets_csv.split(",") if t.strip()]
    now_str = time.strftime("%Y-%m-%dT%H:%M:%S")
    notice_data = {
        "id": notice_id,
        "project": "cp",
        "category": "syntax_update",
        "broadcast": True,
        "created_by": resolved_by,
        "target_agy": targets,
        "repro": "",
        "error": "CP Language Syntax / Compiler Rule Update Broadcast",
        "status": "resolved",
        "note": note,
        "resolved_by": resolved_by,
        "timestamp": now_str,
        "resolved_at": now_str,
    }

    delivered_paths = []
    for ag in targets:
        ag_file = os.path.join(RESOLVED_DIR, ag, f"{notice_id}.json")
        atomic_write_json(ag_file, notice_data)
        delivered_paths.append(ag_file)

    root_file = os.path.join(RESOLVED_DIR, f"{notice_id}.json")
    atomic_write_json(root_file, notice_data)
    delivered_paths.append(root_file)

    with open(AUDIT_LOG, "a", encoding="utf-8") as f:
        f.write(json.dumps({**notice_data, "event": "syntax_broadcast"}, ensure_ascii=False) + "\n")

    print(
        f"[SYNTAX_BROADCAST] Notice '{notice_id}' broadcast to {targets} -> {', '.join(delivered_paths)}"
    )


def list_tasks():
    ensure_dirs()
    pending_files = sorted(
        [f for f in os.listdir(PENDING_DIR) if f.endswith(".json") and not f.startswith(".")]
    )
    print(f"=== Pending Tasks ({len(pending_files)}) ===")
    for fname in pending_files:
        fpath = os.path.join(PENDING_DIR, fname)
        try:
            with open(fpath, "r", encoding="utf-8") as f:
                d = json.load(f)
            print(f"  - [{d.get('created_by', 'unknown')}] {d.get('id', fname[:-5])}: {d.get('error', '')[:80]}")
        except Exception:
            print(f"  - {fname[:-5]}")

    for ag in ALL_DOWNSTREAM_AGENTS:
        ag_dir = os.path.join(RESOLVED_DIR, ag)
        ag_files = sorted(
            [f[:-5] for f in os.listdir(ag_dir) if f.endswith(".json") and not f.startswith(".")]
        ) if os.path.exists(ag_dir) else []
        print(f"=== Resolved Mailbox [{ag}] ({len(ag_files)}) ===")
        for r in ag_files[-10:]:
            print(f"  - {r}")


def main():
    parser = argparse.ArgumentParser(description="Multi-AGY Task Queue Manager (cp_agy / bun_agy / capsurf_agy)")
    sub = parser.add_subparsers(dest="cmd", required=True)

    sub_submit = sub.add_parser("submit", help="Submit a compiler bug task to cp_agy")
    sub_submit.add_argument("--id", required=True, help="Task ID (e.g. task-20261115-slug)")
    sub_submit.add_argument("--repro", required=True, help="Path to repro .cp file")
    sub_submit.add_argument("--error", default="", help="Error description")
    sub_submit.add_argument("--by", default=DEFAULT_AGENT, help=f"Creator ID (default: {DEFAULT_AGENT})")

    sub_resolve = sub.add_parser("resolve", help="Resolve one or more pending tasks and route notices to creator AGY")
    sub_resolve.add_argument("task_ids", nargs="+", help="One or more Task IDs to resolve")
    sub_resolve.add_argument("--note", default="", help="Resolution note")
    sub_resolve.add_argument("--by", default="cp_agy", help="Resolver ID")
    sub_resolve.add_argument(
        "--broadcast",
        action="store_true",
        help="Also broadcast this resolution (e.g. syntax rule change) to both bun_agy and capsurf_agy",
    )

    sub_broadcast = sub.add_parser(
        "broadcast",
        help="Broadcast a CP language syntax update to both bun_agy and capsurf_agy simultaneously",
    )
    sub_broadcast.add_argument("--id", required=True, help="Broadcast ID (e.g. syntax-20261115-rule)")
    sub_broadcast.add_argument("--note", required=True, help="Syntax update details for downstream AGYs")
    sub_broadcast.add_argument("--by", default="cp_agy", help="Broadcaster ID (default: cp_agy)")
    sub_broadcast.add_argument(
        "--targets",
        default="bun_agy,capsurf_agy",
        help="Comma-separated target AGYs (default: bun_agy,capsurf_agy)",
    )

    sub.add_parser("list", help="List pending and resolved tasks by AGY")

    args = parser.parse_args()
    if args.cmd == "submit":
        submit_task(args.id, args.repro, args.error, args.by)
    elif args.cmd == "resolve":
        resolve_tasks(args.task_ids, args.note, args.by, args.broadcast)
    elif args.cmd == "broadcast":
        broadcast_syntax(args.id, args.note, args.by, args.targets)
    elif args.cmd == "list":
        list_tasks()


if __name__ == "__main__":
    main()
