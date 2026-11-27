[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# chokidar

Zero-heap pure CP file watcher state machine, snapshot differ, debounce engine, and glob ignore filter (clean-room MIT implementation of npm chokidar v4 core)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:chokidar` ([https://github.com/paulmillr/chokidar](https://github.com/paulmillr/chokidar))（原库名 `chokidar` → CP 包名 `chokidar`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `chokidar` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add chokidar
```

```cp
import {
    WatchEvent,
    ChokidarOptions,
    TrackedEntry,
    FileWatcherCore,
    chkStrEq,
} from "chokidar";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `WatchEvent` | `class` | 字段: `kind: u8, path: string, mtime_ms: u64, size: u64` |
| `ChokidarOptions` | `class` | 字段: `ignore_initial: bool, await_write_finish_ms: u64, follow_symlinks: bool, depth: usize, ignore_dotfiles: bool` · 方法: `defaults` |
| `TrackedEntry` | `class` | 字段: `active: bool, path: string, is_dir: bool, mtime_ms: u64, size: u64, last_emit_ms: u64` |
| `FileWatcherCore` | `class` | 字段: `opts: ChokidarOptions, matcher: GlobPatternSet, tracked: [32]TrackedEntry, tracked_count: usize, events: [32]WatchEvent, event_head: usize, +1 项` · 方法: `new, addIgnore, isIgnored, chkEnqueue, recordSnapshot, recordUnlink, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `chkStrEq` | `(a: string, b: string): bool` | 调用 `chkStrEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { chkStrEq } from "chokidar";

pub fn main() {
    let result = chkStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
