# CP 标准库：跨平台系统错误码子系统手册 (`cap:errno`)

> 参考来源：`/data/dev/bun/src/errno/linux_errno.cp`, `/data/dev/bun/src/errno/` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:errno` 是 CP 语言官方内置的跨平台操作系统错误码标准库，吸收自 Bun 的系统错误码体系（`/data/dev/bun/src/errno/{linux_errno,darwin_errno,windows_errno}.cp`），提供覆盖完整 POSIX/Linux 134 项标准系统错误码（涵盖文件、网络、进程、IPC、信号、内存、权限、密钥与硬件错误等）的 `SystemErrno` 枚举、正负 syscall 返回值双向转换、人类可读 `strerror` 诊断描述及全面语义分类断言。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                      CP 应用层 / 标准库 (fs / net / sys)                │
├────────────────────────────────────────────────────────────────────────┤
│                   SystemErrno (u16 强类型枚举 · 135 项全集)             │
│  fromCode(i32) · toCode() · .name / name() / codeName() · description()│
│  isOk() · isRetryable() · isNotFound() · isPermissionDenied()          │
│  isWouldBlock() · isConnectionError() · isTimeout()                    │
│  isAlreadyExists() · isNetworkError()                                  │
├────────────────────────────────────────────────────────────────────────┤
│         兼容 POSIX 正整数错误码与 Linux 负系统调用返回值 (-errno)        │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/errno/index.cp` | `// Source: /data/dev/bun/src/errno/linux_errno.cp` | `cap:errno` 统一公开导出入口 |
| `lib/errno/errno.cp` | `// Source: /data/dev/bun/src/errno/linux_errno.cp` | 完整 134 项 POSIX/Linux 标准错误码（`0..133` + `UNKNOWN = 65535`）与全部语义判定方法 |

---

## 二、 核心 API 详解

### 1. 错误码全集分类概览 (`0..133`)

| 类别 | 涵盖枚举成员 | 数值区间 |
| :--- | :--- | :--- |
| **成功 / 未知** | `SUCCESS`, `UNKNOWN` | `0`, `65535` |
| **文件、路径与磁盘** | `ENOENT`, `EEXIST`, `EXDEV`, `ENOTDIR`, `EISDIR`, `ENAMETOOLONG`, `ENOTEMPTY`, `ELOOP`, `EFBIG`, `ENOSPC`, `EROFS`, `EMLINK`, `ESPIPE`, `ESTALE`, `EDQUOT`, `EISNAM` | `2`..`122` |
| **权限与密钥** | `EPERM`, `EACCES`, `EROFS`, `ENOKEY`, `EKEYEXPIRED`, `EKEYREVOKED`, `EKEYREJECTED` | `1`, `13`, `30`, `126..129` |
| **进程、信号与内存** | `ESRCH`, `EINTR`, `E2BIG`, `ENOEXEC`, `EBADF`, `ECHILD`, `ENOMEM`, `EFAULT`, `EBUSY`, `EINVAL`, `ENFILE`, `EMFILE`, `EPIPE`, `EDEADLK`, `ENOSYS`, `ECANCELED`, `ERESTART`, `EOWNERDEAD`, `ENOTRECOVERABLE`, `EHWPOISON` | `3`..`133` |
| **流、IPC 与消息** | `ENOMSG`, `EIDRM`, `ENOSTR`, `ENODATA`, `ETIME`, `ENOSR`, `ENOLINK`, `ECOMM`, `EPROTO`, `EMULTIHOP`, `EBADMSG`, `EOVERFLOW`, `EILSEQ`, `ESTRPIPE` | `42`..`86` |
| **网络与套接字** | `ENOTSOCK`, `EDESTADDRREQ`, `EMSGSIZE`, `EPROTOTYPE`, `ENOPROTOOPT`, `EPROTONOSUPPORT`, `ESOCKTNOSUPPORT`, `ENOTSUP`, `EPFNOSUPPORT`, `EAFNOSUPPORT`, `EADDRINUSE`, `EADDRNOTAVAIL`, `ENETDOWN`, `ENETUNREACH`, `ENETRESET`, `ECONNABORTED`, `ECONNRESET`, `ENOBUFS`, `EISCONN`, `ENOTCONN`, `ESHUTDOWN`, `ETOOMANYREFS`, `ETIMEDOUT`, `ECONNREFUSED`, `EHOSTDOWN`, `EHOSTUNREACH`, `EALREADY`, `EINPROGRESS` | `88`..`115` |

### 2. 方法列表

| API 签名 | 说明 |
| :--- | :--- |
| `SystemErrno.fromCode(code: i32): SystemErrno` | 将正数错误码或 Linux 负系统调用返回值（如 `-2`、`-111`）转为 `SystemErrno`，未知码返回 `.UNKNOWN` |
| `err.toCode(): i32` | 返回底层数值错误码 |
| `err.name` / `err.name()` / `err.codeName()` | 返回标准 POSIX 符号名（如 `"ENOENT"`、`"ECONNREFUSED"`） |
| `err.description(): []const u8` | 返回人类可读英文 POSIX `strerror` 诊断描述 |
| `err.isOk(): bool` | 是否为 `SUCCESS` (`0`) |
| `err.isRetryable(): bool` | 是否为可重试瞬时错误（`EINTR`, `EAGAIN`, `EWOULDBLOCK`, `EBUSY`, `ETIMEDOUT`, `ERESTART`） |
| `err.isNotFound(): bool` | 是否为目标不存在错误（`ENOENT`, `ESRCH`, `ENODEV`, `ENXIO`, `ENOMEDIUM`） |
| `err.isPermissionDenied(): bool` | 是否为权限拒绝、只读或密钥失效错误（`EPERM`, `EACCES`, `EROFS`, `EKEYEXPIRED`, `EKEYREVOKED`, `EKEYREJECTED`） |
| `err.isWouldBlock(): bool` | 是否为非阻塞 I/O 阻塞提示（`EAGAIN`, `EWOULDBLOCK`） |
| `err.isConnectionError(): bool` | 是否为连接断开、重置、拒绝或网络不可达错误 |
| `err.isTimeout(): bool` | 是否为超时错误（`ETIMEDOUT`, `ETIME`） |
| `err.isAlreadyExists(): bool` | 是否为目标已存在错误（`EEXIST`） |
| `err.isNetworkError(): bool` | 是否为网络与套接字传输层错误（含地址占用、协议不支持、连接与传输错误） |

