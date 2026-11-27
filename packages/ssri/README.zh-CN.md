[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ssri

Zero-allocation W3C Subresource Integrity (SRI) parser, validator, constant-time matcher, and formatter for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ssri` ([https://github.com/npm/ssri](https://github.com/npm/ssri))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ssri` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ssri
```

```cp
import {
    SriAlgo,
    SriHash,
    SriIntegrity,
    emptyHash,
    isValidAlgoName,
    parseAlgo,
    algoName,
    algoStrength,
} from "ssri";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `SriAlgo` | `Sha256, Sha384, Sha512, Blake3, Unknown` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SriHash` | `class` | 字段: `valid: bool, algo: SriAlgo, algo_name: string, digest: string, options: string` |
| `SriIntegrity` | `class` | 字段: `hashes: [MAX_HASHES]SriHash, count: usize` · 方法: `get, isEmpty` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `emptyHash` | `(): SriHash` | 调用 `emptyHash` 并返回 `SriHash`。 |
| `isValidAlgoName` | `(name: string): bool` | 执行 `isValidAlgoName` 条件判定，返回 `bool`。 |
| `parseAlgo` | `(name: string): SriAlgo` | 通过 `parseAlgo` 解析输入数据，返回 `SriAlgo`。 |
| `algoName` | `(algo: SriAlgo): string` | 调用 `algoName` 并返回 `string`。 |
| `algoStrength` | `(algo: SriAlgo): u8` | 调用 `algoStrength` 并返回 `u8`。 |
| `isBase64DataChar` | `(c: u8): bool` | 执行 `isBase64DataChar` 条件判定，返回 `bool`。 |
| `isValidBase64` | `(s: string): bool` | 执行 `isValidBase64` 条件判定，返回 `bool`。 |
| `isValidOptions` | `(opts: string): bool` | 执行 `isValidOptions` 条件判定，返回 `bool`。 |
| `parseSingleToken` | `(tok: string): SriHash` | 通过 `parseSingleToken` 解析输入数据，返回 `SriHash`。 |
| `parseIntegrity` | `(raw: string): SriIntegrity` | 通过 `parseIntegrity` 解析输入数据，返回 `SriIntegrity`。 |
| `strongestHash` | `(sri: SriIntegrity): SriHash?` | 调用 `strongestHash` 并返回 `SriHash?`。 |
| `pickAlgorithm` | `(sri: SriIntegrity, algo: SriAlgo): SriHash?` | 调用 `pickAlgorithm` 并返回 `SriHash?`。 |
| `matchesIntegrity` | `(expected: SriIntegrity, actual: SriIntegrity): bool` | 调用 `matchesIntegrity` 并返回 `bool`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `formatHash` | `(algo: SriAlgo, base64_digest: string, out: []u8): string` | 通过 `formatHash` 编码或格式化输出，返回 `string`。 |
| `formatIntegrity` | `(sri: SriIntegrity, out: []u8): string` | 通过 `formatIntegrity` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { isValidAlgoName } from "ssri";

pub fn main() {
    let result = isValidAlgoName("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
