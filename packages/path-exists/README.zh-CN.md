[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# path-exists

Zero-heap path/file/directory existence checker with symlink awareness and batch filtering (npm path-exists equivalent, reuses locate-path)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:path-exists` ([https://github.com/sindresorhus/path-exists](https://github.com/sindresorhus/path-exists))

| 元信息 | 值 |
| --- | --- |
| 包名 | `path-exists` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add path-exists
```

```cp
import {
    pathExistsInEntries,
    fileExistsInEntries,
    dirExistsInEntries,
    realPathExistsInEntries,
    countExistingPaths,
} from "path-exists";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pathExistsInEntries` | `(target: string, cwd: string, entries: []FsEntry): bool` | 调用 `pathExistsInEntries` 并返回 `bool`。 |
| `fileExistsInEntries` | `(target: string, cwd: string, entries: []FsEntry): bool` | 调用 `fileExistsInEntries` 并返回 `bool`。 |
| `dirExistsInEntries` | `(target: string, cwd: string, entries: []FsEntry): bool` | 调用 `dirExistsInEntries` 并返回 `bool`。 |
| `realPathExistsInEntries` | `(target: string, cwd: string, entries: []FsEntry): bool` | 调用 `realPathExistsInEntries` 并返回 `bool`。 |
| `countExistingPaths` | `(targets: []string, cwd: string, entries: []FsEntry, out_indices: []usize): usize` | 调用 `countExistingPaths` 并返回 `usize`。 |

## 最小调用示例

```cp
import { pathExistsInEntries } from "path-exists";

pub fn main() {
    let result = pathExistsInEntries("example", "example", /* entries: []FsEntry */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
