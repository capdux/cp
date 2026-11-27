[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# credit-card-type

Zero-allocation payment card BIN network detector, Luhn validator, formatter, and PCI-DSS masker for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:credit-card-type` ([https://github.com/braintree/credit-card-type](https://github.com/braintree/credit-card-type))（原库名 `credit-card-type` → CP 包名 `credit-card-type`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `credit-card-type` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add credit-card-type
```

```cp
import {
    CardNetwork,
    CardInfo,
    ExtractResult,
    extractDigits,
    prefixNum,
    detectNetwork,
    checkValidLength,
    networkCvvLen,
} from "credit-card-type";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `CardNetwork` | `Visa, Mastercard, Amex, UnionPay, Jcb, Discover, DinersClub, Maestro ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CardInfo` | `class` | 字段: `network: CardNetwork, digit_count: usize, valid_length: bool, luhn_ok: bool, is_valid: bool, cvv_len: u8` |
| `ExtractResult` | `class` | 字段: `count: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `extractDigits` | `(raw: string, out: &mut [MAX_CARD_DIGITS]u8): ExtractResult` | 调用 `extractDigits` 并返回 `ExtractResult`。 |
| `prefixNum` | `(digits: &mut [MAX_CARD_DIGITS]u8, count: usize, len: usize): u32` | 调用 `prefixNum` 并返回 `u32`。 |
| `detectNetwork` | `(digits: &mut [MAX_CARD_DIGITS]u8, count: usize): CardNetwork` | 调用 `detectNetwork` 并返回 `CardNetwork`。 |
| `checkValidLength` | `(net: CardNetwork, len: usize): bool` | 校验输入合法性，返回 `bool`。 |
| `networkCvvLen` | `(net: CardNetwork): u8` | 调用 `networkCvvLen` 并返回 `u8`。 |
| `checkLuhnDigits` | `(digits: &mut [MAX_CARD_DIGITS]u8, count: usize): bool` | 校验输入合法性，返回 `bool`。 |
| `luhnValid` | `(raw: string): bool` | 调用 `luhnValid` 并返回 `bool`。 |
| `inspectCard` | `(raw: string): CardInfo` | 调用 `inspectCard` 并返回 `CardInfo`。 |
| `networkName` | `(n: CardNetwork): string` | 调用 `networkName` 并返回 `string`。 |
| `formatCardNumber` | `(raw: string, out: []u8): string` | 通过 `formatCardNumber` 编码或格式化输出，返回 `string`。 |
| `maskCardPci` | `(raw: string, out: []u8): string` | 调用 `maskCardPci` 并返回 `string`。 |

## 最小调用示例

```cp
import { formatCardNumber } from "credit-card-type";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatCardNumber("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
