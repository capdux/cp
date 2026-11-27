[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# locate-path

Zero-allocation path locator over candidate lists with entry-type, symlink, and ordering controls for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:locate-path` ([https://github.com/sindresorhus/locate-path](https://github.com/sindresorhus/locate-path))

| 元信息 | 值 |
| --- | --- |
| 包名 | `locate-path` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add locate-path
```

```cp
import {
    LocateOptions,
    FsEntry,
    lcpStrEq,
    lcpTrimTrailingSlash,
    lcpCopyStr,
    isAbsolutePath,
    lcpResolveCandidate,
    lcpEntryMatches,
} from "locate-path";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LocateOptions` | `class` | 字段: `cwd: string, entry_type: u8, allow_symlinks: bool, preserve_order: bool` · 方法: `defaults` |
| `FsEntry` | `class` | 字段: `path: string, kind: u8, is_symlink: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `lcpStrEq` | `(a: string, b: string): bool` | 调用 `lcpStrEq` 并返回 `bool`。 |
| `lcpTrimTrailingSlash` | `(s: string): string` | 调用 `lcpTrimTrailingSlash` 并返回 `string`。 |
| `lcpCopyStr` | `(dst: []u8, src: string): usize` | 调用 `lcpCopyStr` 并返回 `usize`。 |
| `isAbsolutePath` | `(p: string): bool` | 执行 `isAbsolutePath` 条件判定，返回 `bool`。 |
| `lcpResolveCandidate` | `(cwd: string, cand: string, out: []u8): usize` | 调用 `lcpResolveCandidate` 并返回 `usize`。 |
| `lcpEntryMatches` | `(entry: FsEntry, target: string, opts: LocateOptions): bool` | 调用 `lcpEntryMatches` 并返回 `bool`。 |
| `locatePathInEntries` | `(candidates: []string, entries: []FsEntry, opts: LocateOptions, out: []u8): usize?` | 调用 `locatePathInEntries` 并返回 `usize?`。 |
| `locateAllPathsInEntries` | `(candidates: []string, entries: []FsEntry, opts: LocateOptions, matched_indices: []usize): usize` | 调用 `locateAllPathsInEntries` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isAbsolutePath } from "locate-path";

pub fn main() {
    let result = isAbsolutePath("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
