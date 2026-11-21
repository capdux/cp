# `cap:ci_info`

纯 CP 零堆分配 CI/CD 环境与 PR 构建检测库（遵循 ci-info v4 规范），吸收自 `/data/dev/bun/src/cli/ci_info.cp` 与 `codegen/ci_info.ts`。

## 导出类型与函数

- `CiVendor`：覆盖 31 个主流 CI/CD 供应商（GitHub Actions、GitLab CI、CircleCI、Travis、Jenkins、Buildkite、Azure Pipelines、AWS CodeBuild、Vercel、Netlify、Cloudflare Pages/Workers 等），提供 `displayName()` 与 `constantName()`
- `EnvEntry`：环境变量键值对 `{ key, value }`
- `CiInfo`：`{ is_ci, is_pr, vendor }` 及 `name(): ([]const u8)?`
- `detectFromEntries(entries)`：确定性检测 CI 供应商与 PR 状态，兼容 `CI=false` / `CI=0` 显式关闭与通用 `CI` / `BUILD_ID` 兜底
