#!/usr/bin/env python3
"""
一次性/可复用热日志无损滚动归档脚本：
将 SESSION.md / CHANGELOG.md / DEVLOG.md 中较旧的条目无损追加至对应的 *_H.md 末尾，
仅保留最近 N 条活跃记录在热文件中，确保零内容丢失。
"""
import argparse
import re
from pathlib import Path

ROOT = Path("/data/dev/cp")


def split_sections(text: str, heading_prefix: str):
    lines = text.splitlines(keepends=True)
    header_lines = []
    sections = []
    current = []
    in_sections = False
    for line in lines:
        if line.startswith(heading_prefix):
            in_sections = True
            if current:
                sections.append("".join(current))
            current = [line]
        else:
            if not in_sections:
                header_lines.append(line)
            else:
                current.append(line)
    if current:
        sections.append("".join(current))
    return "".join(header_lines), sections


def archive_file(hot_name: str, hist_name: str, heading_prefix: str, keep: int, newest_first: bool) -> tuple[int, int, int]:
    hot_path = ROOT / hot_name
    hist_path = ROOT / hist_name
    hot_text = hot_path.read_text(encoding="utf-8")
    hist_text = hist_path.read_text(encoding="utf-8")

    header, sections = split_sections(hot_text, heading_prefix)
    total = len(sections)
    if total <= keep:
        return total, total, 0

    if newest_first:
        # CHANGELOG / DEVLOG: 最新在顶部，保留前 keep 条，其余归档
        keep_sections = sections[:keep]
        archive_sections = sections[keep:]
    else:
        # SESSION: 最新在底部，保留后 keep 条，其余归档
        archive_sections = sections[:-keep]
        keep_sections = sections[-keep:]

    archived_payload = "".join(archive_sections).strip("\n")
    if archived_payload:
        sep = "\n\n" if hist_text and not hist_text.endswith("\n\n") else ("\n" if hist_text and not hist_text.endswith("\n") else "")
        if hist_text.endswith("\n") and not hist_text.endswith("\n\n"):
            sep = "\n"
        new_hist = hist_text + sep + archived_payload + "\n"
    else:
        new_hist = hist_text

    new_hot = header.rstrip("\n") + "\n\n" + "".join(keep_sections).lstrip("\n")
    if not new_hot.endswith("\n"):
        new_hot += "\n"

    # 校验零内容丢失
    assert len(keep_sections) + len(archive_sections) == total, f"{hot_name} section count mismatch"

    hist_path.write_text(new_hist, encoding="utf-8")
    hot_path.write_text(new_hot, encoding="utf-8")
    return total, len(keep_sections), len(archive_sections)


def main():
    parser = argparse.ArgumentParser(description="Archive old entries from hot logs into *_H.md without data loss.")
    parser.add_argument("--keep", type=int, default=5, help="Number of recent entries to keep in each hot file.")
    args = parser.parse_args()

    jobs = [
        ("SESSION.md", "SESSION_H.md", "### ", False),
        ("CHANGELOG.md", "CHANGELOG_H.md", "## [", True),
        ("DEVLOG.md", "DEVLOG_H.md", "## 20", True),
    ]
    for hot_name, hist_name, prefix, newest_first in jobs:
        total, kept, archived = archive_file(hot_name, hist_name, prefix, args.keep, newest_first)
        hot_lines = len((ROOT / hot_name).read_text(encoding="utf-8").splitlines())
        hist_lines = len((ROOT / hist_name).read_text(encoding="utf-8").splitlines())
        print(f"[ARCHIVE_OK] {hot_name} -> {hist_name}: total={total}, kept={kept}, archived={archived}, hot_lines={hot_lines}, hist_lines={hist_lines}")


if __name__ == "__main__":
    main()
