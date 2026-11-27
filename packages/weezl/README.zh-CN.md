[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# weezl

Zero-allocation GIF/TIFF/PDF LZW dictionary lossless compressor & decompressor with 9..12-bit variable code widths, Clear/EOI control codes, and KwKwK handling (Rust weezl parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:weezl` ([https://github.com/image-rs/lzw](https://github.com/image-rs/lzw))（原库名 `weezl` → CP 包名 `weezl`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `weezl` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add weezl
```

```cp
import { hashPair, encode, decode } from "weezl";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hashPair` | `(key: u32): usize` | 执行 `hashPair` 条件判定，返回 `bool`。 |
| `encode` | `(src: &[]u8, dst: []u8): i64` | 通过 `encode` 编码或格式化输出，返回 `i64`。 |
| `decode` | `(src: &[]u8, dst: []u8): i64` | 通过 `decode` 解析输入数据，返回 `i64`。 |

## 最小调用示例

```cp
import { hashPair } from "weezl";

pub fn main() {
    let result = hashPair(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
