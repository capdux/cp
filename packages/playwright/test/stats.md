# Playwright 测试迁移与真机 Chromium CDP 验证统计报告

## 1. 系统与真机环境

| 项目 | 实测值 |
| --- | --- |
| 操作系统 | CentOS Stream 10 (Coughlan) |
| 内核与架构 | `Linux beam-01 6.12.0-284.el10.x86_64 #1 SMP PREEMPT_DYNAMIC x86_64 GNU/Linux` |
| Zig 工具链 | `0.17.0` |
| 本机 Chromium 路径 | `/usr/bin/chromium-browser` -> `../../usr/lib64/chromium-browser/chromium-browser.sh` |
| 本机 Chromium 版本 | `Chromium 158.0.8127.88 Built from source for CentOS Stream release 10 (Coughlan)` |

---

## 2. 上游 Playwright 测试全量吸收统计 (`_ref/playwright/tests` -> `packages/playwright/test`)

执行命令：
```bash
cap from -ts _ref/playwright/tests -o packages/playwright/test --test
```

### 2.1 总体转译指标

| 指标 | 实测结果 |
| --- | --- |
| 源目录 | `_ref/playwright/tests` |
| 输出目录 | `packages/playwright/test` |
| 迁移 `.cp` 文件总数 | `691` |
| 代码行数变化（原 -> CP） | `327056` 行 -> `302780` 行 |
| `cpc` 语法解析通过率 | `691 / 691 (100.0%)` |
| `cap fix` 导入自动修复 | `65` 处修复（覆盖 `63` 个文件） |
| 迁移报告总标记数 (`migrate.md`) | `6868`（含工具链降级统计项） |
| 落地文件 `TODO(cap-migrate)` 实测统计 | `3546` 处匹配（分布在 `436` 个 `.cp` 文件，`3352` 行；含通用 `TODO` 共 `3946` 行） |
| 递归外部依赖数 | `0` |

### 2.2 按子目录分布（691 个转换文件）

| 子目录 | 文件数 | 说明 |
| --- | ---: | --- |
| `library/` | 168 | 核心 Browser / Context / Har / Download / Events / Chromium 能力测试 |
| `page/` | 138 | Page / Frame / Locator / Route / Dialog / Screenshot 等测试 |
| `playwright-test/` | 107 | `@playwright/test` Runner / Reporter / Config 测试 |
| `mcp/` | 106 | Playwright MCP CLI 与 Trace fixtures |
| `assets/` | 52 | 前端测试资源脚本（React / Vue / Modernizr / ServiceWorker / Axe-core） |
| `installation/` | 50 | 安装与跨包形态验证脚本 |
| `config/` | 22 | 测试基座与各 Reporter / Server Fixtures |
| `electron/` | 17 | Electron 集成测试 |
| `extension/` | 12 | 浏览器扩展测试 |
| `android/` | 7 | Android / WebView 测试 |
| `image_tools/` | 3 | 图像对比工具测试 |
| `stress/` | 3 | 压力测试静态资源 |
| `webview/` | 3 | WebView 配置测试 |
| `bidi/` | 2 | WebDriver BiDi 配置与 Reporter |
| `third_party/` | 1 | 第三方辅助脚本 |

### 2.3 `TODO(cap-migrate)` 主要降级原因分布

| 计数 | 主要分布类别 |
| ---: | --- |
| 919 | `dynamic TS metaprogramming not supported in CP`（动态 TS 元编程/反射） |
| 767+ | `replace try/catch with CP ?T / ?`（含各类 `catch (e)`/`catch (error)`/`catch (err)` 块） |
| 652+ | `refine TS body` / `refine TS statement` / `refine TS function` / `refine TS class member` |
| 290 | `replace finally with CP defer` |
| 95 | `complex TS conditional/mapped type` |
| 49+ | `throw -> error return` |

> 同步报告已由 `cap from` 自动生成于 [`packages/playwright/test/migrate.md`](/data/dev/cp/packages/playwright/test/migrate.md)。

