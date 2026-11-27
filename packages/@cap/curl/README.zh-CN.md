[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/curl

libcurl (v8.23.0) multiprotocol network transfer library (HTTP/FTP/Proxy/Multipart) for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`curl`](https://github.com/curl/curl)（`curl`）（包名 `@cap/curl`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/curl` |
| 版本 | `8.23.0` |
| 许可证 | `curl` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/curl
```

```cp
import {
    CurlError,
    CurlResponse,
    cp_curl_version,
    cp_curl_global_init,
    cp_curl_global_cleanup,
    cp_curl_http_get,
    cp_curl_free,
    version,
} from "@cap/curl";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `CurlError` | `InitFailed, UrlTooLong, RequestFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CurlResponse` | `class` | 字段: `status: i64, ptr: [*]u8!?, len: usize` · 方法: `statusCode, body, length, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_curl_version` | `(): [*:0]&u8!` | 调用 `cp_curl_version` 并返回 `[*:0]&u8!`。 |
| `cp_curl_global_init` | `(): i32` | 调用 `cp_curl_global_init` 并返回 `i32`。 |
| `cp_curl_global_cleanup` | `()` | 就地执行 `cp_curl_global_cleanup` 操作。 |
| `cp_curl_http_get` | `(url: [*:0]&u8!, out_status: &mut i64, out_len: &mut usize): [*]u8!?` | 调用 `cp_curl_http_get` 并返回 `[*]u8!?`。 |
| `cp_curl_free` | `(ptr: [*]u8!)` | 就地执行 `cp_curl_free` 操作。 |
| `version` | `(): string` | 调用 `version` 并返回 `string`。 |
| `globalInit` | `(): !void` | 调用 `globalInit` 并返回 `!void`。 |
| `globalCleanup` | `()` | 就地执行 `globalCleanup` 操作。 |
| `httpGet` | `(url: string): !CurlResponse` | 调用 `httpGet` 并返回 `!CurlResponse`。 |

## 最小调用示例

```cp
import { httpGet } from "@cap/curl";

pub fn main() {
    let result = httpGet("example") catch return;
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
