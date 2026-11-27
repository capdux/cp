[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# etag

Zero-allocation RFC 9110 HTTP ETag generator, parser, and If-None-Match / If-Match evaluator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:etag` ([https://github.com/jshttp/etag](https://github.com/jshttp/etag))

| 元信息 | 值 |
| --- | --- |
| 包名 | `etag` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add etag
```

```cp
import {
    EntityTag,
    etgIsSpace,
    etgIsEtagChar,
    etgStrEq,
    parseEtag,
    formatEtag,
    strongEqual,
    weakEqual,
} from "etag";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `EntityTag` | `class` | 字段: `weak: bool, tag: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `etgIsSpace` | `(b: u8): bool` | 调用 `etgIsSpace` 并返回 `bool`。 |
| `etgIsEtagChar` | `(b: u8): bool` | 调用 `etgIsEtagChar` 并返回 `bool`。 |
| `etgStrEq` | `(a: string, b: string): bool` | 调用 `etgStrEq` 并返回 `bool`。 |
| `parseEtag` | `(input: string, out: &mut EntityTag): bool` | 通过 `parseEtag` 解析输入数据，返回 `bool`。 |
| `formatEtag` | `(weak: bool, tag: string, out: []u8): usize` | 通过 `formatEtag` 编码或格式化输出，返回 `usize`。 |
| `strongEqual` | `(a: EntityTag, b: EntityTag): bool` | 调用 `strongEqual` 并返回 `bool`。 |
| `weakEqual` | `(a: EntityTag, b: EntityTag): bool` | 调用 `weakEqual` 并返回 `bool`。 |
| `etgHexLower` | `(nibble: u8): u8` | 调用 `etgHexLower` 并返回 `u8`。 |
| `etgWriteHexU64` | `(val: u64, out: []u8, start: usize): usize` | 调用 `etgWriteHexU64` 并返回 `usize`。 |
| `etgHexLenU64` | `(val: u64): usize` | 调用 `etgHexLenU64` 并返回 `usize`。 |
| `generateStatEtag` | `(size: u64, mtime_ms: u64, out: []u8): usize` | 调用 `generateStatEtag` 并返回 `usize`。 |
| `etgFnv1a64` | `(data: &[]u8): u64` | 调用 `etgFnv1a64` 并返回 `u64`。 |
| `generateContentEtag` | `(data: &[]u8, weak: bool, out: []u8): usize` | 调用 `generateContentEtag` 并返回 `usize`。 |
| `etgMatchHeaderList` | `(header: string, current: EntityTag, require_strong: bool): bool` | 调用 `etgMatchHeaderList` 并返回 `bool`。 |
| `matchIfNoneMatch` | `(header: string, current: EntityTag): bool` | 调用 `matchIfNoneMatch` 并返回 `bool`。 |
| `matchIfMatch` | `(header: string, current: EntityTag): bool` | 调用 `matchIfMatch` 并返回 `bool`。 |
| `compareStrong` | `(a: string, b: string): bool` | 调用 `compareStrong` 并返回 `bool`。 |
| `compareWeak` | `(a: string, b: string): bool` | 调用 `compareWeak` 并返回 `bool`。 |

## 最小调用示例

```cp
import { parseEtag } from "etag";

pub fn main() {
    let result = parseEtag("example", /* out: &mut EntityTag */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
