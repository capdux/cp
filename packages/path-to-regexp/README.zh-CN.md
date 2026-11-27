[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# path-to-regexp

Zero-heap Express/Router URL path template matching, parameter extraction, and reverse route compilation (clean-room port of path-to-regexp v8)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:path-to-regexp` ([https://github.com/pillarjs/path-to-regexp](https://github.com/pillarjs/path-to-regexp)) / `npm:path-to-regexp` ([https://github.com/pillarjs/path-to-regexp](https://github.com/pillarjs/path-to-regexp))

| 元信息 | 值 |
| --- | --- |
| 包名 | `path-to-regexp` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add path-to-regexp
```

```cp
import {
    RouteParam,
    RouteMatch,
    ptrStrEq,
    ptrSegEq,
    ptrTrimSlashes,
    ptrFindSegEnd,
    ptrSkipSlashes,
    ptrWriteByte,
} from "path-to-regexp";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RouteParam` | `class` | 字段: `name: string, value: string` · 方法: `new` |
| `RouteMatch` | `class` | 字段: `matched: bool, params: [16]RouteParam, param_count: usize` · 方法: `noMatch, getParam` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ptrStrEq` | `(a: string, b: string): bool` | 调用 `ptrStrEq` 并返回 `bool`。 |
| `ptrSegEq` | `(a: string, b: string, case_sensitive: bool): bool` | 调用 `ptrSegEq` 并返回 `bool`。 |
| `ptrTrimSlashes` | `(s: string): string` | 调用 `ptrTrimSlashes` 并返回 `string`。 |
| `ptrFindSegEnd` | `(s: string, pos: usize): usize` | 调用 `ptrFindSegEnd` 并返回 `usize`。 |
| `ptrSkipSlashes` | `(s: string, pos: usize): usize` | 调用 `ptrSkipSlashes` 并返回 `usize`。 |
| `ptrWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `ptrWriteByte` 并返回 `usize`。 |
| `ptrWriteSlice` | `(out: []u8, pos: usize, s: string): usize` | 调用 `ptrWriteSlice` 并返回 `usize`。 |
| `ptrFindParam` | `(params: &[]RouteParam, name: string): string?` | 调用 `ptrFindParam` 并返回 `string?`。 |
| `matchPath` | `(pattern: string, path: string, case_sensitive: bool): RouteMatch` | 调用 `matchPath` 并返回 `RouteMatch`。 |
| `compilePath` | `(pattern: string, params: &[]RouteParam, out: []u8): usize` | 调用 `compilePath` 并返回 `usize`。 |
| `countRouteParams` | `(pattern: string): usize` | 调用 `countRouteParams` 并返回 `usize`。 |

## 最小调用示例

```cp
import { ptrStrEq } from "path-to-regexp";

pub fn main() {
    let result = ptrStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
