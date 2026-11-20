# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

## 2026-11-20: 提速治理全面固化（三轨解耦规范 + `verify_module.sh` + 动态 `cap` 路由 + 3 路正交并行）与 `cap:html`/`cap:json` 双路完工

- **治理与提速落地**：
  - 将全部标准库契约从 `cp_agent_ref.md` 剥离为独立 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，日常任务零读写 1 万行 `cp_ref.md`（语法修改仅在 `cp_agent_ref.md` 标题打 `[SYNC_PENDING]` 标记）；
  - 全量固化至 [`GEMINI.md`](file:///data/dev/cp/GEMINI.md) 并修正冲突旧条款；新增 [`scripts/verify_module.sh`](file:///data/dev/cp/scripts/verify_module.sh)、升级 [`scripts/sync_cpc.sh`](file:///data/dev/cp/scripts/sync_cpc.sh) 原子替换与 [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs) 自动探测 `lib/<ns>/index.cp`。
- **双路标准库验收**：`lib/html/`（entities/sanitizer/extractor）与 `lib/json/`（pointer/patch/mergePatch/canonical/ndjson）单文件与 `-m` 均 5/5 PASS，既有回归全部 100% PASS。

---

## 2026-11-20: `cap:http`（SSE / Multipart / CookieJar）与 `cap:logger`（结构化 JSON/Logfmt / W3C Trace / 采样器）双路完工

- **落地成果**：
  - `lib/http/` 新增 `sse.cp`（486 行）、`multipart.cp`（417 行）并升级 `cookie.cp`（810 行），顺手修复既有 `lib/http/decompressor.cp` 的 `WINDOW_BITS_AUTO` 显式导入打通 `cap:http` 模块化模式 `-m`（53 模块增量转译仅 17ms），单文件与 `-m` 均 5/5 PASS，既有 3 套 HTTP 回归 14/14 零回归；
  - `lib/logger/` 新增 `structured.cp`（713 行）、`trace.cp`（691 行）、`sampler.cp`（267 行），提供 JSON/Logfmt/PrettyAnsi 结构化日志与敏感字段脱敏、W3C `TraceParent`/`TraceState`/`Span` OTel 导出及 `RatioSampler`/`BurstSampler`/`RingLogBuffer`，单文件与 `-m` 均 5/5 PASS，既有回归 4/4 零回归；
  - 已采纳热文档冷热分离归档（`*_H.md`）并推行“子智能体最后一步导出 `index.cp`”的原子发布纪律，大幅减少主控上下文开销并消除下游误伤风险。
