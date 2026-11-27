[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# id-arena

Zero-allocation typed handle arena allocator with arena_id cross-arena misuse protection in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:id-arena` ([https://github.com/fitzgen/id-arena](https://github.com/fitzgen/id-arena), `MIT OR Apache-2.0`)

| 元信息 | 值 |
| --- | --- |
| 包名 | `id-arena` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add id-arena
```

```cp
import { ArenaId, IdArenaI64, IdArenaU64, idaIsValid } from "id-arena";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ArenaId` | `class` | 字段: `index: usize, arena_id: u32` |
| `IdArenaI64` | `class` | 字段: `arena_id: u32, items: [128]i64, count: usize` · 方法: `new, alloc, get, set, len, isEmpty, +3 项` |
| `IdArenaU64` | `class` | 字段: `arena_id: u32, items: [128]u64, count: usize` · 方法: `new, alloc, get, set, len, isEmpty, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `idaIsValid` | `(id: ArenaId, expected_arena_id: u32, count: usize): bool` | 调用 `idaIsValid` 并返回 `bool`。 |

## 最小调用示例

```cp
import { idaIsValid } from "id-arena";

pub fn main() {
    let result = idaIsValid(/* id: ArenaId */, 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
