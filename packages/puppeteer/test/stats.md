# Puppeteer CP 测试集吸收与真机 Chromium CDP 测试报告 (`packages/puppeteer/test/stats.md`)

> 测试时间：2026-11-28 | 测试环境：CentOS Stream 10 (Coughlan) `x86_64` | Chromium：`/usr/bin/chromium-browser` (`158.0.8127.88`)

---

## 1. 上游测试全量吸收统计 (`_ref/puppeteer/test` → `packages/puppeteer/test/`)

- **执行指令**：`cap from -ts _ref/puppeteer/test -o packages/puppeteer/test --test`
- **迁移总览**（详见 [migrate.md](./migrate.md)）：

| 指标 | 数值 |
| :--- | :--- |
| 源目录 | `_ref/puppeteer/test` |
| 目标目录 | `packages/puppeteer/test` |
| 迁移文件总数 | **131** |
| 代码行数变化 | `38,813` 行 → `35,666` 行 |
| `cpc` 语法解析通过率 | **131 / 131 (100.0%)** |
| `cap fix` 自动修正导入 | **7 处**（覆盖 6 个文件，含 `puppeteer-core` → `puppeteer` 等） |
| 剩余 `TODO(cap-migrate)` 标记 | `913`（主要为 Node test runner fixture / HTTP TestServer 外部环境绑定） |

---

## 2. 本机真实 Chromium 158.0.8127.88 端到端 CDP 连接实测

通过 [real_chromium_smoke.cp](./real_chromium_smoke.cp) 直接在 CentOS Stream 10 上拉起真实无头 `/usr/bin/chromium-browser` (`--headless=new --no-sandbox --disable-dev-shm-usage --remote-allow-origins=* --remote-debugging-port=9225`)，经由纯 CP RFC 6455 WebSocket 客户端对接 `Puppeteer.connectWithTransport` + `cap:cdp` 完成真实端到端测试：

| 测试步骤 | 验证内容 | 实测结果 |
| :--- | :--- | :--- |
| **1. 可执行路径探测** | `Puppeteer.defaultExecutablePath(alloc)` | ✔ PASS → `/usr/bin/chromium-browser` |
| **2. CDP HTTP 元数据** | `GET /json/version` (`127.0.0.1:9225`) | ✔ PASS → `Browser=Chrome/158.0.8127.88`, `Protocol=1.3`, `WS=ws://127.0.0.1:9225/devtools/browser/...` |
| **3. WebSocket 握手与连接** | RFC 6455 `101` 握手 + `Puppeteer.connectWithTransport` | ✔ PASS |
| **4. 协议版本查询** | `browser.version(&ver_buf)` (`Browser.getVersion`) | ✔ PASS → `Chrome/158.0.8127.88` |
| **5. 创建页面并执行 JS** | `browser.newPage()` → `page.goto("data:text/html,<title>CP-Real-Chromium-158</title><h1>OK</h1>")` → `page.evaluate("document.title")` / `page.evaluate("19 + 23")` | ✔ PASS → `type=string, json=CP-Real-Chromium-158`；`type=number, json=42` |
| **6. 生命周期与零泄漏** | `page.close()` + `browser.close()` + `DebugAllocator.deinit() == .ok` | ✔ PASS（exit code `0`，编译链接耗时 `691ms`，零内存泄漏） |

### 原始实测输出摘录
```text
Compiling: packages/puppeteer/test/real_chromium_smoke.cp (Mode: Debug, modular + parallel-obj)...
Compiled and linked in 691ms
Executing: bin/packages/puppeteer/test/real_chromium_smoke...
[1] Probed browser executable: /usr/bin/chromium-browser
[2] Live CDP /json/version: Browser=Chrome/158.0.8127.88, Protocol=1.3, WS=ws://127.0.0.1:9225/devtools/browser/a2397bcc-7088-4205-b030-d13f26764ea6
[3] CDP Browser.getVersion() -> Chrome/158.0.8127.88
[4] CDP Runtime.evaluate(document.title) -> type=string, json=CP-Real-Chromium-158
[5] CDP Runtime.evaluate(19 + 23) -> type=number, json=42
ALL REAL CHROMIUM PUPPETEER CDP CHECKS PASSED!
```
