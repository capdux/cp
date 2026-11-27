[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# tapable

Zero-allocation plugin hook engine (SyncHook, SyncBailHook, SyncWaterfallHook, SyncLoopHook) with stage ordering and before insertion (clean-room MIT implementation of npm tapable v2, #242 / 195M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:tapable` ([https://github.com/webpack/tapable](https://github.com/webpack/tapable)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `tapable` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add tapable
```

```cp
import {
    TapEntry,
    SyncHook,
    SyncBailHook,
    SyncWaterfallHook,
    SyncLoopHook,
    tapEmptyEntry,
    tapStrEq,
    tapInsertSorted,
} from "tapable";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TapEntry` | `class` | 字段: `name: string, stage: i32, handler_id: u32, enabled: bool` |
| `SyncHook` | `class` | 字段: `entries: [32]TapEntry, count: usize` · 方法: `init, tap, tapStage, tapBefore, setEnabled, isUsed, +3 项` |
| `SyncBailHook` | `class` | 字段: `entries: [32]TapEntry, count: usize` · 方法: `init, tap, tapStage, tapBefore, setEnabled, isUsed, +3 项` |
| `SyncWaterfallHook` | `class` | 字段: `entries: [32]TapEntry, count: usize` · 方法: `init, tap, tapStage, tapBefore, setEnabled, isUsed, +3 项` |
| `SyncLoopHook` | `class` | 字段: `entries: [32]TapEntry, count: usize` · 方法: `init, tap, tapStage, tapBefore, setEnabled, isUsed, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `tapEmptyEntry` | `(): TapEntry` | 调用 `tapEmptyEntry` 并返回 `TapEntry`。 |
| `tapStrEq` | `(a: string, b: string): bool` | 调用 `tapStrEq` 并返回 `bool`。 |
| `tapInsertSorted` | `(entries: []TapEntry, count: usize, entry: TapEntry): usize` | 调用 `tapInsertSorted` 并返回 `usize`。 |
| `tapInsertBefore` | `(entries: []TapEntry, count: usize, name: string, before_name: string, handler_id: u32): usize` | 调用 `tapInsertBefore` 并返回 `usize`。 |
| `tapSetEnabled` | `(entries: []TapEntry, count: usize, name: string, enabled: bool): bool` | 调用 `tapSetEnabled` 并返回 `bool`。 |
| `tapCollectOrdered` | `(entries: &[]TapEntry, count: usize, out: []u32): usize` | 调用 `tapCollectOrdered` 并返回 `usize`。 |

## 最小调用示例

```cp
import { tapStrEq } from "tapable";

pub fn main() {
    let result = tapStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
