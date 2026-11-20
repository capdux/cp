# CP 标准库：企业级权限控制引擎手册 (`cap:rbac`)

## 一、 架构总览与设计准则

`cap:rbac` 是 CP 语言官方内置的纯 CP 企业级 RBAC / IAM 策略控制标准库，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为多租户后端与 AI Agent 工具调用权限门控打造，既可通过 `import { ... } from "cap:rbac"` 导入，也支持 `import cap from "cap"; cap.rbac.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    CP 企业级 RBAC / IAM 权限引擎 (cap:rbac)              │
├──────────────────────────────────┬──────────────────────────────────────┤
│ 通配符匹配器 (permission.cp)      │ 分层策略评估引擎 (engine.cp)          │
│ RbacError / PolicyEffect         │ RbacEngine (DAG 继承 + 环路检测)       │
│ PermissionMatcher (:* / :**)     │ AccessDecision (显式拒绝优先 + 审计)   │
└──────────────────────────────────┴──────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/rbac/permission.cp` | `// Source: cp/lib/rbac/permission.cp (Wildcard Permission Matcher & Policy Effect)` | `RbacError`、`PolicyEffect`、`PermissionMatcher`（分段合法性校验、单段 `*` 与尾段 `**` 匹配） |
| `lib/rbac/engine.cp` | `// Source: cp/lib/rbac/engine.cp (Hierarchical RBAC Engine with Explicit-Deny-Overrides & Audit Trail)` | `AccessDecision`、`RbacEngine`（角色定义、分层继承、环路防护、显式拒绝优先鉴权、有效角色展开） |
| `lib/rbac/index.cp` | `// Source: cp/lib/rbac/index.cp (Unified Exports for cap:rbac)` | `cap:rbac` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与策略效果（`permission.cp`）

- **`RbacError`**：
  - `InvalidPermission`：权限模式格式非法（空字符串、前导/尾随/连续冒号、含空白/控制字符、非法通配符如 `foo*` 或非尾段 `**`）。
  - `RoleNotFound`：引用的角色名称尚未通过 `defineRole` 定义。
  - `CycleDetected`：`addRoleInheritance` 会引入角色继承环路（含自环与间接环路）。
  - `OutOfMemory`：底层内存分配失败。
- **`PolicyEffect`**：`Allow` (`0`)、`Deny` (`1`)。

### 2. 通配符权限匹配器 `PermissionMatcher`（`permission.cp`）

- **`isValid(perm: string): bool`**：
  - 按 `:` 分段校验非空段，仅允许可打印非空白 ASCII；
  - 单段 `*` 匹配恰好 1 个分段（如 `"repo:*:read"`、`"tool:*"`、`"*:*"`）；
  - 尾段 `**` 匹配 1 个或多个后续分段（如 `"repo:**"` 匹配 `"repo:issues"` 与 `"repo:issues:read"`，但不匹配 `"repo"`）；非尾段 `**`（如 `"repo:**:read"`）与部分通配符（如 `"repo:read*"`）均视为非法并返回 `false`。
- **`isValidConcrete(perm: string): bool`**：要求权限格式合法且不包含任何 `*` 通配符。
- **`matches(pattern: string, requested: string): bool`**：要求 `pattern` 满足 `isValid` 且 `requested` 满足 `isValidConcrete`，按 `:` 逐段执行精确、单段 `*` 与尾段 `**` 匹配。

### 3. 审计决策与 RBAC 策略引擎（`engine.cp`）

- **`AccessDecision`**：
  - `allowed: bool`（是否放行）
  - `effect: PolicyEffect`（`.Allow` 或 `.Deny`）
  - `matched_rule: string`（命中规则模式；默认拒绝时为空字符串 `""`）
  - `matched_role: string`（命中角色名；主体级拒绝或默认拒绝时为空字符串 `""`）
  - 其中的字符串切片直接借用 `RbacEngine` 内部所有权缓冲，零额外分配且生命周期随 `RbacEngine` 至 `deinit()`。
- **`RbacEngine`**：
  - **生命周期**：`init(alloc)`、`deinit()`（全部角色名、主体 ID 与权限规则均独立深拷贝托管，安全抗调用方缓冲篡改）。
  - **角色与策略管理**：
    - `defineRole(role_name): void ! RbacError`（幂等定义角色）
    - `grantPermission(role_name, perm_pattern): void ! RbacError`（为角色添加允许规则）
    - `denyPermission(role_name, perm_pattern): void ! RbacError`（为角色添加显式拒绝规则）
    - `addRoleInheritance(child_role, parent_role): void ! RbacError`（声明 `child_role` 继承 `parent_role`，BFS 检测环路并返回 `RbacError.CycleDetected`）
  - **主体绑定与鉴权**：
    - `assignRole(subject_id, role_name): void ! RbacError`（幂等绑定角色）
    - `revokeRole(subject_id, role_name): void ! RbacError`（动态撤销角色；角色未定义报 `RoleNotFound`，主体未持有则为安全空操作）
    - `denySubjectPermission(subject_id, perm_pattern): void ! RbacError`（添加主体级显式拒绝）
    - `effectiveRoles(alloc, subject_id): ([]string) ! RbacError`（BFS 展开全部直接与继承角色并去重，调用方需 `alloc.free(roles)`）
    - `check(subject_id, requested_perm): AccessDecision`：严格遵循 **AWS IAM / Casbin** 评估顺序：
      1. 非法具体权限或未知主体 -> 默认拒绝（`Default Deny`）；
      2. **主体级显式拒绝**（最高优先级，覆盖所有角色允许）；
      3. **角色级显式拒绝**（扫描全部直接与继承角色上的 `Deny` 规则，优先级高于任何 `Allow`）；
      4. **角色级允许**（任一有效角色命中 `Allow` 即放行）；
      5. 未命中任何规则 -> 默认拒绝（`Default Deny`）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { RbacEngine, PolicyEffect } from "cap:rbac";
import cap from "cap";

pub fn demoRbac(): ! {
    const alloc = std.heap.page_allocator;
    let engine = RbacEngine.init(alloc);
    defer engine.deinit();

    engine.defineRole("viewer")?;
    engine.defineRole("editor")?;
    engine.addRoleInheritance("editor", "viewer")?;

    engine.grantPermission("viewer", "repo:issues:read")?;
    engine.grantPermission("editor", "repo:issues:*")?;
    engine.denyPermission("viewer", "repo:issues:delete")?;

    engine.assignRole("agent-01", "editor")?;

    const d1 = engine.check("agent-01", "repo:issues:write");  // allowed == true (via editor -> repo:issues:*)
    const d2 = engine.check("agent-01", "repo:issues:delete"); // allowed == false (explicit deny from viewer)

    // 通过 cap.rbac 动态挂载校验通配符
    const ok = cap.rbac.PermissionMatcher.matches("tool:**", "tool:fs:read");
    * = d1;
    * = d2;
    * = ok;
}
```
