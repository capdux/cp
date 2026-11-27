[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# fast-uri

纯 CP 实现的零堆分配 **RFC 3986 URI** 解析器、序列化器、相对路径解析器（Section 5.2 点段消解）与规范等价比较器。

## 源包溯源与零成本迁移

| 字段 | 说明 |
| :--- | :--- |
| **源平台** | [npm](https://www.npmjs.com/package/fast-uri) |
| **原包名与版本** | `fast-uri`（v3 对齐，Fastify 生态核心组件） |
| **上游仓库** | https://github.com/fastify/fast-uri |
| **CP 包名与版本** | `fast-uri`（`1.0.0`） |
| **开源协议** | BSD-3-Clause |

在 CP 中保持原包名直接导入，无需改名：

```typescript
// TypeScript (npm)
import * as URI from "fast-uri";

// CP (cap)
import { UriComponents, parseUri, serializeUri, resolveUri, equalUri } from "fast-uri";
```

## 安装

```bash
cap add fast-uri
```

## 核心特性与性能亮点

- **零堆分配**：`parseUri` 返回指向原输入字符串的零拷贝切片视图；`serializeUri` 与 `resolveUri` 直接写入调用方提供的 `[]u8` 缓冲区。
- **严格遵循 RFC 3986**：支持 IPv6 方括号主机（`[::1]:8080`）、空查询/空锚点区分（`has_query` / `has_fragment`）、端口范围校验（`0..65535`）与 Section 5.2.4 点段规范化。
- **规范化等价比较**：`equalUri(a, b)` 自动小写化 scheme/host、大写化百分号编码、消除默认端口（`http:80`、`https:443`、`ws:80`、`wss:443`、`ftp:21`）及消解 `.`/`..` 路径段。

## 快速上手

```typescript
import { parseUri, serializeUri, resolveUri, equalUri } from "fast-uri";

fn main(): void {
    const uri = parseUri("https://user@Example.COM:443/a/b/../c?k=1#sec");
    if (!uri.valid) {
        return;
    }

    let ser_buf: [256]u8 = undefined;
    const ser_len = serializeUri(uri, ser_buf[0..]);
    _ = ser_buf[0..ser_len];

    let res_buf: [256]u8 = undefined;
    const res_len = resolveUri("https://example.com/api/v1/users", "../items?limit=10", res_buf[0..]);
    // 结果: "https://example.com/api/items?limit=10"
    _ = res_len;

    const same = equalUri("HTTP://Example.COM:80/a/./b", "http://example.com/a/b");
    // same == true
    _ = same;
}
```

## API 速览

| 导出符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `UriComponents` | `{ valid, scheme, userinfo, host, port, path, query, has_query, fragment, has_fragment }` | 解析后的 URI 组件（缺省端口为 `-1`） |
| `parseUri` | `(input: string) -> UriComponents` | 零拷贝 RFC 3986 URI 解析 |
| `serializeUri` | `(c: UriComponents, out: []u8) -> usize` | 将 `UriComponents` 序列化至 `out` |
| `resolveUri` | `(base: string, rel: string, out: []u8) -> usize` | 解析相对引用并执行点段消解 |
| `equalUri` | `(a: string, b: string) -> bool` | RFC 3986 规范化等价判定 |
| `furiRemoveDotSegments` | `(input: string, out: []u8) -> usize` | 路径 `.`/`..` 点段消解 |

## 开源协议

BSD-3-Clause
