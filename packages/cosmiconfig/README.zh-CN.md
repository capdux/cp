[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cosmiconfig

Zero-heap pure CP configuration file search planner, candidate path builder, and loader detector (clean-room MIT implementation of npm cosmiconfig v9)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cosmiconfig` ([https://github.com/cosmiconfig/cosmiconfig](https://github.com/cosmiconfig/cosmiconfig))

| 元信息 | 值 |
| --- | --- |
| 包名 | `cosmiconfig` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cosmiconfig
```

```cp
import {
    ExplorerOptions,
    SearchPlanner,
    CosmiConfigResult,
    cosmiEndsWithIgnoreCase,
    cosmiBasename,
    cosmiTrimTrailingSlash,
    detectLoader,
    formatSearchPlace,
} from "cosmiconfig";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ExplorerOptions` | `class` | 字段: `module_name: string, stop_dir: string, package_prop: string, ignore_empty: bool` · 方法: `defaults` |
| `SearchPlanner` | `class` | 字段: `start_dir: string, stop_dir: string, current_end: usize, done: bool` · 方法: `new, nextDir, dirCount` |
| `CosmiConfigResult` | `class` | 字段: `filepath: string, is_empty: bool, loader_kind: u8, found: bool` · 方法: `notFound, hit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cosmiEndsWithIgnoreCase` | `(s: string, suffix: string): bool` | 调用 `cosmiEndsWithIgnoreCase` 并返回 `bool`。 |
| `cosmiBasename` | `(filepath: string): string` | 调用 `cosmiBasename` 并返回 `string`。 |
| `cosmiTrimTrailingSlash` | `(dir: string): string` | 调用 `cosmiTrimTrailingSlash` 并返回 `string`。 |
| `detectLoader` | `(filepath: string): u8` | 调用 `detectLoader` 并返回 `u8`。 |
| `formatSearchPlace` | `(module_name: string, place_idx: usize, out: []u8): usize` | 通过 `formatSearchPlace` 编码或格式化输出，返回 `usize`。 |
| `buildCandidatePath` | `(dir: string, place: string, out: []u8): usize` | 调用 `buildCandidatePath` 并返回 `usize`。 |

## 最小调用示例

```cp
import { formatSearchPlace } from "cosmiconfig";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatSearchPlace("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
