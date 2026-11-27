[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# encodeurl

纯 CP 实现的零堆分配幂等 URL 编码器、规范化百分号编码器、解码器与校验器，对不安全字符进行百分号编码且不会对已有的 `%XX` 序列二次编码。

- **包名**：`encodeurl`（`v1.0.0`）
- **上游对齐**：洁净室移植自 `npm/encodeurl`（`MIT`，`pillarjs/encodeurl`），并扩展了缓冲区检查版本、`%xx` 大写规范化及 URL 解码函数。

## 安装与导入

```bash
cap add encodeurl
```

```typescript
import {
    needsUrlEncode, encodedUrlLen,
    encodeUrl, encodeUrlChecked, encodeUrlNormalized,
    decodeUrl, isEncodedUrl,
} from "encodeurl";
```

## 核心 API

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `needsUrlEncode` | `(b: u8): bool` | 判断字节 `b` 是否需要百分号编码（控制字符、空格、非 ASCII、`"`, `<`, `>`, `\`, `^`, `` ` ``, `{`, `\|`, `}`）。 |
| `encodedUrlLen` | `(url: string): usize` | 预计算 `encodeUrl(url, out)` 所需的精确输出字节数。 |
| `encodeUrl` | `(url: string, out: []u8): usize` | 幂等百分号编码并保留合法 `%XX` 序列，写入 `out` 并返回写入字节数。 |
| `encodeUrlChecked` | `(url: string, out: []u8): usize?` | 同 `encodeUrl`，但当 `out.len < encodedUrlLen(url)` 时返回 `null`。 |
| `encodeUrlNormalized` | `(url: string, out: []u8): usize` | 编码 URL 并将已有 `%xx` 中的十六进制字母规范为大写（如 `%2f` -> `%2F`）。 |
| `decodeUrl` | `(url: string, out: []u8): usize` | 将合法 `%XX` 序列解码回原始字节写入 `out`，非法 `%` 原样保留。 |
| `isEncodedUrl` | `(url: string): bool` | 判断 `url` 是否不含待编码字符且所有 `%` 均为合法 `%XX` 序列。 |

## 使用示例

### 1. 幂等 URL 编码（适用于 HTTP `Location` 响应头）

```typescript
import { encodeUrl, isEncodedUrl } from "encodeurl";

fn main() {
    let raw = "/files/my doc%20(1).pdf?q=a|b";
    let buf: [128]u8 = undefined;
    let n = encodeUrl(raw, buf[0..]);
    // buf[0..n] == "/files/my%20doc%20(1).pdf?q=a%7Cb"（已有的 %20 不会被二次编码）
    let ok = isEncodedUrl(buf[0..n]); // true
}
```

### 2. 带容量检查的编码与十六进制大写规范化

```typescript
import { encodeUrlChecked, encodeUrlNormalized } from "encodeurl";

fn main() {
    let out: [64]u8 = undefined;
    if (let n = encodeUrlChecked("/hello world", out[0..])) {
        // out[0..n] == "/hello%20world"
    }

    let norm: [64]u8 = undefined;
    let m = encodeUrlNormalized("/a%2fb c", norm[0..]);
    // norm[0..m] == "/a%2Fb%20c"
}
```

### 3. 百分号解码

```typescript
import { decodeUrl } from "encodeurl";

fn main() {
    let out: [64]u8 = undefined;
    let n = decodeUrl("/hello%20world%21", out[0..]);
    // out[0..n] == "/hello world!"
}
```

## 与源包差异及零拷贝安全亮点

- **全程零堆分配**：所有编码/解码结果直接写入调用方栈切片 `out: []u8`，适合高并发 HTTP 路由热路径。
- **严格幂等性**：与 `pillarjs/encodeurl` 一致，保留已有合法 `%XX` 序列，同时将孤立 `%` 编码为 `%25`。
- **完整工具集**：补充了 `encodeUrlChecked`（返回 `usize?`）、`encodeUrlNormalized`、`decodeUrl`、`encodedUrlLen` 与 `isEncodedUrl`。
