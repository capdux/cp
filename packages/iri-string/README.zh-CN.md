[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# iri-string

> **源包溯源**：吸收自 **crates.io** [`iri-string`](https://github.com/lo48576/iri-string)（`MIT`），CP 官方洁净室实现 `iri-string@1.0.0`。

纯 CP 实现的零堆分配 RFC 3986 / RFC 3987 URI & IRI 组件解析器、点段规范化器与相对引用解析器。

## 核心亮点

- **零堆分配零拷贝解析**：`parseIri` 直接返回原始字符串的字节偏移与长度（`IriParts`）；`removeDotSegments` 与 `resolveReference` 直接写入调用方 `[]u8` 缓冲。
- **零栈清零优化**：`resolveReference` 内部路径合并缓冲区（两个 `[512]u8`）使用 `undefined` 初始化，消除每次调用 1KB 的冗余栈清零。
- **严格符合 RFC 3986 §5.2**：完整实现 `remove_dot_segments`（`.`、`..` 及尾斜杠处理）和相对引用解析（覆盖 scheme、authority、path、query、fragment 全组合）。

## 零成本迁移（Rust → CP）

直接沿用原包名 `iri-string`，无需改名：

**Rust：**
```rust
use iri_string::types::{IriStr, IriReferenceStr};
let resolved = rel.resolve_against(base);
```

**CP：**
```typescript
import {
    IriParts, parseIri, removeDotSegments, resolveReference
} from "iri-string";
```

## 实战示例

### 1. 零拷贝解析 URI / IRI 各组件

```typescript
import { parseIri } from "iri-string";

export fn inspectUrl(url: string): bool {
    let p = parseIri(url);
    if (!p.valid || !p.has_scheme) {
        return false;
    }
    let scheme = url[p.scheme_start..(p.scheme_start + p.scheme_len)];
    let path = url[p.path_start..(p.path_start + p.path_len)];
    _ = scheme;
    _ = path;
    return p.has_auth;
}
```

### 2. 相对引用解析与点段消除

```typescript
import { resolveReference, removeDotSegments } from "iri-string";

export fn demoResolve(): usize {
    let mut out: [512]u8 = undefined;
    // "http://a/b/c/d;p?q" + "../g" -> "http://a/b/g"
    let n1 = resolveReference("http://a/b/c/d;p?q", "../g", out[0..512]);

    let mut clean: [256]u8 = undefined;
    // "/a/b/c/./../../g" -> "/a/g"
    let n2 = removeDotSegments("/a/b/c/./../../g", clean[0..256]);
    return n1 + n2;
}
```

## 核心 API 速览

- **`IriParts`**：包含 `scheme/auth/path/query/frag` 的 `*_start` 与 `*_len` 偏移字段，以及 `has_scheme`、`has_auth`、`has_query`、`has_frag`、`valid` 布尔标志。
- **`parseIri(s: string): IriParts`**：零拷贝 RFC 3986/3987 解析。
- **`removeDotSegments(path: string, out: []u8): usize`**：RFC 3986 §5.2.4 路径点段消除。
- **`resolveReference(base: string, rel: string, out: []u8): usize`**：RFC 3986 §5.2.2 相对引用解析（非法或缓冲区溢出返回 `0`）。

## 许可证

MIT
