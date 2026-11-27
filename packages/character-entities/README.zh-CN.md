[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# character-entities

Zero-allocation HTML5 named and numeric character reference decoder and encoder (covers decode-named-character-reference, parse-entities, stringify-entities)

| 元信息 | 值 |
| --- | --- |
| 包名 | `character-entities` |
| 版本 | `2.0.2` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add character-entities
```

```cp
import {
    mapInvalidCharRef,
    decodeNamedEntity,
    decodeNumericEntity,
    decodeEntities,
    encodeEntities,
} from "character-entities";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mapInvalidCharRef` | `(cp: u32): u32` | 调用 `mapInvalidCharRef` 并返回 `u32`。 |
| `decodeNamedEntity` | `(name: string): u32?` | 通过 `decodeNamedEntity` 解析输入数据，返回 `u32?`。 |
| `decodeNumericEntity` | `(body: string): u32?` | 通过 `decodeNumericEntity` 解析输入数据，返回 `u32?`。 |
| `decodeEntities` | `(input: string, out: []u8): usize` | 通过 `decodeEntities` 解析输入数据，返回 `usize`。 |
| `encodeEntities` | `(input: string, out: []u8): usize` | 通过 `encodeEntities` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { decodeNamedEntity } from "character-entities";

pub fn main() {
    let result = decodeNamedEntity("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
