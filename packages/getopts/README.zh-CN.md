[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# getopts

Zero-allocation POSIX/GNU command-line option parser (short flags, clustered flags, --opt=val, -- separator, required options) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:getopts` ([https://github.com/rust-lang/getopts](https://github.com/rust-lang/getopts), `MIT OR Apache-2.0`)

| 元信息 | 值 |
| --- | --- |
| 包名 | `getopts` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add getopts
```

```cp
import {
    HasArg,
    Occur,
    OptDef,
    OptVal,
    Matches,
    Options,
    goptStrEq,
    goptFindEq,
} from "getopts";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `HasArg` | `No, Yes, Maybe` |
| `enum` | `Occur` | `Optional, Req, Multi` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OptDef` | `class` | 字段: `short_name: string, long_name: string, desc: string, has_arg: HasArg, occur: Occur` · 方法: `empty` |
| `OptVal` | `class` | 字段: `opt_idx: usize, has_val: bool, val: string` · 方法: `empty` |
| `Matches` | `class` | 字段: `ok: bool, err_kind: i32, err_opt: string, defs: [32]OptDef, def_count: usize, vals: [64]OptVal, +3 项` · 方法: `optPresent, optCount, optStr, optStrs, freeCount, freeAt` |
| `Options` | `class` | 字段: `defs: [32]OptDef, def_count: usize` · 方法: `new, opt, optflag, optflagmulti, optopt, optmulti, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `goptStrEq` | `(a: string, b: string): bool` | 调用 `goptStrEq` 并返回 `bool`。 |
| `goptFindEq` | `(s: string): usize?` | 调用 `goptFindEq` 并返回 `usize?`。 |
| `goptFindByName` | `(defs: &[]OptDef, count: usize, name: string): usize?` | 调用 `goptFindByName` 并返回 `usize?`。 |
| `goptFindShort` | `(defs: &[]OptDef, count: usize, ch: string): usize?` | 调用 `goptFindShort` 并返回 `usize?`。 |
| `goptFindLong` | `(defs: &[]OptDef, count: usize, name: string): usize?` | 调用 `goptFindLong` 并返回 `usize?`。 |
| `goptCountForIdx` | `(vals: &[]OptVal, val_count: usize, idx: usize): usize` | 调用 `goptCountForIdx` 并返回 `usize`。 |

## 最小调用示例

```cp
import { goptStrEq } from "getopts";

pub fn main() {
    let result = goptStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
