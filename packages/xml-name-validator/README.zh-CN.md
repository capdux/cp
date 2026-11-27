[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# xml-name-validator

Zero-allocation W3C XML 1.0 (5th Ed.) & Namespaces in XML 1.0 Char, Name, NCName, and QName validator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:xml-name-validator` ([https://github.com/jsdom/xml-name-validator](https://github.com/jsdom/xml-name-validator), `Apache-2.0`)，同步覆盖 `npm:xmlchars` 字符类判定规则。

| 元信息 | 值 |
| --- | --- |
| 包名 | `xml-name-validator` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add xml-name-validator
```

```cp
import {
    XnvDecoded,
    xnvDecodeUtf8,
    xnvValidateName,
    isXmlChar,
    isNameStartChar,
    isNameChar,
    isValidXmlName,
    isValidNCName,
} from "xml-name-validator";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `XnvDecoded` | `class` | 字段: `cp: u32, len: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `xnvDecodeUtf8` | `(s: string, pos: usize): XnvDecoded` | 调用 `xnvDecodeUtf8` 并返回 `XnvDecoded`。 |
| `xnvValidateName` | `(s: string, allow_colon: bool): bool` | 调用 `xnvValidateName` 并返回 `bool`。 |
| `isXmlChar` | `(cp: u32): bool` | 执行 `isXmlChar` 条件判定，返回 `bool`。 |
| `isNameStartChar` | `(cp: u32): bool` | 执行 `isNameStartChar` 条件判定，返回 `bool`。 |
| `isNameChar` | `(cp: u32): bool` | 执行 `isNameChar` 条件判定，返回 `bool`。 |
| `isValidXmlName` | `(s: string): bool` | 执行 `isValidXmlName` 条件判定，返回 `bool`。 |
| `isValidNCName` | `(s: string): bool` | 执行 `isValidNCName` 条件判定，返回 `bool`。 |
| `isValidQName` | `(s: string): bool` | 执行 `isValidQName` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isValidXmlName } from "xml-name-validator";

pub fn main() {
    let result = isValidXmlName("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
