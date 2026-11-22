# CP 标准库：AI Agent 安全护栏引擎手册 (`cap:guardrail`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

## 一、 架构总览与设计准则

`cap:guardrail` 是 CP 语言官方内置的纯 CP、零 C 依赖 **AI Agent 安全护栏（Guardrails）引擎**，面向百万级 Agent 并发运行时提供高精度的敏感信息扫描脱敏与多级输入/输出策略审查。全部模块 0 `std.posix`、0 `@import("bun")`。

### 1. 模块文件拓扑

| 文件路径 | 溯源注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/guardrail/pii.cp` | `// Source: cp/lib/guardrail/pii.cp (PII & Secret Scanner / Redactor)` | `GuardrailError`、`PiiKind`、`PiiFinding`、`RedactionResult`、`PiiScanner`（含 Luhn 校验） |
| `lib/guardrail/policy.cp` | `// Source: cp/lib/guardrail/policy.cp (Guardrail Policy & Inspection Pipeline)` | `GuardrailAction`、`GuardrailVerdict`、`GuardrailPolicy`（多级审查流水线） |
| `lib/guardrail/index.cp` | `// Source: cp/lib/guardrail/index.cp (Guardrails Subsystem Entry)` | `cap:guardrail` & `cap.guardrail` 统一对外出口 |

---

## 二、 PII 与高危密钥扫描脱敏 (`pii.cp`)

### 1. 支持识别的敏感实体 (`PiiKind`)

| 枚举变体 | 分类标签 (`label()`) | 识别与零误报边界规则 |
| :--- | :--- | :--- |
| `Email` | `"EMAIL"` | 本地部分 + `@` + 合法多级域名与纯字母 TLD |
| `Ipv4` | `"IPV4"` | 4 段 `0..255` 点分十进制，拒绝前导零与版本号片段 |
| `CreditCard` | `"CREDIT_CARD"` | 13~19 位数字（支持空格/`-` 分组），**强制通过 Luhn 校验算法**才认定 |
| `ApiKey` | `"API_KEY"` | 识别 `sk-...`、`ghp_...`/`gho_...`/`ghu_...`/`ghs_...`/`ghr_...`、`xoxb-` 等令牌并校验标识符前导边界 |
| `AwsAccessKey` | `"AWS_ACCESS_KEY"` | 精确匹配 `AKIA` + 16 位大写字母或数字 |
| `JwtToken` | `"JWT_TOKEN"` | 匹配 `eyJ...` 开头的三段式 Base64URL 结构 |
| `Phone` | `"PHONE"` | 国际前缀 `+` 或带分隔符的 10~15 位电话号码，避免普通整数误报 |

### 2. 核心 API

- `PiiScanner.scan(alloc, text)`: 扫描文本并返回不重叠的 `[]PiiFinding` 切片（每个 `PiiFinding` 包含 `kind`、`start`、`end` 与脱敏预览 `masked_preview`）。调用方使用 `PiiScanner.freeFindings(alloc, findings)` 释放。
- `PiiScanner.redact(alloc, text, mask_token)`: 将全部敏感片段替换为固定掩码 `mask_token`，返回 `RedactionResult`。
- `PiiScanner.redactTyped(alloc, text)`: 将每个敏感片段替换为分类占位符（如 `[REDACTED:EMAIL]`、`[REDACTED:API_KEY]`），返回 `RedactionResult`。
- `PiiScanner.isValidLuhn(text)`: 独立的 Luhn 校验辅助方法。
- `RedactionResult.deinit(alloc)`: 一键释放脱敏文本与内部 findings 列表。

---

## 三、 多级审查策略引擎 (`policy.cp`)

### 1. 审查流水线 (`GuardrailPolicy`)

`GuardrailPolicy` 按以下确定性流水线顺序执行检查：

1. **长度预算拦截 (`setMaxChars`)**：超出字符上限返回 `GuardrailAction.Block`，原因 `"length_limit_exceeded"`；
2. **提示词边界逃逸/越界特征拦截 (`addInjectionSignature` / `addDefaultInjectionSignatures`)**：仅在 `inspectInput` 阶段执行大小写不敏感匹配，命中返回 `GuardrailAction.Block`，原因 `"boundary_escape_signature"`；
3. **违禁短语拦截 (`addBlockedPhrase`)**：大小写不敏感匹配，命中返回 `GuardrailAction.Block`，原因 `"blocked_phrase"`；
4. **PII 与高危密钥自动脱敏 (`setRedactPii` / `setMaskToken`)**：命中敏感信息时自动脱敏并返回 `GuardrailAction.Redact`（`blocked = false`，`sanitized_text` 为脱敏后文本，`finding_count > 0`）。
5. **合规放行**：未触发任何规则时返回 `GuardrailAction.Allow`。

### 2. 完整使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { PiiScanner, GuardrailPolicy, GuardrailAction } from "cap:ml";

pub fn runGuardrailDemo(): !void {
    const alloc = std.heap.page_allocator;

    // 1. 敏感信息分类脱敏
    let red = PiiScanner.redactTyped(alloc, "Contact dev@cp-lang.org with key sk-proj1234567890abcdef")?;
    defer red.deinit(alloc);

    // 2. 构建多级策略
    let policy = GuardrailPolicy.init(alloc);
    defer policy.deinit();
    policy.setMaxChars(2048);
    policy.addDefaultInjectionSignatures()?;
    policy.addBlockedPhrase("restricted-internal-codename")?;

    let verdict = policy.inspectInput(alloc, "Please summarize report for alice@example.com")?;
    defer verdict.deinit(alloc);
    // verdict.action == GuardrailAction.Redact

    // 3. 动态命名空间挂载
    let out_verdict = cap.guardrail.PiiScanner.isValidLuhn("4532015112830366");
    _ = out_verdict;
}
```
