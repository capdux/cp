[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# vcard

Zero-allocation RFC 6350 vCard 4.0 (.vcf) contact generator and parser for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:vcard` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| 元信息 | 值 |
| --- | --- |
| 包名 | `vcard` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add vcard
```

```cp
import {
    VCardData,
    ParsedVCard,
    isSpaceOrTab,
    writeRaw,
    writeEscaped,
    writeProp,
    formatVCard,
    parseVCard,
} from "vcard";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `VCardData` | `class` | 字段: `full_name: &[]u8, org: &[]u8, title: &[]u8, email: &[]u8, tel: &[]u8, url: &[]u8` |
| `ParsedVCard` | `class` | 字段: `fn_start: usize, fn_len: usize, org_start: usize, org_len: usize, title_start: usize, title_len: usize, +9 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isSpaceOrTab` | `(c: u8): bool` | 执行 `isSpaceOrTab` 条件判定，返回 `bool`。 |
| `writeRaw` | `(dst: []u8, pos: &mut usize, src: &[]u8): bool` | 调用 `writeRaw` 并返回 `bool`。 |
| `writeEscaped` | `(dst: []u8, pos: &mut usize, src: &[]u8): bool` | 调用 `writeEscaped` 并返回 `bool`。 |
| `writeProp` | `(dst: []u8, pos: &mut usize, propPrefix: &[]u8, val: &[]u8): bool` | 调用 `writeProp` 并返回 `bool`。 |
| `formatVCard` | `(card: VCardData, dst: []u8): i64` | 通过 `formatVCard` 编码或格式化输出，返回 `i64`。 |
| `parseVCard` | `(vcf: &[]u8): ParsedVCard` | 通过 `parseVCard` 解析输入数据，返回 `ParsedVCard`。 |

## 最小调用示例

```cp
import { isSpaceOrTab } from "vcard";

pub fn main() {
    let result = isSpaceOrTab(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
