[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# playwright (CP)

Playwright browser automation and end-to-end testing library for CP (`playwright` / `playwright-core` / `playwright-chromium` compatible), built zero-copy on top of `cap:cdp` (`lib/cdp/`).

## Features

- **Zero-Copy CDP Foundation**: Directly reuses `cap:cdp`'s `CdpBrowserLauncher`, `CdpEndpoint`, `CdpSession`, `CdpFrameScanner`, and typed CDP domains (`PageDomain`, `RuntimeDomain`, `InputDomain`, `AccessibilityDomain`).
- **Browser Lifecycle**: `chromium.launch`, `chromium.connectOverCDP`, `chromium.executablePath` (auto-probing `chromium`, `google-chrome`, `chromium-browser`, `msedge`, `capsurf`), plus **WebKit Engine** (`webkit.launch`, `webkit.launchWithTransport`, `webkit.connectWithTransport`, `webkit.executablePath`, `WkConnection` / `WkPageProxySession`).
- **BrowserContext Isolation**: `newContext`, `newPage`, `pages`, `cookies`, `addCookies`, `clearCookies`, `storageState`, `route`, `unroute`, `close`.
- **Page Automation**: `goto`, `reload`, `goBack`, `goForward`, `title`, `url`, `content`, `setContent`, `evaluate`, `click`, `fill`, `typeText`, `press`, `waitForSelector`, `route`, `ariaSnapshot`, `screenshot`, `pdf`, `close`.
- **Playwright Locator Engine**: Chaining (`locator`, `first`, `last`, `nth`, `filter`), semantic locators (`getByRole`, `getByText`, `getByLabel`, `getByPlaceholder`, `getByTestId`), and auto-waiting actions (`click`, `fill`, `press`, `check`, `uncheck`, `selectOption`, `textContent`, `innerText`, `getAttribute`, `isVisible`, `isEnabled`, `count`, `waitFor`).
- **Network Interception & AriaSnapshot**: `Route` (`fulfill`, `abort`, `continue_`, `fallback`), `Request`, `Response`, `UrlMatcher` globbing, and `AriaSnapshot` YAML rendering.

## Quick Start

```cp
import { chromium } from "playwright";

pub fn main(): !void {
    const alloc = std.heap.c_allocator;
    let browser = chromium.launch(alloc, .{ headless: true })?;
    defer browser.deinit();

    const page = browser.newPage()?;
    * = page.goto("https://example.com", .{})?;

    const submit = page.getByRole("button", .{ name: "Submit" }).first();
    submit.click()?;
}
```

## WebKit Browser Installation

> **Important:** This package provides **only the pure-CP WebKit Inspector protocol driver and session layer** (`webkit`, `WebKitBrowserType`, `WkConnection`, `WebKitBrowser`, `WebKitPage`). It **does not bundle the open-source WebKit browser binary**.

To run real WebKit / Safari compatibility tests, install Playwright's official open-source patched WebKit binary separately:

```bash
# Install Playwright's patched WebKit browser binary
npx playwright install webkit

# (Optional) Install to a custom directory
PLAYWRIGHT_BROWSERS_PATH=/opt/pw-browsers npx playwright install webkit

# (Linux) Install required system shared libraries for WebKit (WPE / GTK)
npx playwright install-deps webkit
```

### Specifying the WebKit Executable Path

`webkit.executablePath(alloc, preferred)` automatically probes:
1. `WebKitLaunchOptions.executable_path` (when passed to `webkit.launch`)
2. `PLAYWRIGHT_WEBKIT_EXECUTABLE_PATH` environment variable
3. `$PLAYWRIGHT_BROWSERS_PATH/webkit-*/` (`pw_run.sh`, `minibrowser-wpe/MiniBrowser`, `minibrowser-gtk/MiniBrowser`)
4. `~/.cache/ms-playwright/webkit-*/` and `/root/.cache/ms-playwright/webkit-*/`
5. System `MiniBrowser` paths (`/usr/bin/MiniBrowser`, `/usr/local/bin/MiniBrowser`, etc.)
