[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# playwright (CP)

Playwright browser automation and end-to-end testing library for CP (`playwright` / `playwright-core` / `playwright-chromium` compatible), built zero-copy on top of `cap:cdp` (`lib/cdp/`).

## Features

- **Zero-Copy CDP Foundation**: Directly reuses `cap:cdp`'s `CdpBrowserLauncher`, `CdpEndpoint`, `CdpSession`, `CdpFrameScanner`, and typed CDP domains (`PageDomain`, `RuntimeDomain`, `InputDomain`, `AccessibilityDomain`).
- **Browser Lifecycle**: `chromium.launch`, `chromium.connectOverCDP`, `chromium.executablePath` (auto-probing `chromium`, `google-chrome`, `chromium-browser`, `msedge`, `capsurf`).
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
