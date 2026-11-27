[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/libsrtp

Cisco libsrtp2 2.6.0 SRTP/SRTCP encrypted media transport native C binding for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/libsrtp` |
| 版本 | `2.6.0` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/libsrtp
```

```cp
import {
    SrtpError,
    SrtpSession,
    cp_srtp_init,
    cp_srtp_shutdown,
    cp_srtp_version,
    cp_srtp_create_aes128_sha1,
    cp_srtp_protect,
    cp_srtp_unprotect,
} from "@cap/libsrtp";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `SrtpError` | `InitFailed, BadParam, AllocFail, InvalidState, BufferTooSmall, AuthFail, ReplayFail, ProtectFailed ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SrtpSession` | `class` | 字段: `raw: *auto!?, inbound: bool` · 方法: `createAes128Sha1, protect, unprotect, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_srtp_init` | `(): i32` | 调用 `cp_srtp_init` 并返回 `i32`。 |
| `cp_srtp_shutdown` | `(): i32` | 调用 `cp_srtp_shutdown` 并返回 `i32`。 |
| `cp_srtp_version` | `(): [*:0]&u8!?` | 调用 `cp_srtp_version` 并返回 `[*:0]&u8!?`。 |
| `cp_srtp_create_aes128_sha1` | `(key30: [*]&u8!, inbound: i32): *auto!?` | 调用 `cp_srtp_create_aes128_sha1` 并返回 `*auto!?`。 |
| `cp_srtp_protect` | `(session: *auto!?, buf: [*]u8!, len_inout: *i32!): i32` | 调用 `cp_srtp_protect` 并返回 `i32`。 |
| `cp_srtp_unprotect` | `(session: *auto!?, buf: [*]u8!, len_inout: *i32!): i32` | 调用 `cp_srtp_unprotect` 并返回 `i32`。 |
| `cp_srtp_destroy` | `(session: *auto!?)` | 就地执行 `cp_srtp_destroy` 操作。 |
| `mapSrtpStatus` | `(code: i32, is_unprotect: bool): SrtpError` | 调用 `mapSrtpStatus` 并返回 `SrtpError`。 |
| `srtpInit` | `(): !void` | 调用 `srtpInit` 并返回 `!void`。 |
| `srtpShutdown` | `(): !void` | 调用 `srtpShutdown` 并返回 `!void`。 |
| `versionString` | `(): string` | 调用 `versionString` 并返回 `string`。 |

## 最小调用示例

```cp
import { versionString } from "@cap/libsrtp";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
