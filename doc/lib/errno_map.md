# `cap:errno_map` — 纯 CP 跨平台 POSIX/OS Errno 统一映射与元数据查询库

> 参考来源：/data/dev/bun/src/sys/coreutils_error_map.cp, /data/dev/bun/src/errno/{linux_errno,darwin_errno}.cp | 吸收日期：2026-11-22

## 概述

`cap:errno_map` 吸收自 Bun `src/sys/coreutils_error_map.cp` 与 `src/errno/{linux_errno,darwin_errno}.cp`，纯 CP 零外部依赖、零堆分配、零 `std.posix` 依赖，覆盖 69 项核心 POSIX 错误码在 Linux 与 macOS/Darwin 之间的双向数字码映射、名称反查、OS 特定错误消息以及瞬态可重试判定。

## 核心类型与 API

- `ErrnoOs`：`linux` / `darwin`
- `SystemErrno`（69 项 POSIX 错误码枚举）：
  - `fromLinuxCode(code: i32): SystemErrno?`（支持正数及内核 `-errno` 负值）
  - `fromDarwinCode(code: i32): SystemErrno?`（支持正数及 `-errno` 负值）
  - `toOsCode(os: ErrnoOs): i32`
  - `fromName(name: []const u8): SystemErrno?`（支持 `EWOULDBLOCK`/`ENOTSUP` 别名）
  - `name(): []const u8`
  - `message(os: ErrnoOs): []const u8`（区分 Linux 与 Darwin 描述差异，如 `EBUSY`/`ETIMEDOUT`/`ENODEV`/`EOPNOTSUPP`）
  - `isTransient(): bool`（瞬态可重试错误判定）

## 示例

```cp
import { ErrnoOs, SystemErrno } from "cap:errno_map";

const e = SystemErrno.fromLinuxCode(110).?; // SystemErrno.ETIMEDOUT
// e.toOsCode(ErrnoOs.darwin) == 60
// e.message(ErrnoOs.linux) == "Connection timed out"
// e.message(ErrnoOs.darwin) == "Operation timed out"
// e.isTransient() == true
```

## 上游行为契约与跨平台映射边界

1. **正/负系统调用返回值统一解码**：
   - Linux 内核 `syscall` 失败常直接返回 `-errno`（如 `-110`），`fromLinuxCode` 与 `fromDarwinCode` 均兼容正负两种符号输入，未知错误码安全返回 `null`。
2. **POSIX 别名与跨 OS 差异隔离**：
   - `fromName` 识别 `EWOULDBLOCK`（映射至 `EAGAIN`）与 `ENOTSUP`（映射至 `EOPNOTSUPP`）等历史别名；`toOsCode` 与 `message` 按 `ErrnoOs.linux` / `ErrnoOs.darwin` 精确输出对应平台编号与文案（如 `ETIMEDOUT` 在 Linux 为 `110`、Darwin 为 `60`）。
3. **瞬态可重试判定 (`isTransient`)**：
   - 统一识别 `EAGAIN`/`EINTR`/`EBUSY`/`ETIMEDOUT`/`ECONNRESET` 等瞬态错误，便于直接衔接 `cap:retry_backoff` 执行退避重试。
