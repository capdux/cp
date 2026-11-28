# Playwright CP 迁移与架构说明 (`MIGRATE.md`)

## 1. 上游来源与版本信息
- **上游仓库**: [microsoft/playwright](https://github.com/microsoft/playwright) (Apache-2.0)
- **参考包版本**: `playwright-core@1.66.0-next`
- **本地上游镜像路径**: `_ref/playwright/packages/playwright-core/src/`
- **最近同步日期**: `2026-11-28`

---

## 2. 上游与 CP 模块对照表

| CP 模块 (`packages/playwright/src/`) | 上游参考文件 (`_ref/playwright/packages/playwright-core/src/`) | 核心职责与适配说明 |
| :--- | :--- | :--- |
| `index.cp` / `index.d.cp` | `index.ts` | 统一导出 `chromium`、`Browser`、`BrowserContext`、`Page`、`Locator`、`Clock`、`ActionabilityEngine`、`HarRecorder`、`PlaywrightRemoteServer`、`PlaywrightCli` 等 |
| `chromium.cp` | `server/chromium/chromium.ts` | Chromium / Capsurf 进程探测与 `connectOverCDP`，复用 `cap:cdp` `CdpBrowserLauncher` |
| `browser.cp` | `server/browser.ts` | `Browser` 生命周期管理、`CdpEndpoint` 持有与 RAII `~init()`/`deinit()` |
| `context.cp` | `server/browserContext.ts` | `BrowserContext` 多上下文隔离、Cookie、`StorageState`、Context 级路由拦截 |
| `page.cp` | `server/page.ts` | `Page` 导航历史、内容注入、`screenshot`/`pdf`/`ariaSnapshot` 与 `page.clock()` |
| `locator.cp` | `client/locator.ts` | 语义定位器（role/text/label/placeholder/testid）、链式 `>>` 组合、`@Vector(16, u8)` SIMD 扫描与动作分派 |
| `actionability.cp` | `server/dom.ts` | Actionability 自动可操作性状态机（Visible、Stable、ScrollIntoView、Enabled/Editable、HitTarget、Trial/Force） |
| `clock.cp` | `server/clock.ts` | `Clock` 时间控制（`install`、`fastForward`、`fastForwardStr`、`runFor`、`pauseAt`、`resume`、`setFixedTime`、`setSystemTime`） |
| `artifacts.cp` | `server/download.ts`, `server/fileChooser.ts`, `server/console.ts`, `server/har/harRecorder.ts` | `Download`、`FileChooser`、`ConsoleMessage` 与 HAR 1.2 `HarRecorder`（复用 `cap:json` 序列化 + SIMD 扫描） |
| `network.cp` | `server/network.ts` | `Request`/`Response`/`Route` 与 `@Vector(16, u8)` SIMD 加速的 `UrlMatcher` Glob 路由器 |
| `aria.cp` | `server/ariaSnapshot.ts` | 无障碍树 `AriaSnapshot` 解析与 YAML 生成 |
| `remote.cp` | `remote/playwrightServer.ts`, `remote/playwrightConnection.ts`, `remote/serverTransport.ts` | 双模远程网关：CP 同进程直连 + 可选原生 `cap:web` `WsServer` WS RPC 多语言服务 |
| `cli.cp` | `cli/browserActions.ts` | Playwright CLI 动作解析与执行（`open`、`screenshot`、`pdf`、`codegen`） |

---

## 3. 架构决策：为什么同时支持「同进程直连」与「原生 `cap:web` WS RPC 网关」？

1. **CP 本地零开销直连（Zero-IPC In-Process）**：
   - 上游 Playwright 在 Node.js 中存在 `Client <-> Channel/Dispatcher <-> Server <-> CDP` 多层序列化与进程间通信（IPC）开销。
   - 在 CP 中，调用方直接通过 `chromium.launch()` -> `Page` -> `Locator` -> `cap:cdp` 与浏览器会话直连，消除中间 JSON-RPC 序列化与跨进程转发，实现亚毫秒级控制。
2. **可选原生 `cap:web` WS RPC 多语言网关（`PlaywrightRemoteServer`）**：
   - 为了兼容 Playwright 生态中的多语言客户端（Python / Java / .NET / 远程分布式节点），`remote.cp` 基于 `cap:web` 的多核 `SO_REUSEPORT` + `epoll` `WsServer`、以及 `cap:json` 提供了轻量级原生 WebSocket RPC 服务端。
   - 当不需要远程接入时，完全不启动 WS 监听线程，零额外资源占用；需要远程调度或跨语言接入时，调用 `PlaywrightRemoteServer.start()` 即可拉起高并发 WS RPC 网关。

---

## 4. CP 独有性能与安全优化

- **严格所有权与 RAII 生命周期**：所有持有堆内存的对象（`Browser`、`BrowserContext`、`Page`、`Download`、`FileChooser`、`HarRecorder`、`PlaywrightRemoteServer`、`PlaywrightRemoteClient`、`CliResult`）均实现 `deinit(*_)` 与 `~init()` 析构守卫，确保零内存泄漏。
- **借用语义清晰**：只读查询方法省略 `*_` 接收者，状态变更方法显式标注 `*_`。
- **避免栈缓冲清零开销**：所有临时脚本与 CDP 参数栈缓冲统一使用 `= undefined` 声明（如 `let params_buf: [2048]u8 = undefined;`），消除大型局部缓冲区的无谓清零。
- **Arena 与固定池**：`HarRecorder.toJson` 与 `PlaywrightRemoteServer.handleRpcMessage` 内部采用 `std.heap.ArenaAllocator` 集中分配 JSON AST 节点并在函数返回时一次性释放。
- **16-lane `@Vector(16, u8)` SIMD 加速**：
  - `network.cp` (`UrlMatcher.hasWildcardsSimd`)：16 字节并行探测 `*`/`?` 通配符，纯字面量 URL 直接走 `std.mem.eql` 快速路径。
  - `locator.cp` (`Locator.hasComplexSelectorSimd`)：16 字节并行扫描选择器组合符（`>`、`:`、`[`）。
  - `artifacts.cp` (`simdScanHarCleanSpan`)：16 字节并行扫描 HAR 文本转义边界。

---

## 5. 一键同步上游更新

当 `_ref/playwright/packages/playwright-core` 更新时，可通过以下脚本一键检查并同步上游变更：

```bash
python3 scripts/sync_upstream_pkg.py --pkg playwright --upstream _ref/playwright/packages/playwright-core
```
