[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# simdutf8

面向 CP 的零堆分配严格 RFC 3629 UTF-8 校验器，内置 16-lane 硬件 SIMD ASCII 快速通道（对齐 Rust `simdutf8`）。

基于 `@Vector(16, u8)` 高位掩码并行跳过纯 ASCII 数据，严格拒绝过长编码（overlong）、UTF-16 代理项（`U+D800..U+DFFF`）、超出 `U+10FFFF` 的码点及截断序列，同时提供极速布尔接口（`basic`）与精确错误定位接口（`compat`：`valid_up_to` / `has_error_len` / `error_len`）。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **crates.io** |
| 原包名与版本 | `simdutf8`（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/rusticstuff/simdutf8` |
| 开源协议 | MIT（上游：MIT OR Apache-2.0） |

从 Rust 迁移至 CP 保持原包名，零迁移成本：

```rust
// Rust (crates.io: simdutf8)
let ok = simdutf8::basic::from_utf8(bytes).is_ok();
if let Err(e) = simdutf8::compat::from_utf8(bytes) {
    let valid_up_to = e.valid_up_to();
    let error_len = e.error_len();
}
```

```cp
// CP (cap: simdutf8) — 16-lane SIMD ASCII 快速通道，零堆分配
import { basicIsValidUtf8, validateUtf8Compat } from "simdutf8";
const ok = basicIsValidUtf8(bytes);
const res = validateUtf8Compat(bytes);
if (!res.valid) {
    // res.valid_up_to, res.has_error_len, res.error_len
}
```

## 安装

```bash
cap add simdutf8
```

## 实战示例

### 基础用法：极速 UTF-8 / ASCII 校验与码点计数

```cp
import { isAscii, basicIsValidUtf8, countCodepoints } from "simdutf8";

let ascii: [11]u8 = ['h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'];
const pureAscii = isAscii(ascii[0..11]);      // true（16-lane SIMD）
const valid = basicIsValidUtf8(ascii[0..11]); // true

// 中文 UTF-8 "你好"（6 字节 -> 2 个 Unicode 码点）
let zh: [6]u8 = [0xE4, 0xBD, 0xA0, 0xE5, 0xA5, 0xBD];
if (n := countCodepoints(zh[0..6])) {
    // n == 2
}
```

### 安全与边界用法：流式分片截断与畸形序列识别

```cp
import { validateUtf8Compat } from "simdutf8";

// 1. 流式分片尾部被截断的 3 字节序列（"hi" + 0xE4, 0xBD）
let partial: [4]u8 = ['h', 'i', 0xE4, 0xBD];
const r1 = validateUtf8Compat(partial[0..4]);
// r1.valid == false, r1.valid_up_to == 2, r1.has_error_len == false（尾部不完整，等待下一分片）

// 2. 拒绝非法 UTF-16 代理项 U+D800（ED A0 80）与过长编码（C0 AF）
let surrogate: [3]u8 = [0xED, 0xA0, 0x80];
const r2 = validateUtf8Compat(surrogate[0..3]);
// r2.valid == false, r2.valid_up_to == 0, r2.has_error_len == true, r2.error_len == 1
```

## API 速查表

| 导出符号 | 签名 | 说明 |
| --- | --- | --- |
| `Utf8Error` | `class { valid_up_to: usize, has_error_len: bool, error_len: usize }` | 对齐 Rust `Utf8Error` 的错误结构。 |
| `Utf8CompatResult` | `class { valid: bool, valid_up_to: usize, has_error_len: bool, error_len: usize }` | 完整校验结果，携带合法前缀长度与错误字节数。 |
| `isAscii` | `fn(buf: []u8): bool` | 16-lane SIMD 判断是否全部字节 `< 0x80`。 |
| `basicIsValidUtf8` | `fn(buf: []u8): bool` | 严格 RFC 3629 UTF-8 布尔校验。 |
| `validateUtf8Compat` | `fn(buf: []u8): Utf8CompatResult` | 返回精确 `valid_up_to` 与 `error_len` 的兼容校验。 |
| `countCodepoints` | `fn(buf: []u8): usize?` | 先校验 UTF-8 再以 16-lane SIMD 统计码点数，非法返回 `null`。 |

## 安全与性能

- **16-lane SIMD ASCII 快车道**：使用 `@Vector(16, u8)` 与 `0x80` 掩码单次验证 16 字节纯 ASCII。
- **严格 RFC 3629 防护**：完整拦截过长编码（`C0..C1`、`E0 80..9F`、`F0 80..8F`）、代理区（`ED A0..BF`）及越界码点（`F4 90..BF`、`F5..FF`）。

## 协议

MIT — 对齐 Rust `simdutf8` 的纯 CP 净室实现。
