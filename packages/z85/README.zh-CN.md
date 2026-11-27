[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# z85

Zero-allocation Base85 multi-standard encoder/decoder supporting ZeroMQ Z85 (RFC 32) and Adobe Ascii85 with 'z' zero-compression and <~...~> framing (Rust z85 / ascii85 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:z85` ([https://github.com/murar8/z85](https://github.com/murar8/z85))（原库名 `z85` → CP 包名 `z85`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `z85` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add z85
```

```cp
import { encodeZ85, decodeZ85, encodeAscii85, decodeAscii85 } from "z85";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `encodeZ85` | `(input: string, out: []u8): string?` | 通过 `encodeZ85` 编码或格式化输出，返回 `string?`。 |
| `decodeZ85` | `(encoded: string, out: []u8): string?` | 通过 `decodeZ85` 解析输入数据，返回 `string?`。 |
| `encodeAscii85` | `(input: string, out: []u8): string?` | 通过 `encodeAscii85` 编码或格式化输出，返回 `string?`。 |
| `decodeAscii85` | `(encoded: string, out: []u8): string?` | 通过 `decodeAscii85` 解析输入数据，返回 `string?`。 |

## 最小调用示例

```cp
import { encodeZ85 } from "z85";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeZ85("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