---

## 三、 可运行代码示例

```typescript
import { SystemErrno } from "cap:errno";

pub fn handleSyscallResult(ret: i32) {
    const e = SystemErrno.fromCode(ret);
    if (e.isOk()) return;

    if (e.isWouldBlock()) {
        // EAGAIN (-11) 或 EWOULDBLOCK (-41) -> 注册 epoll/kqueue 可读/可写事件
        return;
    }
    if (e.isAlreadyExists()) {
        // EEXIST (-17) -> 目标文件已存在
        return;
    }
    if (e.isTimeout()) {
        // ETIMEDOUT (-110) / ETIME (-62) -> 超时重试
        return;
    }
    if (e.isConnectionError()) {
        // ECONNRESET / ECONNREFUSED / EPIPE 等 -> 关闭并重连
        const sym = e.codeName();
        const msg = e.description();
        * = sym;
        * = msg;
    }
}
```

---

## 上游 Bun `errno` 行为契约与跨平台边界

1. **正负 syscall 错误码统一归一化（`SystemErrno.fromCode`）**：
   - Linux 原始系统调用失败返回负值（如 `-11` 表示 `-EAGAIN`），而 C 库 `errno` 为正值（`11`）；`SystemErrno.fromCode(code)` 自动取绝对值并映射至统一的 135 项强类型枚举（未知码安全映射为 `UNKNOWN = 65535`）。
2. **`EAGAIN` 与 `EWOULDBLOCK` 等价语义**：
   - 在 Linux 上 `EAGAIN == EWOULDBLOCK`，而在部分 BSD/macOS 平台二者数值不同；非阻塞 I/O 判断务必统一调用 `e.isWouldBlock()` 而非硬编码单个枚举值。
3. **语义分类谓词指导重试策略**：
   - `isRetryable()`（含 `EINTR`/`EAGAIN`/`EWOULDBLOCK`）适合立即重试或注册事件循环；`isConnectionError()`（`ECONNRESET`/`ECONNREFUSED`/`EPIPE`）提示应关闭失效连接并重建；`isPermissionDenied()` / `isNotFound()` 属确定性错误应直接上报。
4. **macOS / Darwin 原生错误码与跨平台双向桥接（`darwinErrno.cp`）**：
   - 吸收自 `/data/dev/bun/src/errno/darwin_errno.cp`：导出 `DARWIN_MAX_ERRNO`（`107`）与完整 107 项 `DarwinErrno`（`0..106`），精确区分 Linux 与 macOS 差异数值（如 Darwin `EAGAIN=35`、`EADDRINUSE=48`、`ETIMEDOUT=60`、`ECONNREFUSED=61`、`ENOTEMPTY=66`、`ENOSYS=78`）及 macOS 独有 Mach-O / 电源 / 扩展属性错误码（`EBADMACHO`、`EBADARCH`、`ENOATTR`、`EQFULL` 等）；
   - 通过 `DarwinErrno.toSystemErrno()` 与 `DarwinErrno.fromSystemErrno(err)` 可在跨平台日志诊断、远程 macOS 节点回报解析与统一重试逻辑间零开销双向转换。

```cp
import { DarwinErrno, SystemErrno } from "cap:errno";

const d = DarwinErrno.fromCode(-61).?; // Darwin ECONNREFUSED (61)
const canonical = d.toSystemErrno();   // SystemErrno.ECONNREFUSED (111)
* = canonical.isConnectionError();     // true
```

5. **FreeBSD 原生错误码与 Capsicum 沙箱诊断（`freebsdErrno.cp`）**：
   - 吸收自 `/data/dev/bun/src/errno/freebsd_errno.cp`：导出 `FREEBSD_MAX_ERRNO`（`98`）与完整 98 项 `FreebsdErrno`（`0..97`），覆盖 FreeBSD Capsicum 能力模式沙箱专有错误（`ENOTCAPABLE = 93`、`ECAPMODE = 94`，提供 `isCapsicumError()` 谓词）、完整性错误（`EINTEGRITY = 97`）与 `EOPNOTSUPP = 45`（兼容 `"ENOTSUP"` 别名），同样支持 `toSystemErrno()` / `fromSystemErrno(err)` 双向映射。

```cp
import { FreebsdErrno } from "cap:errno";

const cap_err = FreebsdErrno.fromCode(94).?; // ECAPMODE
* = cap_err.isCapsicumError();               // true
const sys = FreebsdErrno.ECANCELED.toSystemErrno(); // SystemErrno.ECANCELED
* = sys;
```
