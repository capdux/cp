# `cap:cdp` (`lib/cdp/`) 迁移与上游同步指南 (`MIGRATE.md`)

> **定位**：`puppeteer` 与 `playwright` 共用的零拷贝原生 Chrome DevTools Protocol (CDP 1.3) 核心底座。  
> **同步时间戳**：`2026-11-28`

## 1. 上游来源对照表

| CP 模块 | 上游参考来源 | 职责 |
| :--- | :--- | :--- |
| [protocol.cp](./protocol.cp) | `_ref/puppeteer/packages/puppeteer-core/src/cdp/Connection.ts` & CDP JSON-RPC | 16-lane SIMD 字符串/结构扫描与零分配请求/响应/事件解码 |
| [session.cp](./session.cp) | `_ref/puppeteer/packages/puppeteer-core/src/cdp/CDPSession.ts` & `_ref/playwright/.../crConnection.ts` | 多 Target / SessionId 复用、回调挂起与事件分发 |
| [launcher.cp](./launcher.cp) | `_ref/puppeteer/packages/browsers/src/launch.ts` & `_ref/playwright/.../crBrowser.ts` | 跨平台 Chromium/Chrome/Edge/Capsurf 自动探测与启动参数构造 |
| [domains.cp](./domains.cp) | `devtools-protocol` (`Page`/`Runtime`/`DOM`/`Input`/`Network`/`Accessibility`) | 核心高频域参数构造与 AXNode 零堆解析 |
| [devtools_types.cp](./devtools_types.cp) | `devtools-protocol` (`Target`/`Emulation`/`Fetch`/`Storage`/`Browser`/`Log`/`Tracing`) | 扩展 CDP 强类型域零堆请求构造器 |
| [index.cp](./index.cp) | `cap:cdp` 聚合出口 | 统一对外导出与 `cdp` 命名空间 |

## 2. CP 架构优化点
1. **所有权与零泄漏**：`CdpEndpoint` / `CdpSession` 显式持有固定槽位与 `deinit()` RAII 释放，经 `DebugAllocator` 校验 0 字节泄漏。
2. **SIMD 与 `undefined` 优化**：`CdpFrameScanner` 使用 `@Vector(16, u8)` SIMD 加速引号/结构字节扫描，发送参数缓冲使用外部切片或栈上 `= undefined` 避免清零开销。
3. **跨包零重复**：同时驱动 `packages/puppeteer` 与 `packages/playwright`，消除双份 CDP 实现。

## 3. 上游更新后一键同步步骤

```bash
# 1. 拉取上游变更并审计所有 // Source: 溯源头
python3 scripts/sync_upstream_pkg.py --pkg cap:cdp --upstream _ref/puppeteer/packages/puppeteer-core --pull

# 2. 更新时间戳（合并完成后）
python3 scripts/sync_upstream_pkg.py --pkg cap:cdp --stamp

# 3. 回归校验
cpc check lib/cdp/*.cp
cap test -m cap/test/test_cdp_production_deep.cp
```

## 4. 全量 Chromium CDP `protocol.d.ts` 强类型生成 (`lib/cdp/protocol_types/`)
- 已通过 [scripts/gen_protocol_cp.py](../../scripts/gen_protocol_cp.py) 全量吸收 `_ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts`（24,886 行）：
  - 生成 **13 个模块化 `.cp` 文件**（10,502 行，单文件均 < 1,100 行）；
  - 完整覆盖 **59 个 CDP Domain**、**2,197 个强类型 Struct/Type**、**232 个 Event 名常量**、**677 个 Command 名常量**，`cpc check` 与 `zig ast-check` 13/13 全过。
