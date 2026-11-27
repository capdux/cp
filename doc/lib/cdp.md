# `cap:cdp` — Chrome DevTools Protocol & Browser Automation Core

> **模块路径**: `lib/cdp/` (`cap:cdp`)  
> **定位**: `puppeteer` 与 `playwright` 共享的零拷贝原生 CDP 协议栈与浏览器生命周期核心底座。

## 模块结构

| 文件 | 核心导出 | 职责 |
| --- | --- | --- |
| `protocol.cp` | `CdpError`, `CdpMessageKind`, `CdpRequest`, `CdpResponse`, `CdpEvent`, `CdpMessage`, `CdpFrameScanner` | 零分配 CDP JSON-RPC 帧扫描与请求/响应/事件解析 |
| `session.cp` | `CdpConnectionState`, `CdpSessionInfo`, `CdpPendingCall`, `CdpEventListener`, `CdpSession`, `CdpEndpoint` | 多 Target / SessionId 复用、请求回调挂起匹配与事件订阅 |
| `launcher.cp` | `CdpBrowserLaunchOptions`, `CdpWsEndpoint`, `CdpBrowserLauncher` | 跨平台 Chromium/Chrome/Edge/Capsurf 二进制自动探测与启动参数构造 |
| `domains.cp` | `PageDomain`, `RuntimeDomain`, `DomDomain`, `InputDomain`, `NetworkDomain`, `AccessibilityDomain` | 常用 CDP 域（Page/Runtime/DOM/Input/Network/Accessibility）参数与指令构造 |
| `index.cp` | `cdp` 命名空间及全量导出 | 统一对外入口 |

## 快速用法

```typescript
import { CdpBrowserLauncher, CdpEndpoint, PageDomain, RuntimeDomain } from "cap:cdp";

let found = CdpBrowserLauncher.detectBrowserBinary();
let ep = CdpEndpoint.connect("ws://127.0.0.1:9222/devtools/browser/1");
let page = ep.createSession("target-1", "session-1");
```
