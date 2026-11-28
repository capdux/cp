[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# playwright (CP 中文文档)

面向 CP 语言的 Playwright 浏览器自动化与端到端测试库（兼容 `playwright` / `playwright-core` / `playwright-chromium`），底层零拷贝复用 `cap:cdp`（`lib/cdp/`）。

## 核心特性

- **零拷贝复用 `cap:cdp`**：直接复用 `CdpBrowserLauncher`、`CdpEndpoint`、`CdpSession`、`CdpFrameScanner` 以及强类型 CDP 域（`PageDomain`、`RuntimeDomain`、`InputDomain`、`AccessibilityDomain`）。
- **浏览器启动与连接**：支持 `chromium.launch`、`chromium.connectOverCDP`、`chromium.executablePath`（自动探测 `chromium`、`google-chrome`、`chromium-browser`、`msedge`、`capsurf`），以及 **WebKit 引擎驱动**（`webkit.launch`、`webkit.launchWithTransport`、`webkit.connectWithTransport`、`webkit.executablePath`、`WkConnection` / `WkPageProxySession`）。
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

## WebKit 浏览器安装说明

> **重要提示：** 当前项目**仅包含纯 CP 版 WebKit 协议驱动与会话层**（`webkit`、`WebKitBrowserType`、`WkConnection`、`WebKitBrowser`、`WebKitPage`），**不包含开源 WebKit 浏览器二进制文件本身**。

如需运行真实的 WebKit / Safari 兼容性测试，请自行安装 Playwright 官方开源定制版 WebKit 二进制：

```bash
# 安装 Playwright 官方定制版 WebKit 浏览器
npx playwright install webkit

# （可选）指定自定义浏览器缓存路径
PLAYWRIGHT_BROWSERS_PATH=/opt/pw-browsers npx playwright install webkit

# （Linux 环境）安装 WebKit 运行所需的系统依赖（WPE / GTK）
npx playwright install-deps webkit
```

### 指定 WebKit 可执行文件路径

`webkit.executablePath(alloc, preferred)` 会按以下优先级自动探测：
1. `WebKitLaunchOptions.executable_path`（通过 `webkit.launch` 传入）
2. 环境变量 `PLAYWRIGHT_WEBKIT_EXECUTABLE_PATH`
3. `$PLAYWRIGHT_BROWSERS_PATH/webkit-*/`（`pw_run.sh`、`minibrowser-wpe/MiniBrowser`、`minibrowser-gtk/MiniBrowser`）
4. `~/.cache/ms-playwright/webkit-*/` 与 `/root/.cache/ms-playwright/webkit-*/`
5. 系统路径（`/usr/bin/MiniBrowser`、`/usr/local/bin/MiniBrowser` 等）
