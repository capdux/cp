[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# which

Zero-heap PATH executable candidate resolver for POSIX and Windows (clean-room CP implementation of npm which)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:which` ([https://github.com/npm/node-which](https://github.com/npm/node-which))（原库名 `which` → CP 包名 `which`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `which` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add which
```

```cp
import {
    WhichIterator,
    whEqIgnoreCase,
    isExplicitPath,
    countPathDirs,
    pathDirAt,
    hasPathExt,
    formatCandidatePath,
} from "which";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `WhichIterator` | `class` | 字段: `cmd: string, path_env: string, pathext: string, delim: u8, dir_idx: usize, dir_count: usize, +3 项` · 方法: `newPosix, newWindows, next` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `whEqIgnoreCase` | `(a: string, b: string): bool` | 调用 `whEqIgnoreCase` 并返回 `bool`。 |
| `isExplicitPath` | `(cmd: string): bool` | 执行 `isExplicitPath` 条件判定，返回 `bool`。 |
| `countPathDirs` | `(path_env: string, delim: u8): usize` | 调用 `countPathDirs` 并返回 `usize`。 |
| `pathDirAt` | `(path_env: string, delim: u8, idx: usize, out: []u8): usize` | 调用 `pathDirAt` 并返回 `usize`。 |
| `hasPathExt` | `(cmd: string, pathext: string): bool` | 执行 `hasPathExt` 条件判定，返回 `bool`。 |
| `formatCandidatePath` | `(dir: string, cmd: string, ext: string, out: []u8): usize` | 通过 `formatCandidatePath` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isExplicitPath } from "which";

pub fn main() {
    let result = isExplicitPath("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
