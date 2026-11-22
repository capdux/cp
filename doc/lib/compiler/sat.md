# CP 标准库：布尔可满足性 (SAT) 求解器与声明式约束建模引擎 (`cap:sat`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:compiler`，导入请使用 `import { ... } from "cap:compiler"`。

## 一、 架构总览与设计准则

`cap:sat` 是 CP 语言官方内置的纯 CP **布尔可满足性（SAT）求解器与高级约束建模引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为 CP 包依赖冲突解析、任务排班调度、配置合规校验与组合逻辑推理打造。既可通过 `import { ... } from "cap:compiler"` 导入，也支持 `import cap from "cap"; cap.sat.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/sat/solver.cp` | `// Source: cp/lib/sat/solver.cp (...)` | `SatError`、`SatStatus`、`SatSolution`、`NamedSolution`、`SatSolver`（DPLL + 单位传播 + 纯文字消除 + MOM 启发式 + DIMACS 解析）、`ConstraintModel` |
| `lib/sat/index.cp` | `// Source: cp/lib/sat/index.cp (...)` | `cap:sat` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与求解状态 (`SatError` / `SatStatus`)

- **`SatError`**：
  - `InvalidVariable`：变量编号非正数（如文字 `0`）或变量数非法。
  - `EmptyClause`：尝试添加空子句（或 DIMACS 中出现独立 `0` 空子句）。
  - `InvalidDimacs`：DIMACS CNF 文本缺少 `p cnf <vars> <clauses>` 头、格式错误或子句数不匹配。
  - `MaxDecisionsExceeded`：分支决策次数超过 `max_decisions` 上限。
  - `OutOfMemory`：底层切片或名字表内存分配失败。
- **`SatStatus`**：`Satisfiable` | `Unsatisfiable`。

### 2. 结果对象 (`SatSolution` / `NamedSolution`)

- **`SatSolution`**（由 `SatSolver.solve()` 返回）：
  - 字段：`status: SatStatus`、`assignment: []bool`（1-based 索引）、`decisions: u64`、`propagations: u64`
  - `deinit(*_)`：释放底层赋值数组。
  - `isTrue(var_id: i32): bool`：查询 1-based 变量是否被赋值为 `true`。
- **`NamedSolution`**（由 `ConstraintModel.solveNamed(alloc)` 返回）：
  - 字段：`decisions: u64`、`propagations: u64`
  - `deinit(*_)`：释放内部持有的变量名拷贝与布尔值数组。
  - `isSelected(name: string): bool`：按变量名查询是否被选中为 `true`。

### 3. `SatSolver`（1-based DPLL 求解器与 DIMACS 解析器）

- **生命周期与配置**：
  - `SatSolver.init(alloc: std.mem.Allocator): SatSolver`
  - `deinit(*_)`
  - `setMaxDecisions(*_, limit: u64)`（默认 `10_000_000`）
- **变量与子句构建**：
  - `newVar(*_): i32`：分配新的 1-based 变量编号。
  - `ensureVars(*_, n: i32): void ! SatError`
  - `addClause(*_, lits: []const i32): void ! SatError`：正数 `k` 表示 `x_k`，负数 `-k` 表示 `¬x_k`；拦截 `0` 与空子句。
- **DIMACS CNF 解析与求解**：
  - `SatSolver.parseDimacs(alloc: std.mem.Allocator, cnf_text: string): SatSolver ! SatError`：支持 `c` 注释行、`p cnf <vars> <clauses>` 头、跨行 `0` 结尾子句与 `%` EOF 标记。
  - `solve(): SatSolution ! SatError`：执行 DPLL（单位传播 + 纯文字消除 + MOM 最小子句高频分支启发式），并在返回 `Satisfiable` 前 100% 校验所有原始子句均被满足。

### 4. `ConstraintModel`（高级具名声明式约束建模器）

- **生命周期与变量声明**：
  - `ConstraintModel.init(alloc: std.mem.Allocator): ConstraintModel`
  - `deinit(*_)`
  - `declare(*_, name: string): i32 ! SatError`
- **声明式约束原语**：
  - `requireTrue(*_, name: string): void ! SatError`
  - `requireFalse(*_, name: string): void ! SatError`
  - `implies(*_, a: string, b: string): void ! SatError`（`a ⇒ b`）
  - `impliesAny(*_, a: string, choices: []const string): void ! SatError`（`a ⇒ (c_1 ∨ c_2 ∨ ...)`）
  - `conflicts(*_, a: string, b: string): void ! SatError`（`¬a ∨ ¬b`）
  - `allOf(*_, names: []const string): void ! SatError`
  - `anyOf(*_, names: []const string): void ! SatError`
  - `atMostOne(*_, names: []const string): void ! SatError`
  - `exactlyOne(*_, names: []const string): void ! SatError`
  - `requireIfAll(*_, conds: []const string, target: string): void ! SatError`（`(c_1 ∧ ... ∧ c_k) ⇒ target`）
- **一键求解**：
  - `solveNamed(alloc: std.mem.Allocator): NamedSolution? ! SatError`（可满足返回 `NamedSolution`，不可满足返回 `null`）。

---

## 三、 快速示例

```typescript
const std = @import("std");
import { ConstraintModel } from "cap:compiler";

fn resolveDeps(alloc: std.mem.Allocator): bool ! {
    let model = ConstraintModel.init(alloc);
    defer model.deinit();

    model.requireTrue("app")?;
    model.impliesAny("app", &["libA_v1", "libA_v2"])?;
    model.exactlyOne(&["libA_v1", "libA_v2"])?;
    model.implies("app", "plugin")?;
    model.implies("plugin", "libA_v2")?;

    if (sol := model.solveNamed(alloc)?) {
        defer sol.deinit();
        return sol.isSelected("libA_v2") && !sol.isSelected("libA_v1");
    }
    return false;
}
```
