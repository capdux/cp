[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# p-locate

Zero-allocation bounded-concurrency ordered/unordered locator planner (npm p-locate v7 equivalent) for CP, reusing p-limit.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-locate` ([https://github.com/sindresorhus/p-locate](https://github.com/sindresorhus/p-locate))

| 元信息 | 值 |
| --- | --- |
| 包名 | `p-locate` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add p-locate
```

```cp
import { PLocateOptions, LocateCandidate, PLocatePlanner, plocEmptyCandidate } from "p-locate";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PLocateOptions` | `class` | 字段: `concurrency: usize, preserve_order: bool` · 方法: `defaults` |
| `LocateCandidate` | `class` | 字段: `id: u32, matched: bool, finished: bool` |
| `PLocatePlanner` | `class` | 字段: `opts: PLocateOptions, limiter: PLimit, items: [32]LocateCandidate, count: usize, settled: bool, has_winner: bool, +1 项` · 方法: `new, defaultPlanner, addCandidate, dispatchNext, reportResult, isSettled, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `plocEmptyCandidate` | `(): LocateCandidate` | 调用 `plocEmptyCandidate` 并返回 `LocateCandidate`。 |

## 最小调用示例

```cp
import { plocEmptyCandidate } from "p-locate";

pub fn main() {
    let result = plocEmptyCandidate();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
