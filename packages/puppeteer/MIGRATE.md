# Puppeteer CP 迁移与性能优化指南 (`packages/puppeteer`)

## 1. 迁移来源与版本信息
- **上游仓库**：`https://github.com/puppeteer/puppeteer`
- **上游包名与版本**：`puppeteer-core@25.19.0`
- **本地参考源码路径**：`_ref/puppeteer/packages/puppeteer-core/src/`
- **最近同步日期**：`2026-11-28`
- **一键同步命令**：
  ```bash
  python3 scripts/sync_upstream_pkg.py --pkg puppeteer --upstream _ref/puppeteer/packages/puppeteer-core
  ```

---

## 2. 源码对照表

| CP 模块 (`packages/puppeteer/src/`) | 上游源码 (`_ref/puppeteer/packages/puppeteer-core/src/`) | 核心职责与覆盖范围 |
| :--- | :--- | :--- |
| `index.cp` / `index.d.cp` | `index.ts` | 统一包入口导出与类型声明 |
| `types.cp` | `common/types.ts`, `common/Errors.ts` | `PuppeteerError`、`Viewport`、`LaunchOptions`、`Cookie` 等核心类型与错误映射 |
| `puppeteer.cp` | `node/PuppeteerNode.ts` | `launch` / `connect`（原生 `CdpNativeWsTransport` 零胶水闭环） / `launchWithTransport` / `connectWithTransport` / `defaultExecutablePath` |
| `browser.cp` | `cdp/Browser.ts`, `cdp/BrowserContext.ts` | `Browser`（可选持有 `*CdpNativeWsTransport`，`close` / `deinit` 自动关闭连接与回收浏览器子进程）与 `BrowserContext` 生命周期、多上下文隔离、目标管理 |
| `cdp_session.cp` | `cdp/CDPSession.ts` | 多目标 CDP 会话复用、命令收发与事件过滤 |
| `page.cp` | `cdp/Page.ts` | `Page` 全量 API（导航、截图、PDF、`emulate`、`waitForFunction`、事件、超时设置） |
| `frame.cp` | `cdp/Frame.ts` | `Frame` 导航、JS 求值、P-Selector 与 `waitForFunction`、无障碍树快照 |
| `handles.cp` | `cdp/JSHandle.ts`, `cdp/ElementHandle.ts` | `JSHandle` / `ElementHandle` DOM 交互、盒模型、截图与释放 |
| `network.cp` | `cdp/HTTPRequest.ts`, `cdp/HTTPResponse.ts`, `cdp/Dialog.ts` | 请求拦截、`HTTPResponse` 元数据与 `Dialog` 处理 |
| `devices.cp` | `common/Device.ts` | 完整 `KnownDevices` 全量表（155 款设备：全系 iPhone / iPad / Pixel / Galaxy / Nexus / Kindle / Blackberry / Moto / Desktop）与 `page.emulate(device)` |
| `tracing.cp` | `cdp/Tracing.ts` | `Page.tracing().start` / `stop`（复用 `cap:cdp` `TracingDomain`，支持 `screenshots`、`categories`、`buffer_size` 与文件输出） |
| `coverage.cp` | `cdp/Coverage.ts` | `Page.coverage().startJSCoverage` / `stopJSCoverage` / `startCSSCoverage` / `stopCSSCoverage`（复用 `ProfilerDomain` / `CSSDomain` 与 `convertToDisjointRanges` 扫描线合并） |
| `keyboard_layout.cp` | `common/USKeyboardLayout.ts` | 完整美式键盘 `KeyCode` / `Code` / `Text` / `Location` 与 Shift 解析表 |
| `input.cp` | `cdp/Input.ts` | `Keyboard` / `Mouse` / `Touchscreen` 控制器（集成 `USKeyboardLayout` 精确派发） |
| `selectors.cp` | `common/PSelectorParser.ts`, `common/WaitTask.ts`, `injected/*.ts` | P-Selector 解析（`::-p-text`/`::-p-xpath`/`::-p-aria`/`pierce/`/`>>>`）、注入脚本与 `WaitTask` / `Poller` |
| `events.cp` | `common/EventEmitter.ts` | 类型化 `EventEmitter`（`on` / `once` / `off` / `emit` / 通配符 `"*"`） |
| `extras.cp` | `common/FileChooser.ts`, `common/ConsoleMessage.ts`, `common/SecurityDetails.ts`, `common/TimeoutSettings.ts` | `FileChooser`、`ConsoleMessage`、`SecurityDetails`、`TimeoutSettings` |

---

## 3. CP 独有性能优化与内存安全设计

1. **所有权与确定性资源管理（RAII）**：
   - `Browser`、`Page`、`CDPSession`、`JSHandle`、`ElementHandle` 均实现幂等 `deinit()` 与 `~init()` 析构，配合 `defer` 确保零句柄泄漏。
   - 借用规则严格区分只读借用（`_`）与可变借用（`*_`），`FileChooser` 状态机通过 `handled` 标志防止重复 `accept()` / `cancel()`。
2. **零堆分配与 `= undefined` 栈缓冲区优化**：
   - `KnownDevices`（`devices.cp`）、`USKeyboardLayout`（`keyboard_layout.cp`）全部采用编译期静态常量表，单字节 ASCII 键走 O(1) 快速路径。
   - `EventEmitter`（固定 32 槽位）、`ParsedPSelector`（固定 16 段 + 512B unquote 栈池）、`SecurityDetails`（固定 8 项 SAN 列表）完全零堆分配。
   - 内部栈缓冲区（如 `pbuf` / `fbuf` / `unquote_buf` / `segments`）统一使用 `= undefined`，消除无意义栈清零开销。
3. **16-lane `@Vector(16, u8)` SIMD 热路径加速**：
   - `SimdSelectorScanner.needsPSelectorParsing` 与 `indexOfByteSimd` 使用 `@Vector(16, u8)` 并行扫描 `:`、`>`、`/` 等特殊字符；纯 CSS 选择器零解析直接命中原生 `DOM.querySelector` 快路径。

---

## 4. 功能边界与注意事项
- **原生无 Node.js 依赖**：底层传输直接基于 `cap:cdp`（`CdpEndpoint` / `CdpBrowserLauncher`），不依赖 Node.js 事件循环或 WebSocket JS 垫片。
- **固定池容量限制**：无堆分配路径下 `EventEmitter` 单实例上限 32 个监听器、`ParsedPSelector` 单选择器上限 16 段、`Page.cookies_store` 上限 32 条、`Page.extra_headers` 上限 16 项；若超出可通过外部缓冲区或切片扩展。
- **`::-p-aria` 选择器**：解析器已完整识别 `aria/` 与 `::-p-aria(...)` 并标记 `has_aria`，页面端可结合 `AccessibilitySnapshot` 或注入脚本进一步匹配。
