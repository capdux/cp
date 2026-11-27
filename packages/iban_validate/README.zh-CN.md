[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# iban_validate

Zero-heap ISO 13616 IBAN validator, ISO 7064 Mod-97-10 checksum engine, check-digit generator, and electronic/friendly formatter.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:iban_validate` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `iban_validate` → CP 包名 `iban_validate`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `iban_validate` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add iban_validate
```

```cp
import {
    countryLength,
    electronicFormat,
    friendlyFormat,
    mod97,
    isValid,
    computeCheckDigits,
} from "iban_validate";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `countryLength` | `(c0: u8, c1: u8): usize` | 调用 `countryLength` 并返回 `usize`。 |
| `electronicFormat` | `(input: string, out: []u8): usize` | 调用 `electronicFormat` 并返回 `usize`。 |
| `friendlyFormat` | `(input: string, out: []u8): usize` | 调用 `friendlyFormat` 并返回 `usize`。 |
| `mod97` | `(electronic: string): u32` | 调用 `mod97` 并返回 `u32`。 |
| `isValid` | `(input: string): bool` | 执行 `isValid` 条件判定，返回 `bool`。 |
| `computeCheckDigits` | `(country0: u8, country1: u8, bban: string, out_d0: &mut u8, out_d1: &mut u8): bool` | 调用 `computeCheckDigits` 并返回 `bool`。 |

## 最小调用示例

```cp
import { isValid } from "iban_validate";

pub fn main() {
    let result = isValid("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
