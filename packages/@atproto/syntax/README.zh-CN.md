[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @atproto/syntax

Zero-allocation AT Protocol / Bluesky at:// URI, NSID, and Record Key (rkey) validator, parser, and formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@atproto/syntax` ([https://github.com/bluesky-social/atproto](https://github.com/bluesky-social/atproto)) / `crates.io:atrium-api` ([https://github.com/sugyan/atrium](https://github.com/sugyan/atrium))（原库名 `@atproto/syntax` → CP 包名 `@atproto/syntax`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `@atproto/syntax` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @atproto/syntax
```

```cp
import {
    AtUri,
    invalidAtUri,
    isLowerAlpha,
    isAlpha,
    isDigit,
    isAlphaNum,
    isValidNsid,
    isValidRkey,
} from "@atproto/syntax";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `AtUri` | `class` | 字段: `authority: string, collection: string, rkey: string, is_did: bool, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidAtUri` | `(): AtUri` | 调用 `invalidAtUri` 并返回 `AtUri`。 |
| `isLowerAlpha` | `(b: u8): bool` | 执行 `isLowerAlpha` 条件判定，返回 `bool`。 |
| `isAlpha` | `(b: u8): bool` | 执行 `isAlpha` 条件判定，返回 `bool`。 |
| `isDigit` | `(b: u8): bool` | 执行 `isDigit` 条件判定，返回 `bool`。 |
| `isAlphaNum` | `(b: u8): bool` | 执行 `isAlphaNum` 条件判定，返回 `bool`。 |
| `isValidNsid` | `(nsid: string): bool` | 执行 `isValidNsid` 条件判定，返回 `bool`。 |
| `isValidRkey` | `(rkey: string): bool` | 执行 `isValidRkey` 条件判定，返回 `bool`。 |
| `isValidDid` | `(s: string): bool` | 执行 `isValidDid` 条件判定，返回 `bool`。 |
| `isValidHandle` | `(s: string): bool` | 执行 `isValidHandle` 条件判定，返回 `bool`。 |
| `parseAtUri` | `(uri: string): AtUri` | 通过 `parseAtUri` 解析输入数据，返回 `AtUri`。 |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | 写入或更新状态数据。 |
| `formatAtUri` | `(authority: string, collection: string, rkey: string, out: []u8): usize` | 通过 `formatAtUri` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isValidNsid } from "@atproto/syntax";

pub fn main() {
    let result = isValidNsid("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
