[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cache-control-parser

Zero-allocation RFC 9111 HTTP Cache-Control parser, formatter, and shared/private TTL calculator (cache-control-parser / http-cache-semantics parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cache-control-parser` ([https://github.com/etienne-martin/cache-control-parser](https://github.com/etienne-martin/cache-control-parser))（原库名 `cache-control-parser` → CP 包名 `cache-control-parser`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `cache-control-parser` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cache-control-parser
```

```cp
import {
    CacheControl,
    unquote,
    parseNonNegI64,
    writeStrChecked,
    writeU64Checked,
    appendDirectiveFlag,
    appendDirectiveKv,
} from "cache-control-parser";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CacheControl` | `class` | 字段: `max_age: i64, s_maxage: i64, stale_while_revalidate: i64, stale_if_error: i64, no_cache: bool, no_store: bool, +5 项` · 方法: `parse, isCacheableShared, effectiveTtl, format` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `unquote` | `(s_in: &[]u8): &[]u8` | 调用 `unquote` 并返回 `&[]u8`。 |
| `parseNonNegI64` | `(s_in: &[]u8): i64` | 通过 `parseNonNegI64` 解析输入数据，返回 `i64`。 |
| `writeStrChecked` | `(dst: []u8, pos: usize, s: string): i64` | 调用 `writeStrChecked` 并返回 `i64`。 |
| `writeU64Checked` | `(dst: []u8, pos: usize, v_in: u64): i64` | 调用 `writeU64Checked` 并返回 `i64`。 |
| `appendDirectiveFlag` | `(dst: []u8, pos_in: usize, written: &mut usize, name: string): i64` | 调用 `appendDirectiveFlag` 并返回 `i64`。 |
| `appendDirectiveKv` | `(dst: []u8, pos_in: usize, written: &mut usize, name: string, val: i64): i64` | 调用 `appendDirectiveKv` 并返回 `i64`。 |

## 最小调用示例

```cp
import { writeStrChecked } from "cache-control-parser";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = writeStrChecked(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
