[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# content-security-policy-builder

面向 CP 语言的零堆分配 W3C Content-Security-Policy Level 3 HTTP 响应头构建器、合法性校验器与零拷贝策略解析器。

对齐 npm 源包 [`content-security-policy-builder`](https://github.com/helmetjs/content-security-policy-builder)（MIT）。

## 安装与导入

```bash
cap add content-security-policy-builder
```

```cp
import {
    CspBuilder,
    CspDirectiveSpan,
    CspPolicy,
} from "content-security-policy-builder";
```

## 核心 API

### `CspBuilder` 构建器类

| 成员 | 签名 | 说明 |
| --- | --- | --- |
| 字段 | `buf: []u8`, `pos: usize`, `count: usize`, `overflow: bool` | 调用方输出缓冲区、写入偏移、指令计数与溢出标志。 |
| `CspBuilder.init` | `(buf: []u8): CspBuilder` | 在 `buf` 上初始化 CSP 响应头构建器。 |
| `addDirective` | `(*self, name: &[]u8, sources: &[]&[]u8): bool` | 校验指令名与源值（拦截 `;`、`,`、CR/LF 等非法字符），以 `; ` 拼接写入；输入非法或溢出时返回 `false`。 |
| `written` | `(): &[]u8` | 返回已构建的 CSP 头部只读字节切片。 |
| `len` | `(): usize` | 返回已写入的字节长度。 |
| `isOverflow` | `(): bool` | 若写入过程发生缓冲区溢出则返回 `true`。 |

### `CspPolicy` 与 `CspDirectiveSpan` 解析类

| 成员 | 签名 | 说明 |
| --- | --- | --- |
| `CspDirectiveSpan` | `{ name_start, name_len, val_start, val_len: usize }` | 单条解析指令在原始头部中的零拷贝字节区间。 |
| `CspPolicy.parse` | `(header: &[]u8): CspPolicy` | 零堆分配解析 CSP 头部字符串（最多 32 条指令）。 |
| `len` | `(): usize` | 返回解析出的指令条数。 |
| `getDirective` | `(header: &[]u8, name: &[]u8): &[]u8` | 忽略大小写查询指定指令的值切片（不存在返回空切片）。 |
| `hasDirective` | `(header: &[]u8, name: &[]u8): bool` | 检查指定指令是否存在（忽略大小写）。 |
| `allowsUnsafeInline` | `(header: &[]u8, directive: &[]u8): bool` | 检查指定指令（缺失时回退至 `default-src`）是否允许 `'unsafe-inline'`。 |

## 使用示例

### 1. 构建严格 CSP 响应头

```cp
import { print } from "cap:io";
import { CspBuilder } from "content-security-policy-builder";

pub fn main() {
    let buf: [512]u8 = undefined;
    let b = CspBuilder.init(buf[0..512]);
    let def_srcs: [1][]const u8 = ["'self'"];
    let img_srcs: [2][]const u8 = ["'self'", "https://cdn.example.com"];

    let _ = b.addDirective("default-src", def_srcs[0..1]);
    let _ = b.addDirective("img-src", img_srcs[0..2]);

    if (!b.isOverflow()) {
        print("Content-Security-Policy: {}\n", b.written());
    }
}
```

### 2. 解析并审计已有 CSP 策略头

```cp
import { print } from "cap:io";
import { CspPolicy } from "content-security-policy-builder";

pub fn main() {
    let header = "default-src 'self'; script-src 'self' 'unsafe-inline'";
    let policy = CspPolicy.parse(header);
    print("directives={}\n", policy.len());
    print("script-src={}\n", policy.getDirective(header, "script-src"));
    print("allows unsafe-inline={}\n", policy.allowsUnsafeInline(header, "script-src"));
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配**：构建直接写入调用方 `[]u8` 缓冲区，解析使用固定栈数组（`[32]CspDirectiveSpan`），全程零动态分配。
- **内置解析与安全审计**：除对齐 npm 原包的构建能力外，补充 `CspPolicy.parse`、`getDirective`、`hasDirective` 与 `allowsUnsafeInline`。
- **畸形报文校验**：`addDirective` 严格验证指令名（ASCII 字母数字与 `-`）并拒绝含 `;`、`,` 或控制字符的源值，防止响应头格式破坏。
