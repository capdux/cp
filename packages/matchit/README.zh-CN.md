[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# matchit

Zero-allocation URL router with static, named (:id / {id}), and catch-all (*rest / {*rest}) priority matching (Rust matchit parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:matchit` ([https://github.com/ibraheemdev/matchit](https://github.com/ibraheemdev/matchit))

| 元信息 | 值 |
| --- | --- |
| 包名 | `matchit` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add matchit
```

```cp
import {
    SegKind,
    RouteParam,
    RouteMatch,
    RouteSeg,
    RouteEntry,
    Router,
    emptyParam,
    noMatch,
} from "matchit";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `SegKind` | `Static, Param, CatchAll` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RouteParam` | `class` | 字段: `key: string, value: string` |
| `RouteMatch` | `class` | 字段: `matched: bool, handler_id: u32, params: [MAX_PARAMS]RouteParam, param_count: usize` · 方法: `param` |
| `RouteSeg` | `class` | 字段: `kind: SegKind, text: string` |
| `RouteEntry` | `class` | 字段: `active: bool, handler_id: u32, segs: [MAX_SEGS]RouteSeg, seg_count: usize` |
| `Router` | `class` | 字段: `routes: [MAX_ROUTES]RouteEntry, count: usize` · 方法: `init, insert, at, routeCount` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `emptyParam` | `(): RouteParam` | 调用 `emptyParam` 并返回 `RouteParam`。 |
| `noMatch` | `(): RouteMatch` | 调用 `noMatch` 并返回 `RouteMatch`。 |
| `emptySeg` | `(): RouteSeg` | 调用 `emptySeg` 并返回 `RouteSeg`。 |
| `emptyEntry` | `(): RouteEntry` | 调用 `emptyEntry` 并返回 `RouteEntry`。 |
| `segPriority` | `(kind: SegKind): u8` | 调用 `segPriority` 并返回 `u8`。 |
| `parseRoutePattern` | `(pattern: string, handler_id: u32): RouteEntry?` | 通过 `parseRoutePattern` 解析输入数据，返回 `RouteEntry?`。 |
| `routesConflict` | `(a: RouteEntry, b: RouteEntry): bool` | 调用 `routesConflict` 并返回 `bool`。 |
| `routeBetterThan` | `(cand: RouteEntry, best: RouteEntry): bool` | 调用 `routeBetterThan` 并返回 `bool`。 |
| `matchEntry` | `(entry: RouteEntry, path: string): RouteMatch?` | 调用 `matchEntry` 并返回 `RouteMatch?`。 |

## 最小调用示例

```cp
import { parseRoutePattern } from "matchit";

pub fn main() {
    let result = parseRoutePattern("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
