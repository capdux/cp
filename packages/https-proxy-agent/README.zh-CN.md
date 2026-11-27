[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# https-proxy-agent

Zero-heap HTTP/HTTPS proxy URL parser and RFC 9110 CONNECT tunnel request builder (covers http-proxy-agent)

| 元信息 | 值 |
| --- | --- |
| 包名 | `https-proxy-agent` |
| 版本 | `7.0.6` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add https-proxy-agent
```

```cp
import {
    ProxyEndpoint,
    hpaParsePort,
    parseProxyUrl,
    hpaWriteBase64,
    buildConnectRequest,
} from "https-proxy-agent";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ProxyEndpoint` | `class` | 字段: `scheme: string, host: string, port: u16, username: string, password: string, has_auth: bool, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hpaParsePort` | `(s: string): u16?` | 调用 `hpaParsePort` 并返回 `u16?`。 |
| `parseProxyUrl` | `(url: string): ProxyEndpoint` | 通过 `parseProxyUrl` 解析输入数据，返回 `ProxyEndpoint`。 |
| `hpaWriteBase64` | `(input: &[]u8, out: []u8, pos: usize): usize` | 调用 `hpaWriteBase64` 并返回 `usize`。 |
| `buildConnectRequest` | `(target_host: string, target_port: u16, proxy: ProxyEndpoint, out: []u8): usize` | 调用 `buildConnectRequest` 并返回 `usize`。 |

## 最小调用示例

```cp
import { parseProxyUrl } from "https-proxy-agent";

pub fn main() {
    let result = parseProxyUrl("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
