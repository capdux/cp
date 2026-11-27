[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# querystringify

Zero-allocation WHATWG application/x-www-form-urlencoded parser, serializer, and percent-encoding codec for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:querystringify` ([https://github.com/unshiftio/querystringify](https://github.com/unshiftio/querystringify)) / `crates.io:urlencoding` ([https://github.com/kornelski/rust_urlencoding](https://github.com/kornelski/rust_urlencoding))（原库名 `querystringify` → CP 包名 `querystringify`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `querystringify` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add querystringify
```

```cp
import {
    FormPair,
    FormSerializer,
    isFormSafe,
    encodedSize,
    percentEncode,
    decodedSize,
    percentDecode,
    parseFormPairs,
} from "querystringify";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FormPair` | `class` | 字段: `key_start: usize, key_len: usize, val_start: usize, val_len: usize` |
| `FormSerializer` | `class` | 字段: `len: usize, count: usize` · 方法: `new, appendPair` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isFormSafe` | `(b: u8): bool` | 执行 `isFormSafe` 条件判定，返回 `bool`。 |
| `encodedSize` | `(input: string, space_as_plus: bool): usize` | 通过 `encodedSize` 编码或格式化输出，返回 `usize`。 |
| `percentEncode` | `(input: string, space_as_plus: bool, out: []u8): usize` | 调用 `percentEncode` 并返回 `usize`。 |
| `decodedSize` | `(input: string): usize` | 通过 `decodedSize` 解析输入数据，返回 `usize`。 |
| `percentDecode` | `(input: string, plus_as_space: bool, out: []u8): usize` | 调用 `percentDecode` 并返回 `usize`。 |
| `parseFormPairs` | `(query: string, out: []FormPair): usize` | 通过 `parseFormPairs` 解析输入数据，返回 `usize`。 |
| `decodedMatches` | `(raw: string, plus_as_space: bool, target: string): bool` | 通过 `decodedMatches` 解析输入数据，返回 `bool`。 |
| `getParamDecoded` | `(query: string, target_key: string, out: []u8): i64` | 查询或读取目标数据，返回 `i64`。 |

## 最小调用示例

```cp
import { encodedSize } from "querystringify";

pub fn main() {
    let result = encodedSize("example", false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
