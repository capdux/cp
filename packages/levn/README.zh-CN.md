[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# levn

Zero-heap Light Value Notation (levn) scalar, list, and object parser

| 元信息 | 值 |
| --- | --- |
| 包名 | `levn` |
| 版本 | `0.4.1` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add levn
```

```cp
import {
    LevnKind,
    LevnValue,
    LevnPair,
    lvnMakeInvalid,
    lvnTryParseNumber,
    parseLevnScalar,
    lvnFindTopLevelChar,
    lvnUnquoteKey,
} from "levn";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `LevnKind` | `Null, Bool, Int, Float, String, Invalid` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LevnValue` | `class` | 字段: `kind: LevnKind, bool_val: bool, int_val: i64, float_val: f64, str_val: string` |
| `LevnPair` | `class` | 字段: `key: string, value: LevnValue` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `lvnMakeInvalid` | `(): LevnValue` | 调用 `lvnMakeInvalid` 并返回 `LevnValue`。 |
| `lvnTryParseNumber` | `(s: string): LevnValue` | 调用 `lvnTryParseNumber` 并返回 `LevnValue`。 |
| `parseLevnScalar` | `(input: string): LevnValue` | 通过 `parseLevnScalar` 解析输入数据，返回 `LevnValue`。 |
| `lvnFindTopLevelChar` | `(s: string, start: usize, target: u8): usize` | 调用 `lvnFindTopLevelChar` 并返回 `usize`。 |
| `lvnUnquoteKey` | `(s: string): string` | 调用 `lvnUnquoteKey` 并返回 `string`。 |
| `parseLevnList` | `(input: string, out: []LevnValue): usize` | 通过 `parseLevnList` 解析输入数据，返回 `usize`。 |
| `parseLevnObject` | `(input: string, out: []LevnPair): usize` | 通过 `parseLevnObject` 解析输入数据，返回 `usize`。 |

## 最小调用示例

```cp
import { parseLevnScalar } from "levn";

pub fn main() {
    let result = parseLevnScalar("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
