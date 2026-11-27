[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# puppeteer

High-level browser automation API for CP built directly on top of the native `cap:cdp` (Chrome DevTools Protocol) foundation.

<!-- cap-source-attribution -->
> **Reference / Upstream**: `npm:puppeteer-core` ([https://github.com/puppeteer/puppeteer](https://github.com/puppeteer/puppeteer)) (CP Package: `puppeteer`)

| Metadata | Value |
| --- | --- |
| Package | `puppeteer` |
| Version | `25.19.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add puppeteer
```

```cp
import {
    Puppeteer,
    Browser,
    BrowserContext,
    Page,
    Frame,
    ElementHandle,
    CDPSession,
    LaunchOptions,
    GoToOptions,
    launch,
    launchWithTransport,
    connect,
    defaultExecutablePath,
} from "puppeteer";
```

## Core Architecture

`puppeteer` delegates all JSON-RPC framing, multi-target `sessionId` multiplexing, executable probing, and domain builders (`Page`, `Runtime`, `DOM`, `Input`, `Network`, `Accessibility`) to `cap:cdp`:

- **Entrypoints**: `launch`, `launchWithTransport`, `connect`, `connectWithTransport`, `defaultExecutablePath`, `Puppeteer.buildLaunchArgs`, `Puppeteer.parseDevToolsWsUrl`
- **Browser & Contexts**: `Browser` (`newPage`, `pages`, `createBrowserContext`, `browserContexts`, `defaultBrowserContext`, `wsEndpoint`, `version`, `userAgent`, `isConnected`, `disconnect`, `close`), `BrowserContext`
- **Page & Frame**: `goto`, `goBack`, `goForward`, `reload`, `content`, `setContent`, `title`, `url`, `evaluate`, `setViewport`, `screenshot`, `pdf`, `click`, `typeText`, `focus`, `hover`, `tap`, `waitForSelector`, `querySelector`, `querySelectorAll`, `setExtraHttpHeaders`, `setRequestInterception`, `cookies`, `setCookie`, `deleteCookie`, `accessibilitySnapshot`, `createCDPSession`, `close`
- **Input & Handles**: `Keyboard`, `Mouse`, `Touchscreen`, `ElementHandle`, `JSHandle`
- **Network & Dialogs**: `HTTPRequest`, `HTTPResponse`, `Dialog`, `CDPSession`

## Minimal Example

```cp
const std = @import("std");
import { launchWithTransport, LaunchOptions, GoToOptions } from "puppeteer";

pub fn main(): !void {
    const alloc = std.heap.page_allocator;
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{ headless: true },
        "ws://127.0.0.1:9222/devtools/browser/demo",
        myTransportFn,
        myTransportCtx,
    )?;
    defer browser.deinit();

    const page = browser.newPage()?;
    page.goto("https://example.com", GoToOptions{})?;
    let eval_buf: [512]u8 = undefined;
    const res = page.evaluate("document.title", &eval_buf)?;
    * = res;
}
```

## CP Ownership & Safety Highlights

- **Shared `cap:cdp` engine**: Zero duplicated JSON-RPC or WebSocket framing logic.
- **Deterministic RAII**: `Browser`, `Page`, and `JSHandle` implement `~init()` / idempotent `deinit()` for leak-free cleanup.
- **Caller-owned output buffers**: Methods like `evaluate`, `screenshot`, `pdf`, `title`, and `content` write into caller-provided `[]u8` slices.
- **Explicit mutability**: Mutating methods declare `* _` receivers and return `T!PuppeteerError` or `T?` instead of throwing exceptions.
