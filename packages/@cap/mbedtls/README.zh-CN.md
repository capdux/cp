[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/mbedtls

ARM Mbed TLS 3.6.2 core cryptography suite (SHA-256/512, AES-GCM, ChaCha20-Poly1305, Base64) for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/mbedtls` |
| 版本 | `3.6.2` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/mbedtls
```

```cp
import {
    MbedTlsError,
    cp_mbedtls_version,
    cp_mbedtls_sha256,
    cp_mbedtls_aes_gcm_roundtrip_test,
    versionInto,
    sha256,
    aesGcmRoundtripTest,
} from "@cap/mbedtls";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `MbedTlsError` | `BufferTooSmall, HashFailed, AesGcmFailed` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_mbedtls_version` | `(buf: [*]u8!, cap: usize): usize` | 调用 `cp_mbedtls_version` 并返回 `usize`。 |
| `cp_mbedtls_sha256` | `(data: [*]&u8!?, len: usize, out32: [*]u8!): i32` | 调用 `cp_mbedtls_sha256` 并返回 `i32`。 |
| `cp_mbedtls_aes_gcm_roundtrip_test` | `(): i32` | 调用 `cp_mbedtls_aes_gcm_roundtrip_test` 并返回 `i32`。 |
| `versionInto` | `(buf: []u8): !string` | 调用 `versionInto` 并返回 `!string`。 |
| `sha256` | `(data: &[]u8): ![32]u8` | 调用 `sha256` 并返回 `![32]u8`。 |
| `aesGcmRoundtripTest` | `(): bool` | 调用 `aesGcmRoundtripTest` 并返回 `bool`。 |

## 最小调用示例

```cp
import { versionInto } from "@cap/mbedtls";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = versionInto(out_buf[0..256]) catch return;
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
