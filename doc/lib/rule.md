# CP 标准库：声明式生产规则引擎手册 (`cap:rule`)

## 一、 架构总览与设计准则

`cap:rule` 是 CP 语言官方内置的纯 CP **声明式生产规则引擎（Production Rule Engine）**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，为业务风控反欺诈、动态定价路由与 AI Agent 策略决策提供工业级实现。既可通过 `import { ... } from "cap:rule"` 导入，也支持 `import cap from "cap"; cap.rule.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/rule/rule.cp` | `// Source: cp/lib/rule/rule.cp (...)` | `RuleError`、`FactValue`、`FactMap`、`CompareOp`、`Condition`、`RuleSpec`、`FiredRule`、`RuleEvalReport`、`RuleEngine` |
| `lib/rule/index.cp` | `// Source: cp/lib/rule/index.cp (...)` | `cap:rule` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与事实上下文 (`RuleError` / `FactValue` / `FactMap`)

- **`RuleError`**：
  - `DuplicateRuleId`：注册重复的 `RuleSpec.id`。
  - `RuleNotFound`：`setEnabled` 目标规则 ID 不存在。
  - `MaxDepthExceeded`：条件 AST 嵌套深度超过 `MAX_CONDITION_DEPTH`（32 层）。
  - `TypeMismatch`：`Compare` 操作符与事实字段值类型不兼容（例如对 `Int` 执行 `Contains` 或对 `Str` 执行 `Gt`）。
  - `OutOfMemory`：事实表、规则 AST 克隆或评估审计报告内存分配失败。
- **`FactValue`**：强类型标量联合体 `Str(string)` | `Int(i64)` | `Float(f64)` | `Bool(bool)`，支持 `FactValue.str/int/float/boolean` 便捷构造，并在数值比较时支持 `Int` 与 `Float` 的精确跨类型比较。
- **`FactMap`**：
  - `FactMap.init(alloc: std.mem.Allocator): FactMap` / `deinit(*_)`
  - `setStr(*_, key: string, val: string): void ! RuleError`（内部深拷贝键与字符串值）
  - `setInt(*_, key: string, val: i64): void ! RuleError`
  - `setFloat(*_, key: string, val: f64): void ! RuleError`
  - `setBool(*_, key: string, val: bool): void ! RuleError`
  - `get(key: string): FactValue?` / `count(): usize`

### 2. 条件谓词 AST (`CompareOp` / `Condition`)

- **`CompareOp`**：`Eq`、`Neq`、`Gt`、`Gte`、`Lt`、`Lte`、`Contains`、`StartsWith`
- **`Condition`**：
  - `And([]const Condition)` / `Condition.andAll(children)`：短路逻辑与
  - `Or([]const Condition)` / `Condition.orAny(children)`：短路逻辑或
  - `Not(*const Condition)` / `Condition.notExpr(child)`：逻辑非
  - `Compare(CompareSpec)` / `Condition.cmp(field, op, expected)`：字段比较（字段缺失时安全返回 `false`）
  - `Exists(string)` / `Condition.exists(field)`：判断字段是否存在于 `FactMap`
  - `InList(InListSpec)` / `Condition.inList(field, items)`：集合成员匹配（字段缺失时安全返回 `false`）

### 3. 规则引擎与审计报告 (`RuleSpec` / `FiredRule` / `RuleEvalReport` / `RuleEngine`)

- **`RuleSpec`**：`{ id, name, salience: i32 = 0, activation_group: string? = null, enabled: bool = true, condition: Condition, action_tag: string, action_payload: string = "" }`
- **`FiredRule`**：`{ rule_id, salience, activation_group, action_tag, action_payload }`
- **`RuleEvalReport`**：
  - 字段：`matched: []FiredRule`、`evaluated_count: usize`、`skipped_by_group_count: usize`
  - 方法：`deinit(*_)`、`topAction(): FiredRule?`
- **`RuleEngine`**：
  - `RuleEngine.init(alloc: std.mem.Allocator): RuleEngine` / `deinit(*_)`
  - `addRule(*_, spec: RuleSpec): void ! RuleError`（按 `salience` 降序稳定插入，同权重保持注册先后顺序，深拷贝规则与条件树）
  - `setEnabled(*_, rule_id: string, enabled: bool): void ! RuleError`
  - `ruleCount(): usize`
  - `evaluate(alloc: std.mem.Allocator, facts: &FactMap): RuleEvalReport ! RuleError`（按 `salience` 降序逐条评估启用规则；非空 `activation_group` 一旦命中即记录并跳过同组其余低优先级规则）

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    FactMap,
    FactValue,
    CompareOp,
    Condition,
    RuleSpec,
    RuleEngine,
} from "cap:rule";
import cap from "cap";

pub fn demoRuleEngine(): ! {
    const alloc = std.heap.page_allocator;

    let engine = RuleEngine.init(alloc);
    defer engine.deinit();

    const tiers = [FactValue.Str("gold"), FactValue.Str("platinum")];
    const cond_items = [
        Condition.inList("tier", &tiers),
        Condition.cmp("amount", .Gte, FactValue.Float(100.0)),
    ];

    engine.addRule(RuleSpec{
        id: "vip-discount",
        name: "VIP Discount",
        salience: 100,
        activation_group: "pricing",
        condition: Condition.andAll(&cond_items),
        action_tag: "APPLY_DISCOUNT",
        action_payload: "20%",
    })?;

    let facts = FactMap.init(alloc);
    defer facts.deinit();
    facts.setStr("tier", "gold")?;
    facts.setInt("amount", 150)?;

    let report = engine.evaluate(alloc, &facts)?;
    defer report.deinit();

    if (top := report.topAction()) {
        std.debug.print("Fired: {s} -> {s}\n", .{ top.rule_id, top.action_payload });
    }
    * = cap.rule.MAX_CONDITION_DEPTH;
}
```