---

## 3. 真机 Chromium 158.0.8127.88 CDP 端到端连接实测

测试文件：[`packages/playwright/test/test_real_chromium_cdp.cp`](/data/dev/cp/packages/playwright/test/test_real_chromium_cdp.cp)

### 3.1 启动与编译环境备注

- 真机启动命令：
  ```bash
  chromium-browser --headless=new --no-sandbox --disable-dev-shm-usage --remote-debugging-port=9224 --user-data-dir=/tmp/pw-real-cdp
  ```
- 浏览器就绪输出：
  `DevTools listening on ws://127.0.0.1:9224/devtools/browser/27240c2e-32df-456d-b277-08e52b9a167d`
- 编译与运行链路：
  - `cap build packages/playwright/test/test_real_chromium_cdp.cp -o temp/test_real_chromium_cdp`（`cpc` 转译 `46ms` + Zig 编译 `122ms`，总计 `172ms`）
  - 链接 `-lc` 生成本地原生二进制（注：`cap test` 当前因 `cap:test` 内部依赖 `lib/path` 存在重复符号 `EnvPathIterator` 未直接走通，改用 `cap build` 转译后 `zig build-exe ... -lc` 直连本地运行，全流程 100% 真实 TCP/HTTP/WebSocket 执行）。
- 总执行耗时：**`64 ms`**（进程退出码 `0`）。

### 3.2 逐步实测结果与真实返回数据

| 步骤 | 测试项 / CDP 方法 | 状态 | 实测返回数据 |
| --- | --- | --- | --- |
| 1 | `CdpBrowserLauncher.probeBrowserExecutable(alloc, "")` | ✅ PASS | `/usr/bin/chromium-browser` |
| 2 | `chromium.executablePath(alloc, "")` | ✅ PASS | `/usr/bin/chromium-browser` |
| 3 | HTTP `GET 127.0.0.1:9224/json/version` | ✅ PASS | `Browser=Chrome/158.0.8127.88`, `Protocol-Version=1.3`, `V8-Version=15.8.62.31`, `webSocketDebuggerUrl=ws://127.0.0.1:9224/devtools/browser/27240c2e-32df-456d-b277-08e52b9a167d` |
| 4 | HTTP `GET 127.0.0.1:9224/json/list` | ✅ PASS | `1523` bytes（含有效 `webSocketDebuggerUrl` JSON 数组） |
| 5 | RFC 6455 WebSocket 握手 (`GET /devtools/browser/...`) | ✅ PASS | `HTTP/1.1 101 Switching Protocols`, `Sec-WebSocket-Accept=s3pPLMBiTxaQ9kYGzzhZRbK+xOo=` |
| 6 | `chromium.connectOverCDP` + `Browser.getVersion` | ✅ PASS | `product=Chrome/158.0.8127.88`, `jsVersion=15.8.62.31` |
| 7 | `Target.createTarget` (`about:blank`) | ✅ PASS | `targetId=E1006B63C04DCB93706E576AB9C7503A` |
| 8 | `Target.attachToTarget` (`flatten: true`) | ✅ PASS | `sessionId=8EE23B8491E2A9C74C39F28F5D690FC8` |
| 9 | `Page.enable` + `Page.navigate` (`data:text/html,...`) | ✅ PASS | `frameId=E1006B63C04DCB93706E576AB9C7503A` |
| 10 | `Runtime.evaluate` (`document.title`, `returnByValue: true`) | ✅ PASS | `{"result":{"type":"string","value":"CP Playwright Chromium 158"}}` |
| 11 | `Target.closeTarget` | ✅ PASS | `{"success":true}` |
| 12 | `Browser.close` + WebSocket Close Frame (`1000`) | ✅ PASS | `{}` |
| 13 | 临时进程与数据目录清理 | ✅ PASS | 已终止 `chromium-browser` 进程并清理 `/tmp/pw-real-cdp` |
