[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# querystring

Zero-allocation URL query string parser, stringifier & parameter extractor (Node.js querystring & npm query-string v9 parity) composing percent-encoding for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:querystring` ([https://github.com/sindresorhus/query-string](https://github.com/sindresorhus/query-string))

| 元信息 | 值 |
| --- | --- |
| 包名 | `querystring` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add querystring
```

```cp
import {
    QueryEntry,
    qstFindByte,
    qstNeedsDecode,
    qstHasSlashBefore,
    escapeQuery,
    unescapeQuery,
    extractUrlQuery,
    parseQueryString,
} from "querystring";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `QueryEntry` | `class` | 字段: `key: string, value: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `qstFindByte` | `(s: string, start: usize, end: usize, target: u8): usize` | 调用 `qstFindByte` 并返回 `usize`。 |
| `qstNeedsDecode` | `(s: string): bool` | 调用 `qstNeedsDecode` 并返回 `bool`。 |
| `qstHasSlashBefore` | `(s: string, end: usize): bool` | 调用 `qstHasSlashBefore` 并返回 `bool`。 |
| `escapeQuery` | `(raw: string, out: []u8): usize` | 调用 `escapeQuery` 并返回 `usize`。 |
| `unescapeQuery` | `(raw: string, out: []u8): usize` | 调用 `unescapeQuery` 并返回 `usize`。 |
| `extractUrlQuery` | `(url: string): string` | 调用 `extractUrlQuery` 并返回 `string`。 |
| `parseQueryString` | `(input: string, sep: u8, eq: u8, out_entries: []QueryEntry): usize` | 通过 `parseQueryString` 解析输入数据，返回 `usize`。 |
| `stringifyQueryString` | `(entries: &[]QueryEntry, sep: u8, eq: u8, out: []u8): usize` | 通过 `stringifyQueryString` 编码或格式化输出，返回 `usize`。 |
| `pickQueryParam` | `(url_or_query: string, target_key: string, out: []u8): usize?` | 调用 `pickQueryParam` 并返回 `usize?`。 |

## 最小调用示例

```cp
import { parseQueryString } from "querystring";

pub fn main() {
    let result = parseQueryString("example", 0, 0, /* out_entries: []QueryEntry */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
