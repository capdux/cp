[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# tsconfig-paths

Zero-allocation TypeScript tsconfig.json baseUrl + paths alias wildcard matcher and path resolver for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:tsconfig-paths` ([https://github.com/dividab/tsconfig-paths](https://github.com/dividab/tsconfig-paths))

| 元信息 | 值 |
| --- | --- |
| 包名 | `tsconfig-paths` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add tsconfig-paths
```

```cp
import {
    PathRule,
    PatternMatch,
    CandidateList,
    PathsMatcher,
    SegSpan,
    BestRuleHit,
    matchesPattern,
    appendByte,
} from "tsconfig-paths";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PathRule` | `class` | 字段: `pattern: string, targets: [4]string, target_count: usize` · 方法: `empty` |
| `PatternMatch` | `class` | 字段: `matched: bool, is_exact: bool, prefix_len: usize, captured: string` · 方法: `none` |
| `CandidateList` | `class` | 字段: `items: [4]string, count: usize` · 方法: `empty, itemAt` |
| `PathsMatcher` | `class` | 字段: `base_url: string, rules: [32]PathRule, rule_count: usize` · 方法: `init, addRule, addRuleMulti` |
| `SegSpan` | `class` | 字段: `start: usize, end: usize` |
| `BestRuleHit` | `class` | 字段: `found: bool, rule_idx: usize, match_info: PatternMatch` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `matchesPattern` | `(pattern: string, specifier: string): PatternMatch` | 调用 `matchesPattern` 并返回 `PatternMatch`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `writeNormalizedPath` | `(raw: string, out: []u8, start_pos: usize): usize` | 调用 `writeNormalizedPath` 并返回 `usize`。 |
| `expandTargetInto` | `(base_url: string, target: string, captured: string, out: []u8, start_pos: usize): usize` | 调用 `expandTargetInto` 并返回 `usize`。 |
| `findBestRule` | `(matcher: PathsMatcher, specifier: string): BestRuleHit` | 查询或读取目标数据，返回 `BestRuleHit`。 |
| `resolvePath` | `(matcher: PathsMatcher, specifier: string, out: []u8): string?` | 调用 `resolvePath` 并返回 `string?`。 |
| `resolveAllCandidates` | `(matcher: PathsMatcher, specifier: string, out_buf: []u8): CandidateList` | 调用 `resolveAllCandidates` 并返回 `CandidateList`。 |

## 最小调用示例

```cp
import { matchesPattern } from "tsconfig-paths";

pub fn main() {
    let result = matchesPattern("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
