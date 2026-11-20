#!/usr/bin/env bash
set -euo pipefail

# 支持传入参数：默认 debug，传入 release 则构建正式优化版
MODE="${1:-debug}"

MANIFEST="/data/dev/cp/cpc/Cargo.toml"
CAP_BIN="/data/dev/cp/cap/bin/cpc"
REL_BIN="/data/dev/cp/cpc/target/release/cpc"
BUILD_LOG="/tmp/cpc_build.log"

if [ "$MODE" = "release" ]; then
    CARGO_FLAGS="--release"
    TARGET_BIN="/data/dev/cp/cpc/target/release/cpc"
    SHM_DIR="/dev/shm/cpc_target/release"
    TIMEOUT_SEC=300  # Release 开启编译器优化耗时较长，超时放宽到 300s
else
    CARGO_FLAGS=""
    TARGET_BIN="/data/dev/cp/cpc/target/debug/cpc"
    SHM_DIR="/dev/shm/cpc_target/debug"
    TIMEOUT_SEC=120
fi

# 1. 清理残留的僵尸 cargo 进程，防止死锁
pkill -f "cargo.*cpc/Cargo.toml" 2>/dev/null || true

# 2. 后台执行构建并输出心跳点（防 agy 卡死且不消耗多余 token）
echo -n "[1/2] 构建 cpc ($MODE) 二进制 "
cargo build --manifest-path "$MANIFEST" $CARGO_FLAGS --message-format=short >"$BUILD_LOG" 2>&1 < /dev/null &
BUILD_PID=$!

ELAPSED=0
while kill -0 "$BUILD_PID" 2>/dev/null; do
    sleep 3
    ELAPSED=$((ELAPSED + 3))
    echo -n "."
    if [ "$ELAPSED" -ge "$TIMEOUT_SEC" ]; then
        kill -9 "$BUILD_PID" 2>/dev/null || true
        echo ""
        echo "[ERROR] 编译超时（超过 ${TIMEOUT_SEC}s），已强制终止！"
        exit 1
    fi
done

if ! wait "$BUILD_PID"; then
    echo ""
    echo "[ERROR] 编译失败，关键报错如下："
    grep -E "^error|-->" -A 3 "$BUILD_LOG" | head -n 40 || tail -n 30 "$BUILD_LOG"
    exit 1
fi
echo " 完成 (${ELAPSED}s)"

# 3. 原子部署二进制（临时文件 + mv -f 原子替换，消除下游并发构建 ETXTBSY / 缺文件窗口）
echo "[2/2] 原子部署 cpc ($MODE) 到目标路径..."
atomic_install() {
    local src="$1"
    local dst="$2"
    local tmp="${dst}.tmp.$$"
    install -m 755 "$src" "$tmp"
    mv -f "$tmp" "$dst"
}

atomic_install "$TARGET_BIN" "$CAP_BIN"

# 仅在 debug 模式下，将产物同步一份到 target/release 兼容旧路径
if [ "$MODE" = "debug" ]; then
    atomic_install "$TARGET_BIN" "$REL_BIN"
fi

if [ -d "$SHM_DIR" ]; then
    atomic_install "$TARGET_BIN" "$SHM_DIR/cpc"
fi

echo "[CPC_SYNC_OK] cpc ($MODE) 编译器已更新并成功原子部署生效！"
