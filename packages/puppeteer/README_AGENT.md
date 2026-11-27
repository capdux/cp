[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# puppeteer — AI Agent Quick Ref

High-level browser automation API for CP built on `cap:cdp` (`v25.19.0`, `Apache-2.0`).

## Import

```cp
import {
    Puppeteer, Browser, BrowserContext, Page, Frame, ElementHandle,
    JSHandle, CDPSession, Keyboard, Mouse, Touchscreen,
    HTTPRequest, HTTPResponse, Dialog,
    LaunchOptions, ConnectOptions, GoToOptions, Viewport,
    ScreenshotOptions, PdfOptions, ClickOptions, TypeOptions,
    AccessibilitySnapshotOptions, AccessibilitySnapshot,
    launch, launchWithTransport, connect, connectWithTransport, defaultExecutablePath,
} from "puppeteer";
```

## Core API Signatures

```cp
fn launch(alloc: std.mem.Allocator, options: LaunchOptions): Browser!PuppeteerError;
fn launchWithTransport(alloc: std.mem.Allocator, options: LaunchOptions, ws_url: []const u8, transport_fn: CdpSendTransportFn, transport_ctx: usize): Browser!PuppeteerError;
fn connect(alloc: std.mem.Allocator, browser_ws_url: []const u8): Browser!PuppeteerError;
fn connectWithTransport(alloc: std.mem.Allocator, options: ConnectOptions, transport_fn: CdpSendTransportFn, transport_ctx: usize): Browser!PuppeteerError;
fn defaultExecutablePath(alloc: std.mem.Allocator): ([]u8)?;

// Browser
browser.newPage(): (*Page)!PuppeteerError;
browser.pages(): []*Page;
browser.createBrowserContext(): BrowserContext!PuppeteerError;
browser.wsEndpoint(): []const u8;
browser.version(out_buf: []u8): ([]const u8)!PuppeteerError;
browser.userAgent(out_buf: []u8): ([]const u8)!PuppeteerError;
browser.isConnected(): bool;
browser.close(): void!PuppeteerError;

// Page
page.goto(target_url: []const u8, options: GoToOptions): void!PuppeteerError;
page.evaluate(expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError;
page.setViewport(vp: Viewport): void!PuppeteerError;
page.screenshot(options: ScreenshotOptions, out_buf: []u8): ([]const u8)!PuppeteerError;
page.pdf(options: PdfOptions, out_buf: []u8): ([]const u8)!PuppeteerError;
page.click(selector: []const u8, options: ClickOptions): void!PuppeteerError;
page.typeText(selector: []const u8, text: []const u8, options: TypeOptions): void!PuppeteerError;
page.querySelector(selector: []const u8): ElementHandle?!PuppeteerError;
page.querySelectorAll(selector: []const u8, out_handles: []ElementHandle): usize!PuppeteerError;
page.accessibilitySnapshot(options: AccessibilitySnapshotOptions): AccessibilitySnapshot!PuppeteerError;
page.createCDPSession(child_session_id: []const u8): CDPSession!PuppeteerError;
page.close(): void!PuppeteerError;
```

## Minimal Usage

```cp
const std = @import("std");
import { launchWithTransport, LaunchOptions, GoToOptions } from "puppeteer";

pub fn run(alloc: std.mem.Allocator, tx: CdpSendTransportFn, ctx: usize): !void {
    let browser = launchWithTransport(alloc, LaunchOptions{}, "ws://127.0.0.1:9222/devtools/browser/1", tx, ctx)?;
    defer browser.deinit();
    const page = browser.newPage()?;
    page.goto("https://example.com", GoToOptions{})?;
    let buf: [256]u8 = undefined;
    const eval_res = page.evaluate("1 + 1", &buf)?;
    * = eval_res;
}
```

## Borrowing, Errors & Pitfalls
- **RAII**: Always `defer browser.deinit();` or call `browser.close()`; `deinit()` is idempotent.
- **No `err` in catch**: Use `catch |e|` instead of `catch |err|`; discard unused values with `const _v = expr; * = _v;`.
- **Self-pointers**: Call `page.bindSelf()` if a `Page` struct is moved in memory; `browser.newPage()` binds automatically.
