[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# luhn

Zero-heap ISO/IEC 7812 Luhn Mod-10, Damm, and Verhoeff check digit algorithms in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:luhn` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| 元信息 | 值 |
| --- | --- |
| 包名 | `luhn` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add luhn
```

```cp
import {
    luhnValidate,
    luhnCheckDigit,
    dammCheckDigit,
    dammValidate,
    verhoeffCheckDigit,
    verhoeffValidate,
} from "luhn";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `luhnValidate` | `(input: &[]u8): bool` | 调用 `luhnValidate` 并返回 `bool`。 |
| `luhnCheckDigit` | `(payload: &[]u8): i32` | 调用 `luhnCheckDigit` 并返回 `i32`。 |
| `dammCheckDigit` | `(payload: &[]u8): i32` | 调用 `dammCheckDigit` 并返回 `i32`。 |
| `dammValidate` | `(input: &[]u8): bool` | 调用 `dammValidate` 并返回 `bool`。 |
| `verhoeffCheckDigit` | `(payload: &[]u8): i32` | 调用 `verhoeffCheckDigit` 并返回 `i32`。 |
| `verhoeffValidate` | `(input: &[]u8): bool` | 调用 `verhoeffValidate` 并返回 `bool`。 |

## 最小调用示例

```cp
import { luhnValidate } from "luhn";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = luhnValidate(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
