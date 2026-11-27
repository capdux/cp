[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# file-type

Zero-allocation binary file magic bytes signature sniffer (25+ image, archive, document, executable, audio, video, and database formats) for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:file-type` ([https://github.com/sindresorhus/file-type](https://github.com/sindresorhus/file-type)) / `crates.io:infer` ([https://github.com/bojand/infer](https://github.com/bojand/infer))（原库名 `file-type` → CP 包名 `file-type`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `file-type` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add file-type
```

```cp
import {
    FileType,
    mbMatch,
    mbEqAt,
    detect,
    isImage,
    isArchive,
    isExecutable,
} from "file-type";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FileType` | `class` | 字段: `ext: string, mime: string, kind: u8, matched: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mbMatch` | `(ext: string, mime: string, kind: u8): FileType` | 调用 `mbMatch` 并返回 `FileType`。 |
| `mbEqAt` | `(data: &[]u8, offset: usize, lit: &[]u8): bool` | 调用 `mbEqAt` 并返回 `bool`。 |
| `detect` | `(data: &[]u8): FileType` | 调用 `detect` 并返回 `FileType`。 |
| `isImage` | `(data: &[]u8): bool` | 执行 `isImage` 条件判定，返回 `bool`。 |
| `isArchive` | `(data: &[]u8): bool` | 执行 `isArchive` 条件判定，返回 `bool`。 |
| `isExecutable` | `(data: &[]u8): bool` | 执行 `isExecutable` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { mbMatch } from "file-type";

pub fn main() {
    let result = mbMatch("example", "example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
