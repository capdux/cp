# `cap:makefile_parse` — GNU `Makefile` 目标、`.PHONY` 与变量零分配解析库

> 参考来源：/data/dev/bun/src/cli/cli.zig | 吸收日期：2026-11-22

## 概述

`cap:makefile_parse` 吸收自 Bun CLI 构建脚本与任务发现工具体系（`src/cli/cli.zig`），提供纯 CP、零外部依赖、零堆分配的 GNU `Makefile` 规则、`.PHONY` 伪目标与顶层变量赋值解析能力，适用于任务运行器、IDE 目标自动补全与 CI 构建图静态分析。

## 规范契约与解析边界

- **精确区分变量赋值与规则定义**：
  - 完整识别 GNU Make 全部四种赋值运算符：`=`（递归展开 `recursive`）、`:=` / `::=`（立即展开 `simple`）、`?=`（条件赋值 `conditional`）、`+=`（追加 `append`），并兼容 `export` 与 `override` 前缀；
  - 避免将 `:=` / `::=` 变量赋值误识别为 `target: deps` 规则。
- **Recipe 与注释隔离**：
  - 自动剥离 UTF-8 BOM，跳过以 `\t` 开头的 recipe 命令行、空行与 `#` 注释行；
  - 对规则行支持双冒号规则（`target:: deps`）并自动剥离 `;` 后的内联 recipe 命令。
- **零拷贝行号追踪**：`MakeRule` 与 `MakeVar` 均保留 1-based `line_no` 与原始源码切片，无任何堆分配。

## 核心类型与函数

### `MakeAssignOp` & `MakeVar`

```cp
pub enum MakeAssignOp: u8 {
    recursive = 0,   // =
    simple = 1,      // := / ::=
    conditional = 2, // ?=
    append = 3,      // +=

    fn asStr(): string
}

pub struct MakeVar {
    name: []const u8 = "",
    op: MakeAssignOp = .recursive,
    value: []const u8 = "",
    line_no: usize = 0,
}
```

### `MakeRule`、`MakeRuleIter`、`MakeVarIter` & `isPhonyTarget`

```cp
pub struct MakeRule {
    target: []const u8 = "",
    deps_raw: []const u8 = "",
    line_no: usize = 0,
}

pub struct MakeRuleIter {
    static fn init(content: []const u8): MakeRuleIter
    fn next(*_): MakeRule?
}

pub struct MakeVarIter {
    static fn init(content: []const u8): MakeVarIter
    fn next(*_): MakeVar?
}

pub fn isPhonyTarget(content: []const u8, target: []const u8): bool
```

## 使用示例

```cp
import { MakeRuleIter, MakeVarIter, isPhonyTarget } from "cap:makefile_parse";

const mk = "CC := clang\n.PHONY: all clean\nall: build\n\t@echo ok\n";
const phony = isPhonyTarget(mk, "clean"); // true

let vars = MakeVarIter.init(mk);
while (v := vars.next()) { * = v; }

let rules = MakeRuleIter.init(mk);
while (r := rules.next()) { * = r; }
* = phony;
```
