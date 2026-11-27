[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ldapjs

Zero-heap RFC 4514 LDAP Distinguished Name parser, value escaper/unescaper, validator, and case-insensitive attribute lookup.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ldapjs` ([https://github.com/ldapjs/node-ldapjs](https://github.com/ldapjs/node-ldapjs)) / `crates.io:ldap3` ([https://github.com/inejge/ldap3](https://github.com/inejge/ldap3))（原库名 `ldapjs` → CP 包名 `ldapjs`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `ldapjs` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ldapjs
```

```cp
import {
    RdnAttr,
    isWs,
    isValidAttrType,
    isEscapedAt,
    hasValidEscapes,
    parseDn,
    escapeValue,
    unescapeValue,
} from "ldapjs";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RdnAttr` | `class` | 字段: `attr: string, raw_value: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isWs` | `(c: u8): bool` | 执行 `isWs` 条件判定，返回 `bool`。 |
| `isValidAttrType` | `(attr: string): bool` | 执行 `isValidAttrType` 条件判定，返回 `bool`。 |
| `isEscapedAt` | `(s: string, start: usize, idx: usize): bool` | 执行 `isEscapedAt` 条件判定，返回 `bool`。 |
| `hasValidEscapes` | `(raw: string): bool` | 执行 `hasValidEscapes` 条件判定，返回 `bool`。 |
| `parseDn` | `(dn: string, out: []RdnAttr): usize` | 通过 `parseDn` 解析输入数据，返回 `usize`。 |
| `escapeValue` | `(value: string, out: []u8): usize` | 调用 `escapeValue` 并返回 `usize`。 |
| `unescapeValue` | `(raw: string, out: []u8): usize` | 调用 `unescapeValue` 并返回 `usize`。 |
| `findAttr` | `(dn: string, target_attr: string, out: []u8, out_len: &mut usize): bool` | 查询或读取目标数据，返回 `bool`。 |
| `isValidDn` | `(dn: string): bool` | 执行 `isValidDn` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isValidAttrType } from "ldapjs";

pub fn main() {
    let result = isValidAttrType("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
