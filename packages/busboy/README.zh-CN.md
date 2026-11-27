[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# busboy

Zero-heap RFC 7578 multipart/form-data boundary extractor, part iterator, and Content-Disposition field/filename parser.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:busboy` ([https://github.com/mscdex/busboy](https://github.com/mscdex/busboy)) / `crates.io:multer` ([https://github.com/rousan/multer-rs](https://github.com/rousan/multer-rs))（原库名 `busboy` → CP 包名 `busboy`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `busboy` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add busboy
```

```cp
import {
    MultipartPart,
    isWs,
    isBchar,
    isValidBoundary,
    extractBoundary,
    matchBoundaryAt,
    extractDispositionParam,
    nextPart,
} from "busboy";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MultipartPart` | `class` | 字段: `headers: string, body: string, name: string, filename: string, content_type: string, next_pos: usize, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isWs` | `(c: u8): bool` | 执行 `isWs` 条件判定，返回 `bool`。 |
| `isBchar` | `(c: u8): bool` | 执行 `isBchar` 条件判定，返回 `bool`。 |
| `isValidBoundary` | `(boundary: string): bool` | 执行 `isValidBoundary` 条件判定，返回 `bool`。 |
| `extractBoundary` | `(content_type: string): string` | 调用 `extractBoundary` 并返回 `string`。 |
| `matchBoundaryAt` | `(payload: string, pos: usize, boundary: string): bool` | 调用 `matchBoundaryAt` 并返回 `bool`。 |
| `extractDispositionParam` | `(disp: string, key: string): string` | 调用 `extractDispositionParam` 并返回 `string`。 |
| `nextPart` | `(payload: string, boundary: string, start: usize): MultipartPart` | 调用 `nextPart` 并返回 `MultipartPart`。 |
| `countParts` | `(payload: string, boundary: string): usize` | 调用 `countParts` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isValidBoundary } from "busboy";

pub fn main() {
    let result = isValidBoundary("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
