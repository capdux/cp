[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# idna

Zero-allocation UTS #46 Internationalized Domain Names (IDNA domainToAscii / domainToUnicode) reusing icu-normalizer for NFC and punycode for RFC 3492 Punycode (idna crate parity) in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:idna` ([https://github.com/servo/rust-url](https://github.com/servo/rust-url))（原库名 `idna` → CP 包名 `idna`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `idna` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add idna
```

```cp
import {
    idnaIsLdhLower,
    idnaIsLdhAnyCase,
    isPunycodeLabel,
    idnaMapDomain,
    idnaValidateAsciiLabelLower,
    idnaAppendBytes,
    domainToAscii,
    domainToUnicode,
} from "idna";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `idnaIsLdhLower` | `(ch: u8): bool` | 调用 `idnaIsLdhLower` 并返回 `bool`。 |
| `idnaIsLdhAnyCase` | `(ch: u8): bool` | 调用 `idnaIsLdhAnyCase` 并返回 `bool`。 |
| `isPunycodeLabel` | `(label: string): bool` | 执行 `isPunycodeLabel` 条件判定，返回 `bool`。 |
| `idnaMapDomain` | `(domain: string, out: []u8): i64` | 调用 `idnaMapDomain` 并返回 `i64`。 |
| `idnaValidateAsciiLabelLower` | `(label: &[]u8): i64` | 调用 `idnaValidateAsciiLabelLower` 并返回 `i64`。 |
| `idnaAppendBytes` | `(out: []u8, pos: &mut usize, src: &[]u8): bool` | 调用 `idnaAppendBytes` 并返回 `bool`。 |
| `domainToAscii` | `(domain: string, out: []u8): i64` | 调用 `domainToAscii` 并返回 `i64`。 |
| `domainToUnicode` | `(domain: string, out: []u8): i64` | 调用 `domainToUnicode` 并返回 `i64`。 |
| `isValidIdnaAscii` | `(domain: string): bool` | 执行 `isValidIdnaAscii` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isPunycodeLabel } from "idna";

pub fn main() {
    let result = isPunycodeLabel("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
