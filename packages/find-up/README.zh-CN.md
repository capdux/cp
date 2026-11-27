[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# find-up

Zero-heap upward ancestor directory file/directory search walker with stopAt boundary and multi-name candidate generation (npm find-up equivalent, reuses npm-run-path)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:find-up` ([https://github.com/sindresorhus/find-up](https://github.com/sindresorhus/find-up))

| 元信息 | 值 |
| --- | --- |
| 包名 | `find-up` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add find-up
```

```cp
import {
    FindUpOptions,
    FindUpWalker,
    fupStrEq,
    fupTrimTrailingSlash,
    fupPush,
    joinDirAndName,
    findUpInList,
    findUpMultipleInList,
} from "find-up";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FindUpOptions` | `class` | 字段: `cwd: string, stop_at: string, allow_symlinks: bool` · 方法: `defaults` |
| `FindUpWalker` | `class` | 字段: `current: string, stop_at: string, done: bool, steps: usize` · 方法: `new, nextDir, nextCandidate` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `fupStrEq` | `(a: string, b: string): bool` | 调用 `fupStrEq` 并返回 `bool`。 |
| `fupTrimTrailingSlash` | `(s: string): string` | 调用 `fupTrimTrailingSlash` 并返回 `string`。 |
| `fupPush` | `(out: []u8, pos: usize, s: string): usize` | 调用 `fupPush` 并返回 `usize`。 |
| `joinDirAndName` | `(dir: string, name: string, out: []u8): usize` | 调用 `joinDirAndName` 并返回 `usize`。 |
| `findUpInList` | `(opts: FindUpOptions, filename: string, existing_paths: []string, out: []u8): usize?` | 查询或读取目标数据，返回 `usize?`。 |
| `findUpMultipleInList` | `(opts: FindUpOptions, filenames: []string, existing_paths: []string, out: []u8): usize?` | 查询或读取目标数据，返回 `usize?`。 |

## 最小调用示例

```cp
import { fupStrEq } from "find-up";

pub fn main() {
    let result = fupStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
