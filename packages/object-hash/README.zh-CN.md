[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# object-hash

Zero-allocation deterministic order-independent structured object hasher using FNV-1a 64-bit (npm object-hash parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`object-hash`](https://github.com/puleos/object-hash)（`MIT`）（包名 `object-hash`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `object-hash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add object-hash
```

```cp
import {
    HashField,
    ohshFnvOffset,
    ohshFnvPrime,
    ohshFeedByte,
    ohshFeedBytes,
    ohshCmpStr,
    ohshFieldLess,
    ohshHexDigit,
} from "object-hash";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HashField` | `class` | 字段: `key: string, value: string, kind: u8` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ohshFnvOffset` | `(): u64` | 调用 `ohshFnvOffset` 并返回 `u64`。 |
| `ohshFnvPrime` | `(): u64` | 调用 `ohshFnvPrime` 并返回 `u64`。 |
| `ohshFeedByte` | `(h: u64, b: u8): u64` | 调用 `ohshFeedByte` 并返回 `u64`。 |
| `ohshFeedBytes` | `(h: u64, data: &[]u8): u64` | 调用 `ohshFeedBytes` 并返回 `u64`。 |
| `ohshCmpStr` | `(a: string, b: string): i32` | 调用 `ohshCmpStr` 并返回 `i32`。 |
| `ohshFieldLess` | `(a: HashField, b: HashField): bool` | 调用 `ohshFieldLess` 并返回 `bool`。 |
| `ohshHexDigit` | `(nibble: u8): u8` | 调用 `ohshHexDigit` 并返回 `u8`。 |
| `hashBytesFnv64` | `(data: &[]u8): u64` | 执行 `hashBytesFnv64` 条件判定，返回 `bool`。 |
| `hashStringFnv64` | `(s: string): u64` | 执行 `hashStringFnv64` 条件判定，返回 `bool`。 |
| `hashFieldsFnv64` | `(fields: &[]HashField): u64` | 执行 `hashFieldsFnv64` 条件判定，返回 `bool`。 |
| `hashFieldsHex` | `(fields: &[]HashField, out: []u8): usize` | 执行 `hashFieldsHex` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { hashStringFnv64 } from "object-hash";

pub fn main() {
    let result = hashStringFnv64("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
