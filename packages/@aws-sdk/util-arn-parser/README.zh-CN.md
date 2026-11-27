[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @aws-sdk/util-arn-parser

Zero-heap AWS ARN (Amazon Resource Name) parser, formatter, partition validator, resource type extractor, and IAM wildcard matcher.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@aws-sdk/util-arn-parser` ([https://github.com/aws/aws-sdk-js-v3](https://github.com/aws/aws-sdk-js-v3)) / `crates.io:aws-arn` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `@aws-sdk/util-arn-parser` → CP 包名 `@aws-sdk/util-arn-parser`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `@aws-sdk/util-arn-parser` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @aws-sdk/util-arn-parser
```

```cp
import {
    Arn,
    bytesEq,
    isValidPartition,
    parseArn,
    appendSlice,
    formatArn,
    matchResourcePattern,
    resourceType,
} from "@aws-sdk/util-arn-parser";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Arn` | `class` | 字段: `partition: string, service: string, region: string, account_id: string, resource: string, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bytesEq` | `(a: string, b: string): bool` | 调用 `bytesEq` 并返回 `bool`。 |
| `isValidPartition` | `(partition: string): bool` | 执行 `isValidPartition` 条件判定，返回 `bool`。 |
| `parseArn` | `(input: string): Arn` | 通过 `parseArn` 解析输入数据，返回 `Arn`。 |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | 调用 `appendSlice` 并返回 `i64`。 |
| `formatArn` | `(arn: Arn, out: []u8): usize` | 通过 `formatArn` 编码或格式化输出，返回 `usize`。 |
| `matchResourcePattern` | `(resource: string, pattern: string): bool` | 调用 `matchResourcePattern` 并返回 `bool`。 |
| `resourceType` | `(resource: string): string` | 调用 `resourceType` 并返回 `string`。 |

## 最小调用示例

```cp
import { isValidPartition } from "@aws-sdk/util-arn-parser";

pub fn main() {
    let result = isValidPartition("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
