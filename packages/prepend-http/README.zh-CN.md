[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# prepend-http

面向 CP 语言的零分配 URL 协议前缀补全工具，自动为无协议的简写 URL 补全 `https://` 或 `http://`，对齐 npm [`prepend-http`](https://github.com/sindresorhus/prepend-http) v4。

## 源包对齐说明

- **上游源包**：npm `prepend-http` v4（`sindresorhus/prepend-http`，MIT）
- **CP 实现**：`prepend-http` v1.0.0，纯 CP 净室 MIT 实现
- **处理规则**：先裁剪首尾空白；若已包含符合 RFC 3986 的 `scheme://`、`mailto:`、协议相对前缀 `//` 或相对路径前缀（`./`、`../`、`/`），则原样保留，否则按 `https` 标志补全 `https://` 或 `http://`。

## 安装与导入

```bash
cap add prepend-http
```

```cp
import {
    isProtocolRelative,
    hasScheme,
    prependHttp,
} from "prepend-http";
```

## 核心 API

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `prependHttp` | `fn prependHttp(url: string, https: bool, out: []u8): usize` | 裁剪空白并在缺少协议/相对前缀时补全 `https://` 或 `http://` 写入 `out`，返回写入字节数 |
| `hasScheme` | `fn hasScheme(url: string): bool` | 判断去除空白后的 `url` 是否已含 RFC 3986 `scheme://` 或 `mailto:`（不区分大小写） |
| `isProtocolRelative` | `fn isProtocolRelative(url: string): bool` | 判断去除空白后的 `url` 是否以协议相对前缀 `//` 开头 |

## 使用示例

### 1. 补全 `https://` 与 `http://`

```cp
import { prependHttp } from "prepend-http";
import { print } from "cap:io";

pub fn main() {
    let buf: [128]u8 = [0; 128];

    const n1 = prependHttp("cp-lang.org", true, buf[0..128]);
    print(buf[0..n1] as string); // "https://cp-lang.org"

    const n2 = prependHttp("localhost:3000", false, buf[0..128]);
    print(buf[0..n2] as string); // "http://localhost:3000"
}
```

### 2. 自动保留已有协议与相对路径

```cp
import { prependHttp } from "prepend-http";

pub fn normalizeLink(raw: string, out: []u8): usize {
    return prependHttp(raw, true, out);
    // "http://example.com"  -> "http://example.com"
    // "//cdn.example.com"   -> "//cdn.example.com"
    // "mailto:a@b.com"      -> "mailto:a@b.com"
    // "./docs/index.html"   -> "./docs/index.html"
}
```

### 3. 预检是否缺少协议

```cp
import { hasScheme, isProtocolRelative } from "prepend-http";

pub fn needsProtocol(url: string): bool {
    return !hasScheme(url) && !isProtocolRelative(url);
}
```

## 与源包差异与安全亮点

- **显式 `https` 布尔参数**：用显式 `https: bool` 替代 JS 可选配置对象，调用简洁且零运行期开销。
- **零堆缓冲区写入**：直接写入调用方 `[]u8` 缓冲区，最大输出长度不超过 `url.len + 8`。
- **空串安全**：空字符串或全空白输入直接返回 `0`，不抛出异常。
