[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fast-glob

Zero-heap pure CP fast-glob pattern set, dynamic pattern detector, and path escaper (clean-room MIT implementation of npm fast-glob v3)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fast-glob` ([https://github.com/mrmlnc/fast-glob](https://github.com/mrmlnc/fast-glob))

| 元信息 | 值 |
| --- | --- |
| 包名 | `fast-glob` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fast-glob
```

```cp
import {
    FastGlobOptions,
    GlobPatternSet,
    fglToLower,
    fglIsGlobMeta,
    fglNormalizeRelPath,
    fglPathDepth,
    fglHasDotSegment,
    fglLowerInto,
} from "fast-glob";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FastGlobOptions` | `class` | 字段: `dot: bool, deep: usize, only_files: bool, only_directories: bool, case_sensitive: bool` · 方法: `defaults` |
| `GlobPatternSet` | `class` | 字段: `opts: FastGlobOptions, includes: [16]string, include_count: usize, ignores: [16]string, ignore_count: usize` · 方法: `new, addPattern, addIgnore, matches` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `fglToLower` | `(b: u8): u8` | 调用 `fglToLower` 并返回 `u8`。 |
| `fglIsGlobMeta` | `(b: u8): bool` | 调用 `fglIsGlobMeta` 并返回 `bool`。 |
| `fglNormalizeRelPath` | `(raw: string): string` | 调用 `fglNormalizeRelPath` 并返回 `string`。 |
| `fglPathDepth` | `(path: string): usize` | 调用 `fglPathDepth` 并返回 `usize`。 |
| `fglHasDotSegment` | `(path: string): bool` | 调用 `fglHasDotSegment` 并返回 `bool`。 |
| `fglLowerInto` | `(src: string, dst: []u8): usize` | 调用 `fglLowerInto` 并返回 `usize`。 |
| `fglMatchOne` | `(pattern: string, path: string, case_sensitive: bool): bool` | 调用 `fglMatchOne` 并返回 `bool`。 |
| `isDynamicPattern` | `(pattern: string): bool` | 执行 `isDynamicPattern` 条件判定，返回 `bool`。 |
| `escapePath` | `(path: string, out: []u8): usize` | 调用 `escapePath` 并返回 `usize`。 |
| `convertPathToPattern` | `(path: string, out: []u8): usize` | 调用 `convertPathToPattern` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isDynamicPattern } from "fast-glob";

pub fn main() {
    let result = isDynamicPattern("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
