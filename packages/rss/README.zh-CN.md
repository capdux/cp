[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rss

Zero-allocation RSS 2.0 XML feed generator and CDATA-aware tag extractor in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rss` ([https://github.com/rust-syndication/rss](https://github.com/rust-syndication/rss))

| 元信息 | 值 |
| --- | --- |
| 包名 | `rss` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rss
```

```cp
import {
    RssChannel,
    RssItem,
    RssWriter,
    pushSlice,
    pushXmlEscaped,
    pushTag,
    extractTagValue,
    countItems,
} from "rss";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RssChannel` | `class` | 字段: `title: &[]u8, link: &[]u8, description: &[]u8, language: &[]u8` |
| `RssItem` | `class` | 字段: `title: &[]u8, link: &[]u8, description: &[]u8, guid: &[]u8, pub_date: &[]u8` |
| `RssWriter` | `class` | 字段: `buf: []u8, pos: usize, item_count: usize, failed: bool, finished: bool` · 方法: `init, addItem, finish, itemCount, written` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pushSlice` | `(dst: []u8, pos: usize, src: &[]u8): i64` | 写入或更新状态数据。 |
| `pushXmlEscaped` | `(dst: []u8, pos: usize, src: &[]u8): i64` | 写入或更新状态数据。 |
| `pushTag` | `(dst: []u8, pos: usize, tag: &[]u8, val: &[]u8): i64` | 写入或更新状态数据。 |
| `extractTagValue` | `(xml: &[]u8, tag: &[]u8, start_pos: usize): &[]u8` | 调用 `extractTagValue` 并返回 `&[]u8`。 |
| `countItems` | `(xml: &[]u8): usize` | 调用 `countItems` 并返回 `usize`。 |

## 最小调用示例

```cp
import { pushSlice } from "rss";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = pushSlice(out_buf[0..256], 0, buf_1[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
