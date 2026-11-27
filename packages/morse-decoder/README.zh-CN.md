[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# morse-decoder

Zero-heap ITU-R M.1677 International Morse Code encoder and decoder supporting A-Z, 0-9, punctuation, and '/' word separators.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:morse-decoder` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:morse` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `morse-decoder` → CP 包名 `morse-decoder`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `morse-decoder` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add morse-decoder
```

```cp
import { encodeChar, decodeSymbol, encodeText, decodeText } from "morse-decoder";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `encodeChar` | `(c: u8): string` | 通过 `encodeChar` 编码或格式化输出，返回 `string`。 |
| `decodeSymbol` | `(sym: string): u8` | 通过 `decodeSymbol` 解析输入数据，返回 `u8`。 |
| `encodeText` | `(input: string, out: []u8): usize` | 通过 `encodeText` 编码或格式化输出，返回 `usize`。 |
| `decodeText` | `(morse: string, out: []u8): usize` | 通过 `decodeText` 解析输入数据，返回 `usize`。 |

## 最小调用示例

```cp
import { decodeSymbol } from "morse-decoder";

pub fn main() {
    let result = decodeSymbol("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
