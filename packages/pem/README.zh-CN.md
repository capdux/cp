[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pem

Zero-heap RFC 7468 PEM armor multi-block parser, validator, and 64-column wrapped encoder for certificates and keys

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustls-pemfile` ([https://github.com/rustls/pemfile](https://github.com/rustls/pemfile)) / `crates.io:pem-rfc7468` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))（原库名 `rustls-pemfile` → CP 包名 `pem`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `pem` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pem
```

```cp
import {
    PemBlock,
    PemBundle,
    isValidLabel,
    parsePemFrom,
    parsePem,
    parsePemBundle,
    extractBase64,
    encodePem,
} from "pem";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PemBlock` | `class` | 字段: `valid: bool, label: string, raw_body: string, start_offset: usize, end_offset: usize` |
| `PemBundle` | `class` | 字段: `blocks: [16]PemBlock, count: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isValidLabel` | `(label: string): bool` | 执行 `isValidLabel` 条件判定，返回 `bool`。 |
| `parsePemFrom` | `(input: string, from_offset: usize): PemBlock` | 通过 `parsePemFrom` 解析输入数据，返回 `PemBlock`。 |
| `parsePem` | `(input: string): PemBlock` | 通过 `parsePem` 解析输入数据，返回 `PemBlock`。 |
| `parsePemBundle` | `(input: string): PemBundle` | 通过 `parsePemBundle` 解析输入数据，返回 `PemBundle`。 |
| `extractBase64` | `(blk: PemBlock, out: []u8): string` | 调用 `extractBase64` 并返回 `string`。 |
| `encodePem` | `(label: string, base64_payload: string, out: []u8): string` | 通过 `encodePem` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { isValidLabel } from "pem";

pub fn main() {
    let result = isValidLabel("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
