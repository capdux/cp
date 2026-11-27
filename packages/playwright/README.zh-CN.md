[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# playwright (CP 中文文档)

面向 CP 语言的 Playwright 浏览器自动化与端到端测试库（兼容 `playwright` / `playwright-core` / `playwright-chromium`），底层零拷贝复用 `cap:cdp`（`lib/cdp/`）。

## 核心特性

- **零拷贝复用 `cap:cdp`**：直接复用 `CdpBrowserLauncher`、`CdpEndpoint`、`CdpSession`、`CdpFrameScanner` 以及强类型 CDP 域（`PageDomain`、`RuntimeDomain`、`InputDomain`、`AccessibilityDomain`）。
- **浏览器启动与连接**：支持 `chromium.launch`、`chromium.connectOverCDP`、`chromium.executablePath`（自动探测 `chromium`、`google-chrome`、`chromium-browser`、`msedge`、`capsurf`）。
- **上下文隔离 (`BrowserContext`)**：支持 `newPage`、`pages`、`cookies`、`addCookies`、`clearCookies`、`storageState`、`route`、`unroute`、`close`。
- **页面控制 (`Page`)**：支持 `goto`、`reload`、`goBack`、`goForward`、`title`、`url`、`content`、`setContent`、`evaluate`、`locator`、`getByRole`、`getByText`、`getByLabel`、`getByPlaceholder`、`getByTestId`、`click`、`fill`、`typeText`、`press`、`waitForSelector`、`route`、`ariaSnapshot`、`screenshot`、`pdf`、`close`。
- **Locator 语义引擎**：支持链式定位（`locator`、`first`、`last`、`nth`、`filter`）与自动等待动作（`click`、`fill`、`press`、`check`、`uncheck`、`selectOption`、`textContent`、`innerText`、`getAttribute`、`isVisible`、`isEnabled`、`count`、`waitFor`）。
- **网络拦截与无障碍快照**：支持 `Route`（`fulfill`、`abort`、`continue_`、`fallback`）、`Request`、`Response` 与 `AriaSnapshot` YAML 导出。

## 快速开始

```cp
import { chromium } from "playwright";

pub fn main(): !void {
    const alloc = std.heap.c_allocator;
    let browser = chromium.launch(alloc, .{ headless: true })?;
    defer browser.deinit();

    const page = browser.newPage()?;
    * = page.goto("https://example.com", .{})?;

    const btn = page.getByRole("button", .{ name: "Submit" }).first();
    btn.click()?;
}
```
