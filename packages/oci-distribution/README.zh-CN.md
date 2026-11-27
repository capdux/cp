[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# oci-distribution

Zero-allocation OCI / Docker image reference parser, validator, and normalizer for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:oci-distribution` ([https://github.com/krustlet/oci-distribution](https://github.com/krustlet/oci-distribution)) / `npm:docker-image-parse` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `oci-distribution` → CP 包名 `oci-distribution`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `oci-distribution` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add oci-distribution
```

```cp
import {
    ImageRef,
    drIsLower,
    drIsUpper,
    drIsDigit,
    drIsLowerHex,
    drEqBytes,
    drContainsByte,
    isValidTag,
} from "oci-distribution";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ImageRef` | `class` | 字段: `registry: string, repository: string, tag: string, digest: string, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `drIsLower` | `(c: u8): bool` | 调用 `drIsLower` 并返回 `bool`。 |
| `drIsUpper` | `(c: u8): bool` | 调用 `drIsUpper` 并返回 `bool`。 |
| `drIsDigit` | `(c: u8): bool` | 调用 `drIsDigit` 并返回 `bool`。 |
| `drIsLowerHex` | `(c: u8): bool` | 调用 `drIsLowerHex` 并返回 `bool`。 |
| `drEqBytes` | `(a: &[]u8, b: &[]u8): bool` | 调用 `drEqBytes` 并返回 `bool`。 |
| `drContainsByte` | `(s: &[]u8, target: u8): bool` | 调用 `drContainsByte` 并返回 `bool`。 |
| `isValidTag` | `(tag: string): bool` | 执行 `isValidTag` 条件判定，返回 `bool`。 |
| `isValidDigest` | `(digest: string): bool` | 执行 `isValidDigest` 条件判定，返回 `bool`。 |
| `drIsValidRegistry` | `(reg: &[]u8): bool` | 调用 `drIsValidRegistry` 并返回 `bool`。 |
| `drIsValidRepository` | `(repo: &[]u8): bool` | 调用 `drIsValidRepository` 并返回 `bool`。 |
| `parseImageRef` | `(input: string): ImageRef` | 通过 `parseImageRef` 解析输入数据，返回 `ImageRef`。 |
| `drWrite` | `(out: []u8, pos: &mut usize, src: &[]u8): bool` | 调用 `drWrite` 并返回 `bool`。 |
| `formatNormalized` | `(ref_in: ImageRef, out: []u8): usize` | 通过 `formatNormalized` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isValidTag } from "oci-distribution";

pub fn main() {
    let result = isValidTag("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
