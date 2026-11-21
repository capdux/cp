#!/usr/bin/env python3
"""
CP 新语法批量迁移脚本 (migrate_cp_syntax.py)

特点：严格跳过 `zig { ... }` / `__zig__ { ... }` 逃逸块与行注释，绝不误伤底层 Zig 代码。

支持自动迁移规则：
  1. 唯一析构函数 `~init()`：将非 static、无额外参数、无返回值的 RAII 析构方法
     `[pub] fn release(*_) {` / `[pub] fn release() {` 替换为 `~init() {`
     （自动跳过带参/带返回值/static/引用计数 retain+ref 的普通业务 release 方法）；
  2. 可选 `--rewrite-release-calls`：将手动调用旧析构的 `<expr>.release()` 统一迁移为
     `<expr>.~init()`（双重幂等、绝不 Double-Free；跳过内置容器/引用计数测试文件）；
  3. 机械清理过时语法（仅在纯 CP 代码区生效，跳过 zig{} 块）：
     - `let mut ` -> `let `
     - `mut _` -> `*_`
     - 冗余只读接收者 `fn foo(_)` -> `fn foo()` / `fn foo(_, x)` -> `fn foo(x)`
     - 冗余返回类型 `fn foo(...): void {` -> `fn foo(...) {`
     - 语句级丢弃 `_ = ` -> `* = `
  4. 非破坏性静态审计提示（`--show-audits`）：
     - 疑似漏写 `!` 的二级裸指针 `**T`（应写 `**T!`）；
     - 文件内未迁移的 `.release()` 调用。

用法：
  # 仅预览（默认 dry-run）
  python3 scripts/migrate_cp_syntax.py lib/ cap/test/

  # 一键批量迁移（含 RAII release -> ~init() 与调用点迁移）
  python3 scripts/migrate_cp_syntax.py --apply --rewrite-release-calls lib/ cap/test/

  # 下游 Bun / CapSurf 资产（跳过 release -> ~init，仅做语法清理与审计）
  python3 scripts/migrate_cp_syntax.py --apply --no-release src/
"""

import argparse
import os
import re
import sys
from pathlib import Path
from typing import List, Set, Tuple

RE_ZIG_BLOCK_START = re.compile(r"\b(?:__zig__|zig)\s*\{")
RE_RELEASE_DTOR = re.compile(
    r"^([ \t]*)(?:pub[ \t]+)?fn[ \t]+release[ \t]*\(\s*(?:\*_\s*)?\)[ \t]*\{"
)
RE_HAS_RETAIN = re.compile(r"\bfn[ \t]+(?:retain|ref)\s*\(")
RE_LET_MUT = re.compile(r"\blet[ \t]+mut[ \t]+")
RE_MUT_RECV = re.compile(r"(\bfn[ \t]+\w+\s*\(\s*)mut[ \t]+_")
RE_REDUNDANT_RO_RECV_ONLY = re.compile(r"(\bfn[ \t]+\w+\s*\()\s*_\s*\)")
RE_REDUNDANT_RO_RECV_FIRST = re.compile(r"(\bfn[ \t]+\w+\s*\()\s*_\s*,\s*")
RE_REDUNDANT_VOID_RET = re.compile(r"(\bfn[ \t]+\w+[^{;\n]*\))\s*:\s*void\s*\{")
RE_UNDERSCORE_DISCARD = re.compile(r"^([ \t]*)_[ \t]*=(?!>)[ \t]*")
RE_DOUBLE_STAR_NO_BANG = re.compile(r":\s*\*\*(?:const\s+)?[A-Za-z_]\w*(?!\s*!)")
RE_CALL_RELEASE = re.compile(r"\.release\(\s*\)")

# 保留内置容器/锁守卫/SharedTensor/专门测试普通 release 方法的文件不做 release 替换
SKIP_RELEASE_FILES: Set[str] = {
    "test_raii_deinit_ownership_deep.cp",
    "test_all.cp",
    "test_mutex_atomic.cp",
    "test_collections_matrix.cp",
    "test_sync_primitives.cp",
    "arc_slice.cp",
    "allocator.cp",
}


def compute_zig_block_lines(lines: List[str]) -> Set[int]:
    """返回所有处于 zig { ... } / __zig__ { ... } 块内的 0-based 行号集合。"""
    inside: Set[int] = set()
    zig_depth = 0
    for idx, line in enumerate(lines):
        stripped = line.lstrip()
        if stripped.startswith("//"):
            if zig_depth > 0:
                inside.add(idx)
            continue
        if zig_depth > 0:
            inside.add(idx)
            zig_depth += line.count("{") - line.count("}")
            if zig_depth < 0:
                zig_depth = 0
            continue
        m = RE_ZIG_BLOCK_START.search(line)
        if m:
            inside.add(idx)
            tail = line[m.start() :]
            zig_depth = tail.count("{") - tail.count("}")
            if zig_depth < 0:
                zig_depth = 0
    return inside


