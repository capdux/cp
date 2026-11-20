#!/usr/bin/env bash
# 一键验证标准库模块合规性与模块化测试（主控与子智能体共用，消除重复测试与多轮往返）
# 用法: bash scripts/verify_module.sh <lib_dir> <test_file.cp> [extra_test.cp ...]
set -euo pipefail

if [ $# -lt 2 ]; then
    echo "Usage: bash scripts/verify_module.sh <lib_dir> <test_file.cp> [extra_test.cp ...]"
    exit 2
fi

LIB_DIR="$1"
shift

echo "[1/4] 扫描禁止依赖 (std.posix / @import(\"bun\")) -> $LIB_DIR"
if rg -n 'std\.posix|@import\("bun"\)' "$LIB_DIR" --glob '*.cp'; then
    echo "[FAIL] 检测到禁止依赖！"
    exit 1
fi
echo "  OK: 0 违规"

echo "[2/4] 校验首行 // Source: 溯源头 -> $LIB_DIR"
MISSING_SRC=0
for f in "$LIB_DIR"/*.cp; do
    [ -e "$f" ] || continue
    if ! head -n 1 "$f" | grep -q "^// Source:"; then
        echo "  [MISSING // Source:] $f"
        MISSING_SRC=1
    fi
done
if [ "$MISSING_SRC" -ne 0 ]; then
    exit 1
fi
echo "  OK: 100% // Source: 合规"

echo "[3/4] cpc check --brief 静态快检 -> $LIB_DIR/index.cp"
if [ -f "$LIB_DIR/index.cp" ]; then
    cpc check --brief "$LIB_DIR/index.cp"
fi

echo "[4/4] cap test -m 模块化增量验收 -> $*"
for t in "$@"; do
    cap test -m "$t"
done

echo "[VERIFY_MODULE_OK] $LIB_DIR 全部合规检查与测试 100% PASS！"
