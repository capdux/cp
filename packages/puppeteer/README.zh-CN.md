[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# puppeteer

基于 CP 原生标准协议底座 `cap:cdp`（Chrome DevTools Protocol）构建的高层浏览器自动化 SDK。

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:puppeteer-core` ([https://github.com/puppeteer/puppeteer](https://github.com/puppeteer/puppeteer))（CP 包名 `puppeteer`）

| 元数据 | 值 |
| --- | --- |
| 包名 | `puppeteer` |
| 版本 | `25.19.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

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

## 核心架构

`puppeteer` 全面复用 `cap:cdp` 的 JSON-RPC 帧扫描、多 Target `sessionId` 路由、浏览器路径探测及强类型域构建器（`Page`、`Runtime`、`DOM`、`Input`、`Network`、`Accessibility`）：

- **顶层入口**：`launch`、`launchWithTransport`、`connect`、`connectWithTransport`、`defaultExecutablePath`、`Puppeteer.buildLaunchArgs`、`Puppeteer.parseDevToolsWsUrl`
- **浏览器与上下文**：`Browser`（`newPage`、`pages`、`createBrowserContext`、`browserContexts`、`defaultBrowserContext`、`wsEndpoint`、`version`、`userAgent`、`isConnected`、`disconnect`、`close`）、`BrowserContext`
- **页面与 Frame**：`goto`、`goBack`、`goForward`、`reload`、`content`、`setContent`、`title`、`url`、`evaluate`、`setViewport`、`screenshot`、`pdf`、`click`、`typeText`、`focus`、`hover`、`tap`、`waitForSelector`、`querySelector`、`querySelectorAll`、`setExtraHttpHeaders`、`setRequestInterception`、`cookies`、`setCookie`、`deleteCookie`、`accessibilitySnapshot`、`createCDPSession`、`close`
- **输入与句柄**：`Keyboard`、`Mouse`、`Touchscreen`、`ElementHandle`、`JSHandle`
- **网络与弹窗**：`HTTPRequest`、`HTTPResponse`、`Dialog`、`CDPSession`

## 最小示例

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

## CP 内存安全与所有权特性

- **零重复底座**：直接复用 `cap:cdp` 的 SIMD 帧扫描与会话路由，无冗余协议实现。
- **确定性 RAII 释放**：`Browser`、`Page` 与 `JSHandle` 均提供 `~init()` 与幂等 `deinit()`，重复释放安全无泄漏。
- **调用方缓冲优先**：`evaluate`、`screenshot`、`pdf`、`title` 等接口直接写入调用方提供的 `[]u8` 切片。
- **显式错误与可变借用**：可变方法使用 `* _` 接收者，统一返回 `T!PuppeteerError` 或 `T?`。
