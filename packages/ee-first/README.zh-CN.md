[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ee-first

Zero-heap pure CP multi-emitter first-event race listener with automatic cleanup (clean-room MIT implementation of npm ee-first v1)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ee-first` ([https://github.com/jonathanong/ee-first](https://github.com/jonathanong/ee-first))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ee-first` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ee-first
```

```cp
import { EventWatch, FirstResult, FirstRace, eefStrEq } from "ee-first";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `EventWatch` | `class` | 字段: `emitter_id: u32, event_name: string, active: bool` |
| `FirstResult` | `class` | 字段: `triggered: bool, emitter_id: u32, event_name: string, payload_code: i32, cancelled_count: usize` · 方法: `empty` |
| `FirstRace` | `class` | 字段: `watchers: [16]EventWatch, count: usize, settled: bool, result: FirstResult` · 方法: `new, add, emit, cancel, activeCount, isSettled, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `eefStrEq` | `(a: string, b: string): bool` | 调用 `eefStrEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { eefStrEq } from "ee-first";

pub fn main() {
    let result = eefStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
