[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# browserslist

Zero-heap Browserslist query clause parser and browser version matcher

| 元信息 | 值 |
| --- | --- |
| 包名 | `browserslist` |
| 版本 | `4.24.4` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add browserslist
```

```cp
import {
    BrowserOp,
    BrowserClause,
    blqStartsWithWord,
    blqParseU32,
    blqIsDeadBrowser,
    blqCmpVersion,
    parseBrowserClause,
    matchBrowserVersion,
} from "browserslist";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `BrowserOp` | `Ge, Gt, Le, Lt, Eq, LastN, Defaults, Dead ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BrowserClause` | `class` | 字段: `negated: bool, is_and: bool, browser: string, op: BrowserOp, version_major: u32, version_minor: u32, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `blqStartsWithWord` | `(s: string, word: string): bool` | 调用 `blqStartsWithWord` 并返回 `bool`。 |
| `blqParseU32` | `(s: string): u32` | 调用 `blqParseU32` 并返回 `u32`。 |
| `blqIsDeadBrowser` | `(browser: string): bool` | 调用 `blqIsDeadBrowser` 并返回 `bool`。 |
| `blqCmpVersion` | `(maj_a: u32, min_a: u32, maj_b: u32, min_b: u32): i32` | 调用 `blqCmpVersion` 并返回 `i32`。 |
| `parseBrowserClause` | `(clause: string): BrowserClause` | 通过 `parseBrowserClause` 解析输入数据，返回 `BrowserClause`。 |
| `matchBrowserVersion` | `(clause: BrowserClause, browser: string, major: u32, minor: u32, latest_major: u32): bool` | 调用 `matchBrowserVersion` 并返回 `bool`。 |

## 最小调用示例

```cp
import { parseBrowserClause } from "browserslist";

pub fn main() {
    let result = parseBrowserClause("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
