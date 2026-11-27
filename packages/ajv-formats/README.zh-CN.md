[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ajv-formats

Zero-allocation JSON Schema Draft 2019-09/2020-12 string format validators: date, time, date-time, email, hostname, ipv4, ipv6, uuid, uri, json-pointer (clean-room MIT implementation of npm ajv-formats, #205 / 212M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ajv-formats` ([https://github.com/ajv-validator/ajv-formats](https://github.com/ajv-validator/ajv-formats)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `ajv-formats` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ajv-formats
```

```cp
import {
    ajvfIsDigit,
    ajvfIsAlpha,
    ajvfIsAlphaNum,
    ajvfIsHex,
    ajvfParse2Digits,
    ajvfParse4Digits,
    ajvfIsLeapYear,
    ajvfDaysInMonth,
} from "ajv-formats";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ajvfIsDigit` | `(b: u8): bool` | 调用 `ajvfIsDigit` 并返回 `bool`。 |
| `ajvfIsAlpha` | `(b: u8): bool` | 调用 `ajvfIsAlpha` 并返回 `bool`。 |
| `ajvfIsAlphaNum` | `(b: u8): bool` | 调用 `ajvfIsAlphaNum` 并返回 `bool`。 |
| `ajvfIsHex` | `(b: u8): bool` | 调用 `ajvfIsHex` 并返回 `bool`。 |
| `ajvfParse2Digits` | `(s: string, offset: usize): i32` | 调用 `ajvfParse2Digits` 并返回 `i32`。 |
| `ajvfParse4Digits` | `(s: string, offset: usize): i32` | 调用 `ajvfParse4Digits` 并返回 `i32`。 |
| `ajvfIsLeapYear` | `(year: u32): bool` | 调用 `ajvfIsLeapYear` 并返回 `bool`。 |
| `ajvfDaysInMonth` | `(year: u32, month: u32): u32` | 调用 `ajvfDaysInMonth` 并返回 `u32`。 |
| `isDate` | `(s: string): bool` | 执行 `isDate` 条件判定，返回 `bool`。 |
| `isTime` | `(s: string): bool` | 执行 `isTime` 条件判定，返回 `bool`。 |
| `isDateTime` | `(s: string): bool` | 执行 `isDateTime` 条件判定，返回 `bool`。 |
| `isHostname` | `(s: string): bool` | 执行 `isHostname` 条件判定，返回 `bool`。 |
| `ajvfIsEmailLocalChar` | `(b: u8): bool` | 调用 `ajvfIsEmailLocalChar` 并返回 `bool`。 |
| `isEmail` | `(s: string): bool` | 执行 `isEmail` 条件判定，返回 `bool`。 |
| `isIpv4` | `(s: string): bool` | 执行 `isIpv4` 条件判定，返回 `bool`。 |
| `isIpv6` | `(s: string): bool` | 执行 `isIpv6` 条件判定，返回 `bool`。 |
| `isUuid` | `(s: string): bool` | 执行 `isUuid` 条件判定，返回 `bool`。 |
| `ajvfIsSchemeChar` | `(b: u8): bool` | 调用 `ajvfIsSchemeChar` 并返回 `bool`。 |
| `isUri` | `(s: string): bool` | 执行 `isUri` 条件判定，返回 `bool`。 |
| `isJsonPointer` | `(s: string): bool` | 执行 `isJsonPointer` 条件判定，返回 `bool`。 |
| `ajvfStrEq` | `(a: string, b: string): bool` | 调用 `ajvfStrEq` 并返回 `bool`。 |
| `validateFormat` | `(format_name: string, value: string): bool` | 校验输入合法性，返回 `bool`。 |

## 最小调用示例

```cp
import { isDate } from "ajv-formats";

pub fn main() {
    let result = isDate("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
