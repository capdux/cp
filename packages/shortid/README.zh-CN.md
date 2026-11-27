[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# shortid

URL-friendly non-sequential short unique ID generator and validator (clean-room CP implementation of npm shortid)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shortid` ([https://github.com/dylang/shortid](https://github.com/dylang/shortid))

| 元信息 | 值 |
| --- | --- |
| 包名 | `shortid` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add shortid
```

```cp
import { ShortIdGenerator, sidU64, sidCharIndex, isValidShortId } from "shortid";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ShortIdGenerator` | `class` | 字段: `alphabet: [64]u8, worker_id: u8, fmt_version: u8, last_tick: u64, counter: u32, rng_state: u64` · 方法: `new, workerId, version, nextRand, encodeValue, generate, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sidU64` | `(hi: u32, lo: u32): u64` | 调用 `sidU64` 并返回 `u64`。 |
| `sidCharIndex` | `(c: u8, alphabet: string): i32` | 调用 `sidCharIndex` 并返回 `i32`。 |
| `isValidShortId` | `(id: string): bool` | 执行 `isValidShortId` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isValidShortId } from "shortid";

pub fn main() {
    let result = isValidShortId("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
