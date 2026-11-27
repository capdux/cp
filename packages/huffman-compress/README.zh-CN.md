[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# huffman-compress

Zero-allocation RFC 1951 Canonical Huffman prefix encoder & decoder with 256-symbol alphabet, 16-bit length limiting, and MSB-first bitstream framing (Rust huffman-compress parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:huffman-compress` ([https://github.com/sozu-proxy/huffman-compress](https://github.com/sozu-proxy/huffman-compress))（原库名 `huffman-compress` → CP 包名 `huffman-compress`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `huffman-compress` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add huffman-compress
```

```cp
import { HuffmanCodec } from "huffman-compress";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HuffmanCodec` | `class` | 字段: `code_len: [MAX_SYMBOLS]u8, codes: [MAX_SYMBOLS]u16, symbol_count: usize` · 方法: `empty, fromCodeLengths, fromFrequencies, codeLength, codeBits, encode, +1 项` |

## 最小调用示例

```cp
import { HuffmanCodec } from "huffman-compress";

pub fn main() {
    // Refer to src/index.d.cp for HuffmanCodec fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
