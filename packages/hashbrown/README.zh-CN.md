[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hashbrown

Zero-heap SwissTable-style control-byte hash map and hash set (clean-room CP implementation of crates.io #1 hashbrown)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hashbrown` ([https://github.com/rust-lang/hashbrown](https://github.com/rust-lang/hashbrown))

| 元信息 | 值 |
| --- | --- |
| 包名 | `hashbrown` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add hashbrown
```

```cp
import {
    HbGetResult,
    HbInsertResult,
    HashMapU64,
    HashSetU64,
    hbHashU64,
    hbH2,
} from "hashbrown";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HbGetResult` | `class` | 字段: `found: bool, value: i64` |
| `HbInsertResult` | `class` | 字段: `ok: bool, replaced: bool, old_value: i64` |
| `HashMapU64` | `class` | 字段: `ctrl: [128]u8, keys: [128]u64, vals: [128]i64, used: usize` · 方法: `new, len, capacity, isEmpty, clear, findSlot, +4 项` |
| `HashSetU64` | `class` | 字段: `inner: HashMapU64` · 方法: `new, len, isEmpty, clear, insert, contains, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hbHashU64` | `(key: u64): u64` | 调用 `hbHashU64` 并返回 `u64`。 |
| `hbH2` | `(h: u64): u8` | 调用 `hbH2` 并返回 `u8`。 |

## 最小调用示例

```cp
import { hbHashU64 } from "hashbrown";

pub fn main() {
    let result = hbHashU64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
