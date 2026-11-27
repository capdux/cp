[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ignore

Zero-heap .gitignore specification rule engine (clean-room CP implementation of npm ignore)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ignore` ([https://github.com/kaelzhang/node-ignore](https://github.com/kaelzhang/node-ignore))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ignore` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ignore
```

```cp
import {
    IgnoreRule,
    IgnoreMatcher,
    igTrimRightSpaces,
    igHasInnerSlash,
    igRuleMatches,
    igMatchSingle,
} from "ignore";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `IgnoreRule` | `class` | 字段: `pat: [128]u8, pat_len: usize, negated: bool, dir_only: bool, anchored: bool` |
| `IgnoreMatcher` | `class` | 字段: `rules: [64]IgnoreRule, count: usize, ignore_case: bool` · 方法: `new, newCaseInsensitive, ruleCount, addRule, addLines, ignores, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `igTrimRightSpaces` | `(s: string): usize` | 调用 `igTrimRightSpaces` 并返回 `usize`。 |
| `igHasInnerSlash` | `(s: string, start: usize, end: usize): bool` | 调用 `igHasInnerSlash` 并返回 `bool`。 |
| `igRuleMatches` | `(rule: IgnoreRule, rel_path: string, is_dir: bool, case_insensitive: bool): bool` | 调用 `igRuleMatches` 并返回 `bool`。 |
| `igMatchSingle` | `(pat: &[]u8, anchored: bool, path: string, case_insensitive: bool): bool` | 调用 `igMatchSingle` 并返回 `bool`。 |

## 最小调用示例

```cp
import { igTrimRightSpaces } from "ignore";

pub fn main() {
    let result = igTrimRightSpaces("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