def migrate_content(
    path: Path,
    text: str,
    migrate_release: bool,
    rewrite_release_calls: bool,
) -> Tuple[str, List[str], List[str]]:
    changes: List[str] = []
    audits: List[str] = []
    lines = text.splitlines(keepends=True)
    zig_lines = compute_zig_block_lines(lines)

    skip_rel = path.name in SKIP_RELEASE_FILES
    has_retain = bool(RE_HAS_RETAIN.search(text))

    counts = {
        "dtor": 0,
        "rel_call": 0,
        "let_mut": 0,
        "mut_recv": 0,
        "ro_recv": 0,
        "void_ret": 0,
        "discard": 0,
    }

    out_lines: List[str] = []
    for idx, line in enumerate(lines):
        if idx in zig_lines or line.lstrip().startswith("//"):
            out_lines.append(line)
            continue

        cur = line

        # 1. RAII destructor header: [pub] fn release(*_) { / [pub] fn release() { -> ~init() {
        if migrate_release and not skip_rel and not has_retain:
            m = RE_RELEASE_DTOR.match(cur)
            if m:
                indent = m.group(1)
                cur = indent + "~init() {" + cur[m.end() :]
                counts["dtor"] += 1

        # 2. Optional rewrite of manual `.release()` calls -> `.~init()`
        if rewrite_release_calls and not skip_rel and not has_retain:
            cur, n = RE_CALL_RELEASE.subn(".~init()", cur)
            counts["rel_call"] += n

        # 3. let mut -> let
        cur, n = RE_LET_MUT.subn("let ", cur)
        counts["let_mut"] += n

        # 4. fn foo(mut _ -> fn foo(*_
        cur, n = RE_MUT_RECV.subn(r"\1*_", cur)
        counts["mut_recv"] += n

        # 5. Redundant read-only receiver (_) -> ()
        cur, n1 = RE_REDUNDANT_RO_RECV_ONLY.subn(r"\1)", cur)
        cur, n2 = RE_REDUNDANT_RO_RECV_FIRST.subn(r"\1", cur)
        counts["ro_recv"] += n1 + n2

        # 6. Redundant ': void {' -> ' {'
        cur, n = RE_REDUNDANT_VOID_RET.subn(r"\1 {", cur)
        counts["void_ret"] += n

        # 7. Statement discard `_ = ` -> `* = `
        cur, n = RE_UNDERSCORE_DISCARD.subn(r"\1* = ", cur)
        counts["discard"] += n

        # 8. Non-destructive audits
        if RE_DOUBLE_STAR_NO_BANG.search(cur):
            audits.append(f"L{idx + 1}: 检测到 `**T` 未带尾置 `!`（若是二级裸指针请改为 `**T!`）")

        out_lines.append(cur)

    if counts["dtor"]:
        changes.append(f"release(*_)/release() -> ~init() ({counts['dtor']} 处)")
    if counts["rel_call"]:
        changes.append(f".release() -> .~init() ({counts['rel_call']} 处)")
    if counts["let_mut"]:
        changes.append(f"let mut -> let ({counts['let_mut']} 处)")
    if counts["mut_recv"]:
        changes.append(f"mut _ -> *_ ({counts['mut_recv']} 处)")
    if counts["ro_recv"]:
        changes.append(f"移除冗余只读接收者 '_' ({counts['ro_recv']} 处)")
    if counts["void_ret"]:
        changes.append(f"移除冗余 ': void' ({counts['void_ret']} 处)")
    if counts["discard"]:
        changes.append(f"丢弃语法 `_ =` -> `* =` ({counts['discard']} 处)")

    if migrate_release and has_retain and not skip_rel:
        if any(
            idx not in zig_lines and RE_RELEASE_DTOR.match(l)
            for idx, l in enumerate(lines)
        ):
            audits.append("检测到 retain/ref 引用计数方法，已自动跳过 release -> ~init()")

    return "".join(out_lines), changes, audits


def collect_cp_files(targets: List[str]) -> List[Path]:
    files: List[Path] = []
    for t in targets:
        p = Path(t)
        if p.is_file() and p.suffix == ".cp":
            files.append(p)
        elif p.is_dir():
            for root, dirs, filenames in os.walk(p):
                dirs[:] = [
                    d
                    for d in dirs
                    if d not in (".git", ".cap", "bin", "target", "node_modules", "vendor")
                ]
                for fn in sorted(filenames):
                    if fn.endswith(".cp"):
                        files.append(Path(root) / fn)
    return sorted(set(files))


def main() -> int:
    parser = argparse.ArgumentParser(description="CP 新语法批量迁移脚本")
    parser.add_argument("targets", nargs="+", help="目标 .cp 文件或目录")
    parser.add_argument(
        "--apply",
        action="store_true",
        help="就地写入修改（默认 dry-run 仅预览）",
    )
    parser.add_argument(
        "--no-release",
        action="store_true",
        help="跳过 `fn release(*_) -> ~init()` 替换（适用于业务 release 方法较多的目录）",
    )
    parser.add_argument(
        "--rewrite-release-calls",
        action="store_true",
        help="同步将 `.release()` 调用改写为 `.~init()`（自动跳过内置容器与引用计数文件）",
    )
    parser.add_argument(
        "--show-audits",
        action="store_true",
        help="打印所有静态审计提示",
    )
    args = parser.parse_args()

    files = collect_cp_files(args.targets)
    changed_files = 0
    audit_files = 0

    for f in files:
        try:
            orig = f.read_text(encoding="utf-8")
        except Exception as exc:
            print(f"[SKIP] {f}: {exc}", file=sys.stderr)
            continue

        updated, changes, audits = migrate_content(
            f,
            orig,
            migrate_release=not args.no_release,
            rewrite_release_calls=args.rewrite_release_calls,
        )
        if changes:
            changed_files += 1
            mode = "APPLY" if args.apply else "DRY-RUN"
            print(f"[{mode}] {f}: " + "; ".join(changes))
            if args.apply and updated != orig:
                f.write_text(updated, encoding="utf-8")
        if audits:
            audit_files += 1
            if args.show_audits or changes:
                for a in audits:
                    print(f"  [AUDIT] {f}: {a}")

    print(
        f"\n汇总：扫描 {len(files)} 个 .cp 文件，"
        f"{'已更新' if args.apply else '待更新'} {changed_files} 个文件，"
        f"存在审计提示 {audit_files} 个文件。"
    )
    if not args.apply and changed_files > 0:
        print("提示：追加 `--apply` 参数以就地执行批量迁移。")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
