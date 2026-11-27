[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/usrsctp

Portable userland SCTP stack (usrsctp 0.9.5.0) for WebRTC DataChannel in CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/usrsctp` |
| 版本 | `0.9.5` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/usrsctp
```

```cp
import {
    SctpError,
    SctpSocket,
    cp_usrsctp_init,
    cp_usrsctp_finish,
    cp_usrsctp_socket_create,
    cp_usrsctp_socket_close,
    cp_usrsctp_socket_create_and_close,
    sctpInit,
} from "@cap/usrsctp";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `SctpError` | `InitFailed, FinishFailed, SocketCreateFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SctpSocket` | `class` | 字段: `raw: *auto!?` · 方法: `createStream, isOpen, close, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_usrsctp_init` | `(udp_port: u16): i32` | 调用 `cp_usrsctp_init` 并返回 `i32`。 |
| `cp_usrsctp_finish` | `(): i32` | 调用 `cp_usrsctp_finish` 并返回 `i32`。 |
| `cp_usrsctp_socket_create` | `(domain: i32, sock_type: i32): *auto!?` | 调用 `cp_usrsctp_socket_create` 并返回 `*auto!?`。 |
| `cp_usrsctp_socket_close` | `(sock: *auto!?)` | 就地执行 `cp_usrsctp_socket_close` 操作。 |
| `cp_usrsctp_socket_create_and_close` | `(): i32` | 调用 `cp_usrsctp_socket_create_and_close` 并返回 `i32`。 |
| `sctpInit` | `(udp_port: u16): !void` | 调用 `sctpInit` 并返回 `!void`。 |
| `sctpFinish` | `(): !void` | 调用 `sctpFinish` 并返回 `!void`。 |
| `socketCreateAndClose` | `(): !void` | 调用 `socketCreateAndClose` 并返回 `!void`。 |

## 最小调用示例

```cp
import { cp_usrsctp_init } from "@cap/usrsctp";

pub fn main() {
    let result = cp_usrsctp_init(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
