# `cap:ci_info` — CI/CD 运行环境与 PR 构建检测库

> 参考来源：/data/dev/bun/src/cli/ci_info.cp | 吸收日期：2026-11-22

## 概述

`cap:ci_info` 吸收自 Bun CLI 的 CI 环境探测模块（`src/cli/ci_info.cp`，移植自 `ci-info@4.0.0` 规范），采用纯 CP 零堆分配实现，覆盖 31 个主流 CI/CD 供应商与 Pull Request 构建状态检测，用于控制终端色彩/进度条降级、测试报告格式切换以及包管理器非交互模式。

## 上游行为契约与 CP 增强

- **显式关闭优先（`CI=false` / `CI=0`）**：严格遵循 Bun `ci_info.cp` 逻辑（`if (bun.env_var.CI.get() == false) return null;`）——当环境变量中显式设置 `CI=false` 或 `CI=0` 时，即使存在其他供应商特征变量，也会覆盖并判定为非 CI 环境（`is_ci == false`、`vendor == .unknown`）。
- **通用 CI 兜底与 `name()` 可空语义**：遵循上游注释提示——`is_ci` 可以为 `true` 而 `name()` 返回 `null`（例如仅设置了通用的 `CI=true`、`CONTINUOUS_INTEGRATION=true` 或 `BUILD_ID`，但未匹配到 31 个已知供应商之一）。
- **CP 确定性纯函数与 `is_pr` 补全**：
  - 上游 Bun 仅实现 `isCI()` 与 `detectCIName()`，未实现 `isPR`，且通过 `bun.once` 缓存全局进程环境；
  - CP 标准库改为纯函数接口 `detectFromEntries(entries: []const EnvEntry): CiInfo`，由调用方注入环境变量切片，零全局状态、100% 可单测，并完整补全了各主流平台的 `is_pr`（Pull Request / Merge Request）构建检测。

## 导出类型与函数

### `CiVendor`

覆盖 31 个主流 CI/CD 供应商（`unknown = 0` + 31 个平台，包括 `github_actions`、`gitlab`、`circleci`、`travis`、`jenkins`、`buildkite`、`azure_pipelines`、`aws_codebuild`、`bitbucket`、`appveyor`、`vercel`、`netlify`、`cloudflare_pages`、`cloudflare_workers`、`drone`、`teamcity`、`bamboo`、`bitrise`、`buddy`、`cirrus`、`codefresh`、`codemagic`、`render`、`railway`、`woodpecker`、`semaphore`、`harness`、`google_cloud_build`、`agola`、`appcircle`、`github_codespaces`）：

- `fn displayName(): []const u8` — 返回人类可读名称（如 `"GitHub Actions"`、`"GitLab CI"`、`"Unknown"`）。
- `fn constantName(): []const u8` — 返回 `ci-info` 标准常量名（如 `"GITHUB_ACTIONS"`、`"GITLAB"`、`"UNKNOWN"`）。

### `EnvEntry`

```cp
pub struct EnvEntry {
    key: []const u8,
    value: []const u8,
}
```

### `CiInfo`

- 字段：`is_ci: bool`、`is_pr: bool`、`vendor: CiVendor`
- `fn name(): ([]const u8)?` — 当 `vendor != .unknown` 时返回 `vendor.displayName()`，否则返回 `null`（即使 `is_ci == true`）。

### `detectFromEntries`

- `detectFromEntries(entries: []const EnvEntry): CiInfo` — 零堆分配遍历环境变量表，确定性检测 CI 供应商、通用 CI 兜底与 PR 构建状态。

## 边界注意事项

1. **环境变量优先级**：若同一 `entries` 中存在 `CI=false` 或 `CI=0`，优先视为显式禁用 CI 检测。
2. **非 PR 平台默认 `false`**：对于不支持或未设置 PR 特征变量的 CI 平台，`is_pr` 安全返回 `false`。
3. **空环境表**：传入空切片时返回 `CiInfo{ is_ci: false, is_pr: false, vendor: .unknown }`。

## 使用示例

```cp
import { EnvEntry, CiVendor, detectFromEntries } from "cap:ci_info";

const env = [
    EnvEntry{ key: "GITHUB_ACTIONS", value: "true" },
    EnvEntry{ key: "GITHUB_EVENT_NAME", value: "pull_request" },
];
const info = detectFromEntries(env[0..]);
// info.is_ci == true, info.is_pr == true, info.vendor == CiVendor.github_actions
// info.name().? == "GitHub Actions"
* = info;
```
