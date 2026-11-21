# `cap:makefile_parse` — GNU `Makefile` 目标、`.PHONY` 与变量零分配解析库

> 来源：吸收自 `/data/dev/bun/src/cli.zig`（Makefile Target & Variable Parser）

`cap:makefile_parse` 提供纯 CP、零外部依赖、零堆分配的 GNU `Makefile` 规则、`.PHONY` 伪目标与顶层变量赋值解析能力。

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

- 自动剥离 UTF-8 BOM、跳过 `\t` 开头的 recipe 命令行、空行与 `#` 注释。
- 准确区分 `:=` / `::=` / `?=` / `+=` / `=` 变量赋值（含 `export` / `override` 前缀）与 `target: deps` 规则定义（含双冒号规则与 `;` 内联 recipe 剥离）。

## 使用示例

```cp
import { MakeRuleIter, MakeVarIter, isPhonyTarget } from "cap:makefile_parse";

const mk = "CC := clang\n.PHONY: all clean\nall: build\n\t@echo ok\n";
// isPhonyTarget(mk, "clean") == true
```
