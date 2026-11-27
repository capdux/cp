[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bs58

Zero-allocation Base58 encoder/decoder supporting Bitcoin, Ripple, and Flickr alphabets with leading-zero preservation (Rust bs58 / npm bs58 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bs58` ([https://github.com/Nullus157/bs58-rs](https://github.com/Nullus157/bs58-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `bs58` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bs58
```

```cp
import {
    Base58Alphabet,
    alphabetChars,
    charIndex,
    isValidBase58,
    encodeBase58With,
    decodeBase58With,
    encodeBase58,
    decodeBase58,
} from "bs58";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `Base58Alphabet` | `Bitcoin, Ripple, Flickr` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `alphabetChars` | `(alpha: Base58Alphabet): string` | 调用 `alphabetChars` 并返回 `string`。 |
| `charIndex` | `(alphabet: string, c: u8): i8` | 调用 `charIndex` 并返回 `i8`。 |
| `isValidBase58` | `(encoded: string, alpha: Base58Alphabet): bool` | 执行 `isValidBase58` 条件判定，返回 `bool`。 |
| `encodeBase58With` | `(input: string, alpha: Base58Alphabet, out: []u8): string?` | 通过 `encodeBase58With` 编码或格式化输出，返回 `string?`。 |
| `decodeBase58With` | `(encoded: string, alpha: Base58Alphabet, out: []u8): string?` | 通过 `decodeBase58With` 解析输入数据，返回 `string?`。 |
| `encodeBase58` | `(input: string, out: []u8): string?` | 通过 `encodeBase58` 编码或格式化输出，返回 `string?`。 |
| `decodeBase58` | `(encoded: string, out: []u8): string?` | 通过 `decodeBase58` 解析输入数据，返回 `string?`。 |

## 最小调用示例

```cp
import { isValidBase58 } from "bs58";

pub fn main() {
    let result = isValidBase58("example", /* alpha: Base58Alphabet */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
