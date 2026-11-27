[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# quick-lru

Zero-heap two-generation QuickLRU cache (QuickLruI64) with O(1) rotation eviction (clean-room port of npm quick-lru)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:quick-lru` ([https://github.com/sindresorhus/quick-lru](https://github.com/sindresorhus/quick-lru))

| 元信息 | 值 |
| --- | --- |
| 包名 | `quick-lru` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add quick-lru
```

```cp
import { QlruSlot, QuickLruI64, qlruStrEq } from "quick-lru";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `QlruSlot` | `class` | 字段: `occupied: bool, key: string, val: i64, hash: u64` |
| `QuickLruI64` | `class` | 字段: `cur: [32]QlruSlot, old: [32]QlruSlot, cur_len: usize, old_len: usize, max_size: usize` · 方法: `new, maxSize, size, findInCur, findInOld, rotateGenerations, +7 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `qlruStrEq` | `(a: string, b: string): bool` | 调用 `qlruStrEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { qlruStrEq } from "quick-lru";

pub fn main() {
    let result = qlruStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
