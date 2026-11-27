[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# range-parser

Zero-heap RFC 7233 HTTP Range header parser with suffix ranges and interval combining (clean-room port of npm range-parser)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:range-parser` ([https://github.com/jshttp/range-parser](https://github.com/jshttp/range-parser))

| 元信息 | 值 |
| --- | --- |
| 包名 | `range-parser` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add range-parser
```

```cp
import { ByteRange, RangeResult, parseRange, rangeByteLength } from "range-parser";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ByteRange` | `class` | 字段: `start: u64, end: u64` |
| `RangeResult` | `class` | 字段: `status: i32, unit: string, ranges: [32]ByteRange, count: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `parseRange` | `(size: u64, header: string, combine: bool): RangeResult` | 通过 `parseRange` 解析输入数据，返回 `RangeResult`。 |
| `rangeByteLength` | `(r: ByteRange): u64` | 调用 `rangeByteLength` 并返回 `u64`。 |

## 最小调用示例

```cp
import { parseRange } from "range-parser";

pub fn main() {
    let result = parseRange(0, "example", false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
