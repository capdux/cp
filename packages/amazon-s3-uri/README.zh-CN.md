[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# amazon-s3-uri

Zero-heap AWS S3 / R2 / MinIO URI parser (s3://, virtual-hosted-style, path-style) and bucket name validator for CP (compatible with npm amazon-s3-uri & Rust s3-uri)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:amazon-s3-uri` ([https://github.com/jvdl/amazon-s3-uri](https://github.com/jvdl/amazon-s3-uri)) / `crates.io:aws-sdk-s3` ([https://github.com/awslabs/aws-sdk-rust](https://github.com/awslabs/aws-sdk-rust))（原库名 `amazon-s3-uri` → CP 包名 `amazon-s3-uri`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `amazon-s3-uri` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add amazon-s3-uri
```

```cp
import {
    S3Uri,
    indexOfByte,
    isValidBucketName,
    stripQueryAndFrag,
    parseS3Uri,
    formatS3Uri,
} from "amazon-s3-uri";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `S3Uri` | `class` | 字段: `style: u8, bucket: string, key: string, region: string, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `indexOfByte` | `(s: string, ch: u8): i64` | 调用 `indexOfByte` 并返回 `i64`。 |
| `isValidBucketName` | `(name: string): bool` | 执行 `isValidBucketName` 条件判定，返回 `bool`。 |
| `stripQueryAndFrag` | `(s: string): string` | 调用 `stripQueryAndFrag` 并返回 `string`。 |
| `parseS3Uri` | `(input: string): S3Uri` | 通过 `parseS3Uri` 解析输入数据，返回 `S3Uri`。 |
| `formatS3Uri` | `(bucket: string, key: string, out: []u8): usize` | 通过 `formatS3Uri` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isValidBucketName } from "amazon-s3-uri";

pub fn main() {
    let result = isValidBucketName("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
