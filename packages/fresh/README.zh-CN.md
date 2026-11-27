[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fresh

Zero-allocation RFC 9110 HTTP conditional request freshness testing (ETag, If-None-Match, If-Modified-Since, Cache-Control) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fresh` ([https://github.com/jshttp/fresh](https://github.com/jshttp/fresh))

| 元信息 | 值 |
| --- | --- |
| 包名 | `fresh` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fresh
```

```cp
import {
    RequestHeaders,
    ResponseHeaders,
    TrimRange,
    ParsedEtag,
    trimRange,
    hasNoCache,
    parseEtagSlice,
    etagWeakMatch,
} from "fresh";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RequestHeaders` | `class` | 字段: `if_none_match: &[]u8, if_modified_since: &[]u8, cache_control: &[]u8` |
| `ResponseHeaders` | `class` | 字段: `etag: &[]u8, last_modified: &[]u8` |
| `TrimRange` | `class` | 字段: `start: usize, end: usize` |
| `ParsedEtag` | `class` | 字段: `start: usize, end: usize, weak: bool, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `trimRange` | `(s: &[]u8, start: usize, end: usize): TrimRange` | 调用 `trimRange` 并返回 `TrimRange`。 |
| `hasNoCache` | `(cache_control: &[]u8): bool` | 执行 `hasNoCache` 条件判定，返回 `bool`。 |
| `parseEtagSlice` | `(s: &[]u8, raw_start: usize, raw_end: usize): ParsedEtag` | 通过 `parseEtagSlice` 解析输入数据，返回 `ParsedEtag`。 |
| `etagWeakMatch` | `(a: &[]u8, b: &[]u8): bool` | 调用 `etagWeakMatch` 并返回 `bool`。 |
| `etagStrongMatch` | `(a: &[]u8, b: &[]u8): bool` | 调用 `etagStrongMatch` 并返回 `bool`。 |
| `parse2Digits` | `(s: &[]u8, pos: usize): i32` | 通过 `parse2Digits` 解析输入数据，返回 `i32`。 |
| `parse4Digits` | `(s: &[]u8, pos: usize): i32` | 通过 `parse4Digits` 解析输入数据，返回 `i32`。 |
| `parseMonth` | `(s: &[]u8, pos: usize): i32` | 通过 `parseMonth` 解析输入数据，返回 `i32`。 |
| `isValidWeekday` | `(s: &[]u8, pos: usize): bool` | 执行 `isValidWeekday` 条件判定，返回 `bool`。 |
| `isLeapYear` | `(y: u64): bool` | 执行 `isLeapYear` 条件判定，返回 `bool`。 |
| `daysInMonth` | `(month: i32, leap: bool): i32` | 调用 `daysInMonth` 并返回 `i32`。 |
| `parseHttpDate` | `(date_str: &[]u8): i64` | 通过 `parseHttpDate` 解析输入数据，返回 `i64`。 |
| `matchIfNoneMatch` | `(inm: &[]u8, res_etag: &[]u8): bool` | 调用 `matchIfNoneMatch` 并返回 `bool`。 |
| `isFresh` | `(req: RequestHeaders, res: ResponseHeaders): bool` | 执行 `isFresh` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { parseEtagSlice } from "fresh";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseEtagSlice(out_buf[0..256], 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
