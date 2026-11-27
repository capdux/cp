[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# keep-a-changelog

Zero-allocation Keep a Changelog 1.1.0 parser, release lookup, section counter, and header formatter (keep-a-changelog / changelog-parser / Rust parse-changelog parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:keep-a-changelog` ([https://github.com/oscarotero/keep-a-changelog](https://github.com/oscarotero/keep-a-changelog))（原库名 `keep-a-changelog` → CP 包名 `keep-a-changelog`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `keep-a-changelog` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add keep-a-changelog
```

```cp
import {
    ChangeKind,
    ReleaseInfo,
    ParsedChangelog,
    containsIgnoreCase,
    isRefLinkDef,
    classifySection,
    parseReleaseHeading,
    isTopLevelBullet,
} from "keep-a-changelog";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `ChangeKind` | `Added, Changed, Deprecated, Removed, Fixed, Security, Other` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ReleaseInfo` | `class` | 字段: `version: string, date: string, is_unreleased: bool, is_yanked: bool, body: string, added_count: usize, +3 项` · 方法: `empty` |
| `ParsedChangelog` | `class` | 字段: `releases: [MAX_RELEASES]ReleaseInfo, count: usize` · 方法: `empty, releaseAt` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `containsIgnoreCase` | `(hay: string, needle_lower: string): bool` | 调用 `containsIgnoreCase` 并返回 `bool`。 |
| `isRefLinkDef` | `(trimmed: string): bool` | 执行 `isRefLinkDef` 条件判定，返回 `bool`。 |
| `classifySection` | `(heading: string): ChangeKind` | 调用 `classifySection` 并返回 `ChangeKind`。 |
| `parseReleaseHeading` | `(rest_raw: string): ReleaseInfo` | 通过 `parseReleaseHeading` 解析输入数据，返回 `ReleaseInfo`。 |
| `isTopLevelBullet` | `(line: string): bool` | 执行 `isTopLevelBullet` 条件判定，返回 `bool`。 |
| `parseChangelog` | `(md: string): ParsedChangelog` | 通过 `parseChangelog` 解析输入数据，返回 `ParsedChangelog`。 |
| `findRelease` | `(log: ParsedChangelog, version: string): ReleaseInfo?` | 查询或读取目标数据，返回 `ReleaseInfo?`。 |
| `latestRelease` | `(log: ParsedChangelog): ReleaseInfo?` | 调用 `latestRelease` 并返回 `ReleaseInfo?`。 |
| `formatReleaseHeader` | `(version: string, date: string, yanked: bool, out: []u8): string` | 通过 `formatReleaseHeader` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { isRefLinkDef } from "keep-a-changelog";

pub fn main() {
    let result = isRefLinkDef("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
