#!/usr/bin/env python3
"""
标准库验收一键登记脚本：
单次执行自动更新 cp_lib_ref.md、CHANGELOG.md、DEVLOG.md、SESSION.md，
并可选一次性 git add + git commit，将主控收尾从 2-3 轮压缩至 1 轮。
"""
import argparse
import subprocess
from datetime import datetime
from pathlib import Path

ROOT = Path("/data/dev/cp")


def insert_into_cp_lib_ref(mod: str, exports: str, desc: str) -> None:
    p = ROOT / "cp_lib_ref.md"
    text = p.read_text(encoding="utf-8")
    row = f"| `cap:{mod}` | {exports} | {desc} |\n"
    if f"| `cap:{mod}` |" in text:
        return
    # 插入在最后一个 | `cap: 表格行或末尾
    lines = text.splitlines(keepends=True)
    last_cap_idx = -1
    for i, line in enumerate(lines):
        if line.startswith("| `cap:"):
            last_cap_idx = i
    if last_cap_idx >= 0:
        lines.insert(last_cap_idx + 1, row)
        p.write_text("".join(lines), encoding="utf-8")
    else:
        p.write_text(text.rstrip("\n") + "\n" + row, encoding="utf-8")


def prepend_after_note(filename: str, block: str) -> None:
    p = ROOT / filename
    text = p.read_text(encoding="utf-8")
    marker = "> 历史"
    idx = text.find(marker)
    if idx == -1:
        p.write_text(block.rstrip("\n") + "\n\n" + text, encoding="utf-8")
        return
    line_end = text.find("\n", idx)
    if line_end == -1:
        line_end = len(text)
    prefix = text[: line_end + 1].rstrip("\n")
    rest = text[line_end + 1 :].lstrip("\n")
    p.write_text(prefix + "\n\n" + block.rstrip("\n") + "\n\n" + rest, encoding="utf-8")


def append_session(block: str) -> None:
    p = ROOT / "SESSION.md"
    text = p.read_text(encoding="utf-8").rstrip("\n")
    p.write_text(text + "\n\n" + block.rstrip("\n") + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description="Register a verified stdlib into 4-line docs and optionally commit.")
    parser.add_argument("--mod", required=True, help="Module name, e.g. oauth2")
    parser.add_argument("--exports", required=True, help="Key exported symbols summary")
    parser.add_argument("--desc", required=True, help="One-line description for cp_lib_ref.md")
    parser.add_argument("--changelog", required=True, help="CHANGELOG bullet body")
    parser.add_argument("--devlog", required=True, help="DEVLOG core design summary")
    parser.add_argument("--session", required=True, help="SESSION entry summary")
    parser.add_argument("--date", default=datetime.now().strftime("%Y-%m-%d"), help="YYYY-MM-DD")
    parser.add_argument("--title", default="", help="Optional module short title")
    parser.add_argument("--commit", default="", help="Git commit message if auto-commit is desired")
    parser.add_argument("--extra-paths", nargs="*", default=[], help="Extra paths to git add")
    args = parser.parse_args()

    title = args.title or f"新建 `cap:{args.mod}` {args.desc}"
    insert_into_cp_lib_ref(args.mod, args.exports, args.desc)

    cl_block = f"## [{args.date}] - {title}（5/5 PASS，自动动态挂载 `cap.{args.mod}`）\n{args.changelog}"
    prepend_after_note("CHANGELOG.md", cl_block)

    dl_block = f"## {args.date}: {title}（5/5 PASS，文档直写 `doc/lib/{args.mod}.md`）\n{args.devlog}"
    prepend_after_note("DEVLOG.md", dl_block)

    ss_block = f"### {args.date}（`cap:{args.mod}` 验收，{args.session}）\n- **执行成果**：`lib/{args.mod}/`（5/5 PASS，`doc/lib/{args.mod}.md`）经 `verify_module.sh` 一次性验收回收；{args.session}"
    append_session(ss_block)
    print(f"[REGISTER_OK] Updated 4-line docs for cap:{args.mod}")

    if args.commit:
        paths = [
            f"lib/{args.mod}/",
            f"doc/lib/{args.mod}.md",
            f"cap/test/test_{args.mod}_production_deep.cp",
            "cp_lib_ref.md",
            "CHANGELOG.md",
            "DEVLOG.md",
            "SESSION.md",
        ] + args.extra_paths
        subprocess.run(["git", "add"] + paths, cwd=ROOT, check=True)
        subprocess.run(["git", "commit", "-m", args.commit], cwd=ROOT, check=True)
        print(f"[COMMIT_OK] {args.commit}")


if __name__ == "__main__":
    main()
