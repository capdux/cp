#!/usr/bin/env python3
"""
SESSION.md / SESSION_H.md 全量无损审计与补录脚本（零遗漏铁律守卫）

功能：
1. 扫描 /root/.agy/comm/queue.db 中所有发往 cp_agy（或 bun/capsurf 项目）的下游工单，
   将 SESSION.md + SESSION_H.md 中任何遗漏的工单（含 task_id、来源、状态、报错摘要与结单说明）
   按时间顺序无损补录；
2. 扫描当前会话 transcript_full.jsonl 中的全部用户原始指令（USER_EXPLICIT）及其对应的
   主智能体最终答复，将任何遗漏的原始指令按日期无损补录（历史日期 -> SESSION_H.md，当日 -> SESSION.md）；
3. 幂等运行：已记录的 task_id 与 [step:<N>] 不会重复写入，绝不覆盖或删除任何既有内容。
"""
import argparse
import datetime
import json
import re
import sqlite3
from pathlib import Path

ROOT = Path("/data/dev/cp")
SESSION_HOT = ROOT / "SESSION.md"
SESSION_HIST = ROOT / "SESSION_H.md"
QUEUE_DB = Path("/root/.agy/comm/queue.db")
DEFAULT_TRANSCRIPT = Path(
    "/root/.agy/node1/.gemini/antigravity-cli/brain/"
    "07bf4a4f-b4e5-42a0-a482-ba735a6138e5/.system_generated/logs/transcript_full.jsonl"
)
LOCAL_TZ = datetime.timezone(datetime.timedelta(hours=11))


def to_local_dt(iso_str: str) -> tuple[str, str]:
    """Return (YYYY-MM-DD, YYYY-MM-DD HH:MM:SS) in local +11:00 timezone."""
    if not iso_str:
        now = datetime.datetime.now(LOCAL_TZ)
        return now.strftime("%Y-%m-%d"), now.strftime("%Y-%m-%d %H:%M:%S")
    s = iso_str.strip()
    if s.endswith("Z"):
        s = s[:-1] + "+00:00"
    try:
        dt = datetime.datetime.fromisoformat(s)
        if dt.tzinfo is None:
            # queue.db timestamps are local server time
            dt_local = dt.replace(tzinfo=LOCAL_TZ)
        else:
            dt_local = dt.astimezone(LOCAL_TZ)
        return dt_local.strftime("%Y-%m-%d"), dt_local.strftime("%Y-%m-%d %H:%M:%S")
    except Exception:
        day = s[:10] if len(s) >= 10 else "2026-11-22"
        return day, s[:19].replace("T", " ")


def extract_user_body(raw: str) -> str:
    m = re.search(r"<USER_REQUEST>\s*(.*?)\s*</USER_REQUEST>", raw, re.S)
    return (m.group(1) if m else raw).strip()


def sanitize_block(text: str, max_chars: int | None = None) -> str:
    text = text.strip()
    if max_chars is not None and len(text) > max_chars:
        text = text[:max_chars].rstrip() + " ...(截断)"
    return text


def indent_quote(text: str) -> str:
    lines = text.splitlines() or [""]
    return "\n".join(f"  > {ln}" if ln.strip() else "  >" for ln in lines)


def load_transcript_turns(tpath: Path):
    """Extract chronological (step_index, local_day, local_ts, user_body, reply_summary)."""
    if not tpath.exists():
        return []
    steps = []
    with tpath.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                steps.append(json.loads(line))
            except Exception:
                continue

    turns = []
    for i, obj in enumerate(steps):
        if obj.get("type") == "USER_INPUT" and obj.get("source") == "USER_EXPLICIT":
            step_idx = obj.get("step_index")
            day, ts = to_local_dt(obj.get("created_at", ""))
            body = extract_user_body(obj.get("content", ""))
            if not body:
                continue
            # Find the EXACT response to THIS USER_EXPLICIT turn:
            # Take the FIRST terminal PLANNER_RESPONSE (no tool_calls & non-empty content)
            # before the next USER_EXPLICIT, so later subagent SYSTEM_MESSAGE wakeups
            # never overwrite the user's actual reply.
            reply = ""
            fallback_reply = ""
            for j in range(i + 1, len(steps)):
                nxt = steps[j]
                if nxt.get("type") == "USER_INPUT" and nxt.get("source") == "USER_EXPLICIT":
                    break
                if nxt.get("type") == "PLANNER_RESPONSE":
                    c = (nxt.get("content") or "").strip()
                    has_tools = bool(nxt.get("tool_calls"))
                    if c:
                        if not fallback_reply:
                            fallback_reply = c
                        if not has_tools:
                            reply = c
                            break
            if not reply:
                reply = fallback_reply
            turns.append((step_idx, day, ts, body, reply))
    return turns


def load_queue_tickets(db_path: Path):
    if not db_path.exists():
        return []
    conn = sqlite3.connect(str(db_path))
    cur = conn.cursor()
    cur.execute(
        """
        SELECT task_id, project, category, created_by, status, created_at, resolved_at, error_msg, note
        FROM tasks
        WHERE target_agy_json LIKE '%cp_agy%' OR project IN ('bun', 'capsurf')
        ORDER BY created_at ASC
        """
    )
    rows = cur.fetchall()
    conn.close()
    return rows


def append_entries(target: Path, entries: list[str]):
    if not entries:
        return
    orig = target.read_text(encoding="utf-8")
    sep = "\n\n" if orig and not orig.endswith("\n\n") else ("\n" if orig and not orig.endswith("\n") else "")
    if orig.endswith("\n") and not orig.endswith("\n\n"):
        sep = "\n"
    payload = "\n\n".join(entries) + "\n"
    target.write_text(orig + sep + payload, encoding="utf-8")


