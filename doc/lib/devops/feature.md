# CP 标准库：Feature Flags 功能开关与实验分流引擎手册 (`cap:feature`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:devops`，导入请使用 `import { ... } from "cap:devops"`。

## 一、 架构总览与设计准则

`cap:feature` 是 CP 语言官方内置的纯 CP **Feature Flags 功能开关、确定性灰度分桶（Consistent Bucketing）、属性靶向规则引擎与多变体 A/B/n 实验分流引擎**，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，专为后端微服务渐进式发布与 AI 模型 / Prompt 灰度实验设计。既可通过 `import { ... } from "cap:devops"` 导入，也支持 `import cap from "cap"; cap.feature.*` 动态懒挂载调用。

```text
┌──────────────────────────────────────────────────────────────────────────┐
│                 FeatureEngine 严格评估流水线 (Evaluation Pipeline)        │
├──────────────────────────────────────────────────────────────────────────┤
│  1. Flag 查找      ──▶ 缺失则返回 default_val / FlagNotFound (零崩溃)    │
│  2. KillSwitch     ──▶ killed == true 立即熔断返回 false / KillSwitch    │
│  3. TargetingRule  ──▶ 按注册顺序匹配上下文属性，命中即返回 RuleMatch     │
│  4. Consistent     ──▶ FNV-1a(flag_key + ":" + user_id) % 10000          │
│     Bucketing          ├─ BoolFlag: bucket < rollout_pct * 100           │
│                        └─ Experiment: 累计基点权重分流 (总和 10000 bps)  │
└──────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/feature/flags.cp` | `// Source: cp/lib/feature/flags.cp (Pure CP Feature Flags, Consistent Bucketing & Experimentation Engine)` | `FeatureError`、`MatchOp`、`EvalReason`、`EvalContext`、`TargetingRule`、`VariantWeight`、`FeatureDecision`、`consistentBucket`、`FeatureEngine` |
| `lib/feature/index.cp` | `// Source: cp/lib/feature/index.cp (Unified Exports for cap:feature)` | `cap:feature` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误、操作符与评估原因

- **`BUCKET_SCALE: u32 = 10000`**：分桶基点总规模（`0..9999`，支持精确到 `0.01%` 粒度的实验权重）。
- **`FeatureError`**：
  - `FlagNotFound`：开关不存在（或传入空 `key`）。
  - `InvalidPercentage`：灰度百分比不在 `0..100` 范围内。
  - `InvalidWeights`：实验变体列表为空、变体名为空或权重总和不等于 `10000` 基点。
  - `DuplicateFlag`：重复定义同名开关或实验。
  - `OutOfMemory`：内存分配失败。
- **`MatchOp`**：`Equals`、`NotEquals`、`Contains`、`StartsWith`、`InList`（逗号分隔列表）、`NumericGt`、`NumericLt`。
- **`EvalReason`**：`StaticDisabled`、`KillSwitch`、`RuleMatch`、`RolloutIncluded`、`RolloutExcluded`、`DefaultFallback`、`FlagNotFound`。

### 2. 上下文与规则类型

- **`EvalContext`**：
  - `EvalContext.init(alloc, user_id)` / `deinit()`：初始化/释放评估上下文。
  - `setUserId(user_id)`：原地更新 `user_id`（复用上下文对象，零多余字典重建）。
  - `setAttr(key, val)` / `getAttr(key): string?`：设置与读取属性键值对。
- **`TargetingRule`**：`{ attr_key, op: MatchOp, value, enabled_result: bool = true, variant_result: string = "" }`。
- **`VariantWeight`**：`{ name: string, weight_bps: u32 }`（所有变体 `weight_bps` 之和必须为 `10000`）。
- **`FeatureDecision`**：`{ flag_key: string, enabled: bool, variant: string, reason: EvalReason, bucket: u32 }`。

### 3. `FeatureEngine` 与确定性分桶

- **`FeatureEngine.init(alloc)` / `deinit()`**：创建与释放引擎，自动清理内部所有开关、规则与变体内存。
- **`defineBoolFlag(key, default_enabled, rollout_pct)`**：定义布尔灰度开关（校验 `0..100`）。
- **`defineExperiment(key, variants, default_variant)`**：定义多变体 A/B/n 实验（校验非空及总权重 `10000` 基点）。
- **`addRule(flag_key, rule)`**：为指定开关追加靶向规则（优先级高于百分比灰度与实验分桶）。
- **`setKillSwitch(flag_key, killed)`**：一键熔断开关，开启后立即覆盖所有规则与灰度返回 `false` / `KillSwitch`。
- **`setRollout(flag_key, pct)`**：动态调整灰度百分比（`0..100`）。由于 `consistentBucket(flag_key, user_id)` 仅依赖 `flag_key` 与 `user_id`，将灰度从 `25%` 提升到 `50%` 时，原 `25%` 桶内用户 **100% 单调保持命中**。
- **`evaluateBool(flag_key, ctx)` / `evaluateBoolWithDefault(flag_key, ctx, default_val)`**：评估布尔开关。
- **`evaluateVariant(flag_key, ctx)` / `evaluateVariantWithDefault(flag_key, ctx, default_variant)`**：评估实验变体。
- **`toJsonSnapshot(): ([]u8) ! FeatureError`**：导出当前所有开关与灰度/实验配置的合法 JSON 快照（调用方负责用引擎同一 `alloc` 释放）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { FeatureEngine, EvalContext, TargetingRule, VariantWeight, MatchOp } from "cap:devops";
import cap from "cap";

pub fn demoFeature(): ! {
    const alloc = std.heap.page_allocator;
    let engine = cap.feature.FeatureEngine.init(alloc);
    defer engine.deinit();

    engine.defineBoolFlag("ai_streaming", false, 25)?;
    engine.addRule("ai_streaming", TargetingRule{
        attr_key: "tier",
        op: .InList,
        value: "enterprise,vip",
        enabled_result: true,
    })?;

    let ctx = EvalContext.init(alloc, "user-42")?;
    defer ctx.deinit();
    ctx.setAttr("tier", "enterprise")?;

    const decision = engine.evaluateBool("ai_streaming", ctx);
    // decision.enabled == true, decision.reason == .RuleMatch
    * = decision;
}
```
