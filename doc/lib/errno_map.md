# `cap:errno_map` — 纯 CP 跨平台 POSIX/OS Errno 统一映射与元数据查询库

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