def repair_existing_replies(target: Path, turn_map: dict[int, tuple[str, str, str, str]]) -> int:
    """In-place repair of existing [step:<N>] sections so each pairs with its exact reply."""
    text = target.read_text(encoding="utf-8")
    lines = text.splitlines(keepends=True)
    # Split into header + ### sections
    out_parts = []
    cur = []
    repaired = 0

    def flush(sec_lines: list[str]):
        nonlocal repaired
        if not sec_lines:
            return
        sec = "".join(sec_lines)
        m = re.search(r"\[step:(\d+)\]", sec_lines[0])
        if m:
            step_idx = int(m.group(1))
            if step_idx in turn_map:
                day, ts, body, reply = turn_map[step_idx]
                reply_full = sanitize_block(reply or "(正在执行/待本轮结束落盘)", None)
                new_sec = (
                    f"### {day}（用户原始指令与完整回复 {ts}）[step:{step_idx}]\n"
                    f"- **用户完整原始指令**：\n"
                    f"{indent_quote(body)}\n"
                    f"- **完整回复与执行汇报**：\n"
                    f"{indent_quote(reply_full)}\n\n"
                )
                if new_sec.strip() != sec.strip():
                    repaired += 1
                out_parts.append(new_sec)
                return
        out_parts.append(sec)

    for ln in lines:
        if ln.startswith("### "):
            flush(cur)
            cur = [ln]
        else:
            cur.append(ln)
    flush(cur)
    if repaired > 0:
        target.write_text("".join(out_parts).rstrip("\n") + "\n", encoding="utf-8")
    return repaired


def main():
    parser = argparse.ArgumentParser(description="Audit and losslessly backfill SESSION.md & SESSION_H.md.")
    parser.add_argument("--transcript", type=Path, default=DEFAULT_TRANSCRIPT)
    parser.add_argument("--today", type=str, default=datetime.datetime.now(LOCAL_TZ).strftime("%Y-%m-%d"))
    parser.add_argument("--check", action="store_true", help="Only audit and print missing counts without writing.")
    parser.add_argument("--repair-replies", action="store_true", help="Repair existing [step:N] reply pairings in-place.")
    args = parser.parse_args()

    turns = load_transcript_turns(args.transcript)
    if args.repair_replies:
        turn_map = {step_idx: (day, ts, body, reply) for step_idx, day, ts, body, reply in turns}
        r_hist = repair_existing_replies(SESSION_HIST, turn_map)
        r_hot = repair_existing_replies(SESSION_HOT, turn_map)
        print(f"[REPAIR_OK] repaired replies: SESSION_H.md={r_hist}, SESSION.md={r_hot}")

    combined = SESSION_HOT.read_text(encoding="utf-8") + "\n" + SESSION_HIST.read_text(encoding="utf-8")

    # 1. Audit & format missing queue.db tickets
    tickets = load_queue_tickets(QUEUE_DB)
    hist_ticket_entries = []
    hot_ticket_entries = []
    for task_id, project, category, created_by, status, created_at, resolved_at, error_msg, note in tickets:
        if task_id in combined:
            continue
        day, ts = to_local_dt(created_at or "")
        err_snip = sanitize_block(error_msg or "(无报错摘要)", 400).replace("\n", " ")
        note_snip = sanitize_block(note or "(处理中/未填写结单备注)", 500).replace("\n", " ")
        entry = (
            f"### {day}（下游工单全量补录：`{task_id}`）[ticket:{task_id}]\n"
            f"- **工单来源**：`{created_by}`（项目 `{project}` / 分类 `{category}`） | 创建：`{ts}` | 状态：`{status}` | 结单：`{resolved_at or '进行中'}`\n"
            f"- **报错/需求摘要**：{err_snip}\n"
            f"- **结单与处理汇报**：{note_snip}"
        )
        if day < args.today:
            hist_ticket_entries.append(entry)
        else:
            hot_ticket_entries.append(entry)

    # 2. Audit & format missing verbatim USER_EXPLICIT instructions
    turns = load_transcript_turns(args.transcript)
    hist_user_entries = []
    hot_user_entries = []
    for step_idx, day, ts, body, reply in turns:
        marker = f"[step:{step_idx}]"
        if marker in combined:
            continue
        reply_full = sanitize_block(reply or "(正在执行/待本轮结束落盘)", None)
        entry = (
            f"### {day}（用户原始指令与完整回复 {ts}）{marker}\n"
            f"- **用户完整原始指令**：\n"
            f"{indent_quote(body)}\n"
            f"- **完整回复与执行汇报**：\n"
            f"{indent_quote(reply_full)}"
        )
        if day < args.today:
            hist_user_entries.append(entry)
        else:
            hot_user_entries.append(entry)

    print(
        f"[AUDIT] tickets_total={len(tickets)} missing_tickets={len(hist_ticket_entries) + len(hot_ticket_entries)} "
        f"(hist={len(hist_ticket_entries)}, hot={len(hot_ticket_entries)}) | "
        f"user_turns_total={len(turns)} missing_user_turns={len(hist_user_entries) + len(hot_user_entries)} "
        f"(hist={len(hist_user_entries)}, hot={len(hot_user_entries)})"
    )

    if args.check:
        return

    append_entries(SESSION_HIST, hist_ticket_entries + hist_user_entries)
    append_entries(SESSION_HOT, hot_ticket_entries + hot_user_entries)
    print(
        f"[BACKFILL_OK] SESSION_H.md += {len(hist_ticket_entries)} tickets + {len(hist_user_entries)} user cmds | "
        f"SESSION.md += {len(hot_ticket_entries)} tickets + {len(hot_user_entries)} user cmds"
    )


if __name__ == "__main__":
    main()
