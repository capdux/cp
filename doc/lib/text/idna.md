# `cap:idna` — RFC 3492 Punycode 与 IDNA 国际化域名标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

> **模块路径**: `lib/idna/`  
> **导入协议**: `import { IdnaError, punycodeEncode, punycodeDecode, toAscii, toUnicode, isValidDomain, normalizeDomain } from "cap:text";`  
> **动态挂载**: `import cap from "cap";` -> `cap.idna.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:idna` 是纯 CP 实现、零 C 外部依赖（无 `std.posix` / `@import("bun")`）的国际化域名（IDNA）与 RFC 3492 Punycode 编解码及校验库，包含：

1. **RFC 3492 Bootstring / Punycode 编解码 (`lib/idna/punycode.cp`)**：
   - 严格遵循 RFC 3492 参数（`BASE = 36`, `TMIN = 1`, `TMAX = 26`, `SKEW = 38`, `DAMP = 700`, `INITIAL_BIAS = 72`, `INITIAL_N = 128`）。
   - 内置严格 UTF-8 解码与编码（拦截过长编码、孤立代理对 `0xD800..0xDFFF` 与越界码点）。
   - 全路径整数溢出保护（`IdnaError.Overflow`）与截断/非法变长数字拦截（`IdnaError.InvalidPunycode`）。
2. **IDNA 域名转换与 RFC 1034/1123 标签校验 (`lib/idna/idna.cp`)**：
   - 自动识别 `.`（ASCII `0x2E`）及等价全角/半角分隔符 `\u{3002}`、`\u{FF0E}`、`\u{FF61}`，并支持保留可选尾随根点 `.`。
   - 强制执行单标签 `<= 63` 字节（`LabelTooLong`）、总域名 `<= 253` 字节（`DomainTooLong`，不含可选尾随根点）、首尾连字符禁止（`LeadingOrTrailingHyphen`）、空标签禁止（`EmptyLabel`）、ASCII 标签字符集 `[a-zA-Z0-9-]` 校验（`InvalidChar`）与显式双向控制字符/混排防护（`DisallowedBidirection`）。
   - `isValidDomain` 使用固定栈缓冲区执行零堆分配校验。

---

## 二、 核心 API 速查表

| 符号 / 函数 | 签名与说明 |
| :--- | :--- |
| `IdnaError` | `InvalidUtf8`, `Overflow`, `InvalidPunycode`, `LabelTooLong`, `DomainTooLong`, `EmptyLabel`, `InvalidChar`, `LeadingOrTrailingHyphen`, `DisallowedBidirection`, `OutOfMemory` |
| `punycodeEncode(alloc, utf8_label)` | 将 UTF-8 标签编码为 RFC 3492 Punycode ASCII 串（不含 `xn--` 前缀；调用方负责 `alloc.free`） |
| `punycodeDecode(alloc, ascii_punycode)` | 将 Punycode ASCII 串解码为 UTF-8 标签（调用方负责 `alloc.free`） |
| `toAscii(alloc, domain)` | 将国际化域名转换为小写 ASCII A-label 形式（非 ASCII 标签转为 `xn--...`） |
| `toUnicode(alloc, domain)` | 将域名中的 `xn--`（大小写无关）标签解码为规范化 UTF-8 U-label 形式 |
| `isValidDomain(domain)` | 零堆分配校验域名是否符合 IDNA 与 RFC 1034/1123 规范 |
| `normalizeDomain(alloc, domain)` | 小写规范化并统一等价句号分隔符为 `.`，同时执行全量标签合法性校验 |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { toAscii, toUnicode, isValidDomain, normalizeDomain } from "cap:text";

fn demo(alloc: std.mem.Allocator): !void {
    const ascii = toAscii(alloc, "München\u{3002}de")?;
    defer alloc.free(ascii); // "xn--mnchen-3ya.de"

    const uni = toUnicode(alloc, ascii)?;
    defer alloc.free(uni);   // "münchen.de"

    const norm = normalizeDomain(alloc, "Example\u{FF0E}COM")?;
    defer alloc.free(norm);  // "example.com"

    const ok = isValidDomain("例え.jp"); // true
    _ = ok;
}
```
