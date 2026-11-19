# CP 标准库：跨平台系统错误码子系统手册 (`cap:errno`)

## 一、 架构总览与设计准则

`cap:errno` 是 CP 语言官方内置的跨平台操作系统错误码标准库，吸收自 Bun 的系统错误码体系（`/data/dev/bun/src/errno/{linux_errno,darwin_errno,windows_errno}.cp`），提供强类型 `SystemErrno` 枚举、POSIX/Linux 系统调用返回值双向转换、人类可读诊断文本及语义分类断言（重试、未找到、权限不足、网络错误）。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                      CP 应用层 / 标准库 (fs / net / sys)                │
├────────────────────────────────────────────────────────────────────────┤
│                      SystemErrno (u16 强类型枚举)                       │
│  fromCode(i32) · toCode() · .name / codeName() · description()         │
│  isOk() · isRetryable() · isNotFound() · isPermissionDenied()          │
│  isNetworkError()                                                      │
├────────────────────────────────────────────────────────────────────────┤
│         兼容 POSIX 正整数错误码与 Linux 负系统调用返回值 (-errno)        │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/errno/index.cp` | `// Source: /data/dev/bun/src/errno/linux_errno.cp` | `cap:errno` 统一公开导出入口 |
| `lib/errno/errno.cp` | `// Source: /data/dev/bun/src/errno/linux_errno.cp` | `SystemErrno` 枚举定义（覆盖 65 个主流 POSIX/Linux 错误码）与全部辅助方法 |

---

## 二、 核心 API 详解

### 1. 常用 `SystemErrno` 枚举值

| 类别 | 枚举成员 | 典型数值 (Linux) |
| :--- | :--- | :--- |
| **成功 / 未知** | `SUCCESS`、`UNKNOWN` | `0`、`65535` |
| **文件与路径** | `ENOENT`、`EEXIST`、`ENOTDIR`、`EISDIR`、`ENAMETOOLONG`、`ENOTEMPTY`、`ELOOP`、`ENOSPC`、`EROFS` | `2`, `17`, `20`, `21`, `36`, `39`, `40`, `28`, `30` |
| **权限与资源** | `EPERM`、`EACCES`、`ENOMEM`、`EBUSY`、`EMFILE`、`ENFILE`、`EINVAL`、`EBADF`、`EPIPE` | `1`, `13`, `12`, `16`, `24`, `23`, `22`, `9`, `32` |
| **可重试与并发** | `EINTR`、`EAGAIN`、`EWOULDBLOCK`、`EDEADLK`、`ECANCELED` | `4`, `11`, `41`, `35`, `125` |
| **网络与套接字** | `EADDRINUSE`、`EADDRNOTAVAIL`、`ECONNRESET`、`ECONNABORTED`、`ETIMEDOUT`、`ECONNREFUSED`、`ENETUNREACH`、`EHOSTUNREACH`、`EINPROGRESS` | `98`, `99`, `104`, `103`, `110`, `111`, `101`, `113`, `115` |

### 2. 方法列表

| API 签名 | 说明 |
| :--- | :--- |
| `SystemErrno.fromCode(code: i32): SystemErrno` | 将正数错误码或 Linux 负系统调用返回值（如 `-2`）转为 `SystemErrno`，未知码返回 `.UNKNOWN` |
| `err.toCode(): i32` | 返回底层数值错误码 |
| `err.name` / `err.codeName()` / `SystemErrno.name(err)` | 返回标准 POSIX 符号名（如 `"ENOENT"`、`"ECONNREFUSED"`） |
| `err.description(): []const u8` | 返回人类可读 POSIX 诊断描述（如 `"No such file or directory"`） |
| `err.isOk(): bool` | 是否为 `SUCCESS` (`0`) |
| `err.isRetryable(): bool` | 是否为可重试瞬时错误（`EINTR`、`EAGAIN`、`EWOULDBLOCK`、`EBUSY`、`ETIMEDOUT`） |
| `err.isNotFound(): bool` | 是否为目标不存在错误（`ENOENT`、`ESRCH`、`ENODEV`、`ENXIO`） |
| `err.isPermissionDenied(): bool` | 是否为权限拒绝或只读介质错误（`EPERM`、`EACCES`、`EROFS`） |
| `err.isNetworkError(): bool` | 是否为网络与套接字传输错误 |

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { SystemErrno } from "cap:errno";

pub fn handleSyscallResult(ret: i32) {
    const err = SystemErrno.fromCode(ret);
    if (err.isOk()) {
        return;
    }
    if (err.isRetryable()) {
        // EINTR / EAGAIN / EWOULDBLOCK / EBUSY / ETIMEDOUT -> 调度退避重试
        return;
    }
    if (err.isNotFound()) {
        // ENOENT -> 文件不存在分支
        const sym = err.codeName();        // "ENOENT"
        const desc = err.description();    // "No such file or directory"
        _ = sym;
        _ = desc;
    }
}
```

---

## 四、 与 Bun `src/errno/` 对照表

| 功能分类 | Bun (`src/errno/{linux,darwin,windows}_errno.cp`) | CP 标准库 (`cap:errno`) |
| :--- | :--- | :--- |
| **系统错误枚举** | `bun.sys.SystemErrno` (`u16`) | `SystemErrno` (`u16`) |
| **错误码解析** | `SystemErrno.init(code)` | `SystemErrno.fromCode(code)`（自动处理 `-errno`） |
| **符号名与描述** | `@tagName(err)` / `SystemErrno.label` | `err.name` / `err.codeName()` / `err.description()` |
| **语义分类判断** | 分散于 `src/sys.zig` / `libuv` | `isRetryable()` / `isNotFound()` / `isPermissionDenied()` / `isNetworkError()` |
