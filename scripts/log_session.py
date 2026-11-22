#!/usr/bin/env python3
"""
实时向 SESSION.md 追加用户原始指令、下游 AGY 工单及结单汇报的原子工具脚本。
确保绝不遗漏任何用户指令、下游工单或主智能体汇报。
"""
import argparse
import datetime
from pathlib import Path

SESSION_HOT = Path("/data/dev/cp/SESSION.md")
LOCAL_TZ = datetime.timezone(datetime.timedelta(hours=11))


def indent_quote(text: str) -> str:
    lines = text.strip().splitlines() or [""]
    return "\n".join(f"  > {ln}" if ln.strip() else "  >" for ln in lines)


def append_block(block: str):
    orig = SESSION_HOT.read_text(encoding="utf-8")
    sep = "\n\n" if orig and not orig.endswith("\n\n") else ("\n" if orig and not orig.endswith("\n") else "")
    if orig.endswith("\n") and not orig.endswith("\n\n"):
        sep = "\n"
    SESSION_HOT.write_text(orig + sep + block.rstrip("\n") + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description="Append verbatim user command / ticket / report to SESSION.md.")
    parser.add_argument("--title", required=True, help="Entry title")
    parser.add_argument("--user-cmd", default="", help="Verbatim user instruction")
    parser.add_argument("--ticket", default="", help="Downstream AGY ticket ID (e.g. task-bun-...)")
    parser.add_argument("--source", default="", help="Ticket source agent (e.g. bun_agy / capsurf_agy)")
    parser.add_argument("--status", default="", help="Ticket status (received / resolved)")
    parser.add_argument("--report", required=True, help="Execution result / resolution report")
    args = parser.parse_args()

    now = datetime.datetime.now(LOCAL_TZ)
    day = now.strftime("%Y-%m-%d")
    ts = now.strftime("%Y-%m-%d %H:%M:%S")
    lines = [f"### {day}（{args.title} | {ts}）"]
    if args.ticket:
        lines.append(f"- **下游工单**：`{args.ticket}` | 来源：`{args.source or 'unknown'}` | 状态：`{args.status or 'updated'}`")
    if args.user_cmd:
        lines.append("- **用户完整原始指令**：")
        lines.append(indent_quote(args.user_cmd))
    lines.append("- **执行与结单汇报**：")
    lines.append(indent_quote(args.report))

    append_block("\n".join(lines))
    print(f"[SESSION_LOG_OK] appended '{args.title}' to {SESSION_HOT}")


if __name__ == "__main__":
    main()
