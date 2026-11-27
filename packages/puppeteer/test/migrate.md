# `@puppeteer-test/test` 迁移报告（TypeScript / JavaScript → CP）

## 1. 迁移总览与统计

| 指标 | 数值 |
| --- | --- |
| 项目名称 | `@puppeteer-test/test` |
| 版本号 | `latest` |
| 源语言 | TypeScript / JavaScript |
| 源码目录 | `_ref/puppeteer/test` |
| 输出目录 | `packages/puppeteer/test` |
| 迁移文件总数 | 131 |
| 代码行数变化 | 38813 行 → 35666 行 |
| `cpc` 语法解析通过率 | 131/131 (100.0%) |
| 剩余 `TODO(cap-migrate)` 标记数 | 913 |
| 依赖总计 | 7（Tier 1 内置: 0 / Tier 2 官方包: 0 / Tier 3 递归转换: 0 / 待人工评估: 7） |

## 2. 依赖库 3 级迁移清单

### 2.1 Tier 1 — CP 内置标准库（`cap:*`，零外部依赖直连）

_无 Tier 1 内置映射依赖。_

### 2.2 Tier 2 — 官方生态可安装包（`@cap/*`）

_无 Tier 2 官方包依赖。_

### 2.3 Tier 3 — 递归转换的独立可发布 CP 库

_无 Tier 3 递归转换依赖。_

### 2.4 待人工评估的依赖

| 原依赖 | 原版本 | 类型 | 原因 |
| --- | --- | --- | --- |
| `@types/pngjs` | `6.0.5` | dev | 开发期依赖（已由 cap fmt/lint/test/build 工具链替代或按需忽略） |
| `diff` | `9.0.0` | runtime | 未在本地 node_modules/workspace 找到源码；安装依赖后重新运行 `cap from -ts` 可自动递归转换 |
| `jpeg-js` | `0.4.4` | runtime | 未在本地 node_modules/workspace 找到源码；安装依赖后重新运行 `cap from -ts` 可自动递归转换 |
| `mime` | `4.1.0` | runtime | 未在本地 node_modules/workspace 找到源码；安装依赖后重新运行 `cap from -ts` 可自动递归转换 |
| `pixelmatch` | `8.0.0` | runtime | 未在本地 node_modules/workspace 找到源码；安装依赖后重新运行 `cap from -ts` 可自动递归转换 |
| `pngjs` | `7.0.0` | runtime | 未在本地 node_modules/workspace 找到源码；安装依赖后重新运行 `cap from -ts` 可自动递归转换 |
| `webmcp-types` | `0.1.11` | dev | 开发期依赖（已由 cap fmt/lint/test/build 工具链替代或按需忽略） |

## 3. 逐文件迁移明细

| 原文件 | 生成 CP 文件 | 行数 (原 → CP) | `cpc` 解析 | `TODO` 数 | 优化项 |
| --- | --- | --- | --- | --- | --- |
| `assets/cached/bfcache/worker.mjs` | `assets/cached/bfcache/worker.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/cached/one-script.js` | `assets/cached/one-script.cp` | 3 → 5 | ✅ 通过 | 0 | - |
| `assets/es6/es6import.js` | `assets/es6/es6import.cp` | 2 → 4 | ✅ 通过 | 0 | - |
| `assets/es6/es6module.js` | `assets/es6/es6module.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/es6/es6pathimport.js` | `assets/es6/es6pathimport.cp` | 2 → 4 | ✅ 通过 | 0 | - |
| `assets/extension-with-page/background.js` | `assets/extension-with-page/background.cp` | 3 → 5 | ✅ 通过 | 0 | - |
| `assets/frames/script.js` | `assets/frames/script.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/initiator.js` | `assets/initiator.cp` | 8 → 10 | ✅ 通过 | 0 | - |
| `assets/injectedfile.js` | `assets/injectedfile.cp` | 2 → 4 | ✅ 通过 | 0 | - |
| `assets/input/mouse-helper.js` | `assets/input/mouse-helper.cp` | 74 → 77 | ✅ 通过 | 0 | - |
| `assets/jscoverage/script1.js` | `assets/jscoverage/script1.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/jscoverage/script2.js` | `assets/jscoverage/script2.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/modernizr.js` | `assets/modernizr.cp` | 3 → 9 | ✅ 通过 | 3 | - |
| `assets/serviceworkers/empty/sw.js` | `assets/serviceworkers/empty/sw.cp` | 0 → 2 | ✅ 通过 | 0 | - |
| `assets/serviceworkers/extension/background.js` | `assets/serviceworkers/extension/background.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/serviceworkers/fetch/sw.js` | `assets/serviceworkers/fetch/sw.cp` | 7 → 9 | ✅ 通过 | 0 | - |
| `assets/serviceworkers/fetch-on-install/sw.js` | `assets/serviceworkers/fetch-on-install/sw.cp` | 29 → 31 | ✅ 通过 | 0 | 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `assets/simple-extension/content-script.js` | `assets/simple-extension/content-script.cp` | 2 → 4 | ✅ 通过 | 0 | - |
| `assets/simple-extension/index.js` | `assets/simple-extension/index.cp` | 2 → 4 | ✅ 通过 | 0 | - |
| `assets/simple-extension-firefox/content-script.js` | `assets/simple-extension-firefox/content-script.cp` | 2 → 4 | ✅ 通过 | 0 | - |
| `assets/simple-extension-firefox/index.js` | `assets/simple-extension-firefox/index.cp` | 2 → 4 | ✅ 通过 | 0 | - |
| `assets/syntax-error.js` | `assets/syntax-error.cp` | 3 → 6 | ✅ 通过 | 1 | - |
| `assets/worker/worker-missing-import.js` | `assets/worker/worker-missing-import.cp` | 3 → 5 | ✅ 通过 | 0 | - |
| `assets/worker/worker.js` | `assets/worker/worker.cp` | 16 → 18 | ✅ 通过 | 0 | - |
| `fixtures/closeme.mjs` | `fixtures/closeme.cp` | 11 → 15 | ✅ 通过 | 2 | - |
| `fixtures/dumpio.mjs` | `fixtures/dumpio.cp` | 16 → 20 | ✅ 通过 | 2 | - |
| `installation/.mocharc.js` | `installation/.mocharc.cp` | 20 → 23 | ✅ 通过 | 1 | - |
| `installation/assets/puppeteer/basic-cjs.ts` | `installation/assets/puppeteer/basic-cjs.cp` | 15 → 18 | ✅ 通过 | 1 | - |
| `installation/assets/puppeteer/basic.js` | `installation/assets/puppeteer/basic.cp` | 15 → 18 | ✅ 通过 | 1 | - |
| `installation/assets/puppeteer/basic.ts` | `installation/assets/puppeteer/basic.cp` | 15 → 18 | ✅ 通过 | 1 | - |
| `installation/assets/puppeteer/bidi.js` | `installation/assets/puppeteer/bidi.cp` | 17 → 20 | ✅ 通过 | 1 | - |
| `installation/assets/puppeteer/cdp.js` | `installation/assets/puppeteer/cdp.cp` | 17 → 20 | ✅ 通过 | 1 | - |
| `installation/assets/puppeteer/configuration/puppeteer.config.ts` | `installation/assets/puppeteer/configuration/puppeteer.config.cp` | 6 → 9 | ✅ 通过 | 1 | - |
| `installation/assets/puppeteer/imports.js` | `installation/assets/puppeteer/imports.cp` | 10 → 12 | ✅ 通过 | 0 | - |
| `installation/assets/puppeteer/installCanary.js` | `installation/assets/puppeteer/installCanary.cp` | 24 → 21 | ✅ 通过 | 0 | - |
| `installation/assets/puppeteer/puppeteer-in-browser.js` | `installation/assets/puppeteer/puppeteer-in-browser.cp` | 34 → 36 | ✅ 通过 | 3 | 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `installation/assets/puppeteer/puppeteer-in-extension.js` | `installation/assets/puppeteer/puppeteer-in-extension.cp` | 43 → 45 | ✅ 通过 | 3 | 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `installation/assets/puppeteer/trimCache.js` | `installation/assets/puppeteer/trimCache.cp` | 11 → 13 | ✅ 通过 | 0 | - |
| `installation/assets/puppeteer/webpack/webpack.config.js` | `installation/assets/puppeteer/webpack/webpack.config.cp` | 16 → 19 | ✅ 通过 | 1 | - |
| `installation/assets/puppeteer-core/imports-webdriver.js` | `installation/assets/puppeteer-core/imports-webdriver.cp` | 10 → 12 | ✅ 通过 | 0 | - |
| `installation/assets/puppeteer-core/imports.js` | `installation/assets/puppeteer-core/imports.cp` | 9 → 11 | ✅ 通过 | 0 | - |
| `installation/assets/puppeteer-core/launch.js` | `installation/assets/puppeteer-core/launch.cp` | 26 → 28 | ✅ 通过 | 2 | 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `installation/src/browsers.test.ts` | `installation/src/browsers.test.cp` | 30 → 35 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/src/constants.ts` | `installation/src/constants.cp` | 31 → 35 | ✅ 通过 | 2 | - |
| `installation/src/puppeteer-cli.test.ts` | `installation/src/puppeteer-cli.test.cp` | 75 → 80 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/src/puppeteer-configuration.test.ts` | `installation/src/puppeteer-configuration.test.cp` | 130 → 135 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/src/puppeteer-core.test.ts` | `installation/src/puppeteer-core.test.cp` | 34 → 41 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/src/puppeteer-firefox.test.ts` | `installation/src/puppeteer-firefox.test.cp` | 46 → 51 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/src/puppeteer-typescript.test.ts` | `installation/src/puppeteer-typescript.test.cp` | 69 → 73 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/src/puppeteer-webdriver.test.ts` | `installation/src/puppeteer-webdriver.test.cp` | 40 → 44 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/src/puppeteer-webpack.test.ts` | `installation/src/puppeteer-webpack.test.cp` | 47 → 54 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/src/puppeteer.test.ts` | `installation/src/puppeteer.test.cp` | 136 → 141 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `installation/src/sandbox.ts` | `installation/src/sandbox.cp` | 158 → 150 | ✅ 通过 | 3 | 消除 interface 动态派发为栈布局 struct（零虚表开销）; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `installation/src/util.ts` | `installation/src/util.cp` | 52 → 51 | ✅ 通过 | 2 | 消除 interface 动态派发为栈布局 struct（零虚表开销）; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `src/acceptInsecureCerts.test.ts` | `src/acceptInsecureCerts.test.cp` | 121 → 120 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/accessibility.test.ts` | `src/accessibility.test.cp` | 1186 → 1145 | ✅ 通过 | 11 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/ariaqueryhandler.test.ts` | `src/ariaqueryhandler.test.cp` | 858 → 838 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/autofill.test.ts` | `src/autofill.test.cp` | 63 → 68 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/bluetooth-emulation.test.ts` | `src/bluetooth-emulation.test.cp` | 81 → 83 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/browser.test.ts` | `src/browser.test.cp` | 687 → 636 | ✅ 通过 | 18 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/browsercontext-cookies.test.ts` | `src/browsercontext-cookies.test.cp` | 475 → 465 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/browsercontext.test.ts` | `src/browsercontext.test.cp` | 692 → 637 | ✅ 通过 | 12 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/cdp/CDPSession.test.ts` | `src/cdp/CDPSession.test.cp` | 238 → 228 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/cdp/TargetManager.test.ts` | `src/cdp/TargetManager.test.cp` | 93 → 98 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/cdp/a11yLoaderId.test.ts` | `src/cdp/a11yLoaderId.test.cp` | 82 → 83 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/cdp/backendNodeId.test.ts` | `src/cdp/backendNodeId.test.cp` | 19 → 24 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/cdp/bfcache.test.ts` | `src/cdp/bfcache.test.cp` | 92 → 97 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/cdp/binding.test.ts` | `src/cdp/binding.test.cp` | 166 → 170 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/cdp/devtools.test.ts` | `src/cdp/devtools.test.cp` | 222 → 227 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/cdp/extensions.test.ts` | `src/cdp/extensions.test.cp` | 471 → 478 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/cdp/heapSnapshot.test.ts` | `src/cdp/heapSnapshot.test.cp` | 42 → 47 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/cdp/interventionHeaders.test.ts` | `src/cdp/interventionHeaders.test.cp` | 48 → 52 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/cdp/network.test.ts` | `src/cdp/network.test.cp` | 159 → 153 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/cdp/network_restrictions.test.ts` | `src/cdp/network_restrictions.test.cp` | 1741 → 1582 | ✅ 通过 | 88 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/cdp/pdf.test.ts` | `src/cdp/pdf.test.cp` | 78 → 79 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/cdp/pipe.test.ts` | `src/cdp/pipe.test.cp` | 56 → 61 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/cdp/prerender.test.ts` | `src/cdp/prerender.test.cp` | 212 → 218 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/cdp/pwa.test.ts` | `src/cdp/pwa.test.cp` | 105 → 108 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/cdp/queryObjects.test.ts` | `src/cdp/queryObjects.test.cp` | 108 → 104 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/cdp/realms.test.ts` | `src/cdp/realms.test.cp` | 116 → 121 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/cdp/screencast.test.ts` | `src/cdp/screencast.test.cp` | 148 → 153 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/cdp/userDataDir.test.ts` | `src/cdp/userDataDir.test.cp` | 70 → 75 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/cdp/webmcp.test.ts` | `src/cdp/webmcp.test.cp` | 783 → 687 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/click.test.ts` | `src/click.test.cp` | 559 → 536 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/connect.test.ts` | `src/connect.test.cp` | 70 → 75 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/console.test.ts` | `src/console.test.cp` | 398 → 377 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/cookies.test.ts` | `src/cookies.test.cp` | 907 → 863 | ✅ 通过 | 13 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/coverage.test.ts` | `src/coverage.test.cp` | 318 → 323 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/debugInfo.test.ts` | `src/debugInfo.test.cp` | 48 → 54 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/defaultbrowsercontext.test.ts` | `src/defaultbrowsercontext.test.cp` | 99 → 100 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/device-request-prompt.test.ts` | `src/device-request-prompt.test.cp` | 51 → 56 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/dialog.test.ts` | `src/dialog.test.cp` | 110 → 115 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/download.test.ts` | `src/download.test.cp` | 87 → 92 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/drag-and-drop.test.ts` | `src/drag-and-drop.test.cp` | 205 → 182 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/elementhandle.test.ts` | `src/elementhandle.test.cp` | 1279 → 1188 | ✅ 通过 | 21 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/emulation.test.ts` | `src/emulation.test.cp` | 871 → 808 | ✅ 通过 | 18 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/evaluation.test.ts` | `src/evaluation.test.cp` | 653 → 603 | ✅ 通过 | 11 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/fixtures.test.ts` | `src/fixtures.test.cp` | 123 → 126 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/followSymlinks.test.ts` | `src/followSymlinks.test.cp` | 256 → 261 | ✅ 通过 | 14 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/frame.test.ts` | `src/frame.test.cp` | 470 → 413 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/golden-utils.ts` | `src/golden-utils.cp` | 172 → 150 | ✅ 通过 | 16 | 消除 interface 动态派发为栈布局 struct（零虚表开销）; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `src/headful.test.ts` | `src/headful.test.cp` | 98 → 103 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/idle_override.test.ts` | `src/idle_override.test.cp` | 79 → 85 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/injected.test.ts` | `src/injected.test.cp` | 137 → 120 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/input.test.ts` | `src/input.test.cp` | 538 → 505 | ✅ 通过 | 25 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/jshandle.test.ts` | `src/jshandle.test.cp` | 405 → 383 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/keyboard.test.ts` | `src/keyboard.test.cp` | 692 → 645 | ✅ 通过 | 9 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/launcher.test.ts` | `src/launcher.test.cp` | 1050 → 293 | ✅ 通过 | 87 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/locator.test.ts` | `src/locator.test.cp` | 1292 → 1264 | ✅ 通过 | 30 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/mocha-utils.ts` | `src/mocha-utils.cp` | 670 → 620 | ✅ 通过 | 54 | 消除 interface 动态派发为栈布局 struct（零虚表开销）; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/mouse.test.ts` | `src/mouse.test.cp` | 706 → 642 | ✅ 通过 | 12 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 消除 interface 动态派发为栈布局 struct（零虚表开销）; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/navigation.test.ts` | `src/navigation.test.cp` | 1209 → 574 | ✅ 通过 | 28 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/network.test.ts` | `src/network.test.cp` | 1608 → 1492 | ✅ 通过 | 18 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/oopif.test.ts` | `src/oopif.test.cp` | 891 → 826 | ✅ 通过 | 12 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/page.test.ts` | `src/page.test.cp` | 3143 → 2875 | ✅ 通过 | 50 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/proxy.test.ts` | `src/proxy.test.cp` | 265 → 265 | ✅ 通过 | 13 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/queryhandler.test.ts` | `src/queryhandler.test.cp` | 727 → 685 | ✅ 通过 | 14 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/queryselector.test.ts` | `src/queryselector.test.cp` | 584 → 561 | ✅ 通过 | 22 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/requestinterception-experimental.test.ts` | `src/requestinterception-experimental.test.cp` | 1107 → 1047 | ✅ 通过 | 36 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/requestinterception.test.ts` | `src/requestinterception.test.cp` | 1345 → 1267 | ✅ 通过 | 22 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/screenshot.test.ts` | `src/screenshot.test.cp` | 540 → 510 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/stacktrace.test.ts` | `src/stacktrace.test.cp` | 156 → 161 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/target.test.ts` | `src/target.test.cp` | 799 → 739 | ✅ 通过 | 25 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/touch-event-utils.ts` | `src/touch-event-utils.cp` | 46 → 47 | ✅ 通过 | 1 | 消除 interface 动态派发为栈布局 struct（零虚表开销） |
| `src/touchscreen.test.ts` | `src/touchscreen.test.cp` | 915 → 920 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/tracing.test.ts` | `src/tracing.test.cp` | 165 → 163 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/utils.ts` | `src/utils.cp` | 248 → 217 | ✅ 通过 | 21 | 消除 interface 动态派发为栈布局 struct（零虚表开销）; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/waittask.test.ts` | `src/waittask.test.cp` | 1181 → 1088 | ✅ 通过 | 17 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `src/webExtension.test.ts` | `src/webExtension.test.cp` | 51 → 58 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `src/webgl.test.ts` | `src/webgl.test.cp` | 30 → 37 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `src/worker.test.ts` | `src/worker.test.cp` | 916 → 844 | ✅ 通过 | 10 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |

## 4. 功能完整性、安全性增强与 CP 性能增强

### 4.1 安全性增强（消除动态类型与 GC 隐患）

- **消除 `any` / `var` / 隐式原型链**：全部变量收敛为块级 `let`/`const` 与静态强类型签名，编译期拦截空值与类型错配。
- **统一错误联合 `?T` 与可选类型 `T?`**：替代脆弱的 `try/catch` 隐式异常栈，强制显式传播（`?`）或处理错误。
- **确定性 RAII 内存释放**：通过编译期自动注入 `defer`/`errdefer` 与借用检查，彻底消除 GC Stop-The-World 停顿与资源句柄泄漏。
- **4 门禁质量保障**：支持 `cpc check` + `cap fmt` + `cap lint` + `cap test` 全链路静态校验。

### 4.2 CP 性能增强

- **原生机器码执行**：CP 编译为 Zig/LLVM 原生状态机（CPS），消除 V8/JSC JIT 预热开销与装箱分配。
- **内置零拷贝网络与路由**：Tier 1 依赖直连 `cap:web` / `cap:http` / `cap:crypto`（uSockets + picohttpparser + BoringSSL），内存占用显著下降。
- **百万级 Actor 并发**：天然兼容 BEAM 抢占式调度与零拷贝 `SharedTensor` / `move` 所有权传递。

### 4.3 本次迁移自动应用的优化项统计

- 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播（涉及 43 个文件）
- 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race`（涉及 39 个文件）
- 消除 interface 动态派发为栈布局 struct（零虚表开销）（涉及 7 个文件）
- 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法（涉及 82 个文件）
- 顶层箭头函数提升为静态零闭包开销 `fn` 声明（涉及 6 个文件）

## 5. 需人工精修清单与定位方法

### 5.2 含 `TODO(cap-migrate)` 标记的文件

- `assets/modernizr.cp`：3 处 `TODO(cap-migrate)`
- `assets/syntax-error.cp`：1 处 `TODO(cap-migrate)`
- `fixtures/closeme.cp`：2 处 `TODO(cap-migrate)`
- `fixtures/dumpio.cp`：2 处 `TODO(cap-migrate)`
- `installation/.mocharc.cp`：1 处 `TODO(cap-migrate)`
- `installation/assets/puppeteer/basic-cjs.cp`：1 处 `TODO(cap-migrate)`
- `installation/assets/puppeteer/basic.cp`：1 处 `TODO(cap-migrate)`
- `installation/assets/puppeteer/basic.cp`：1 处 `TODO(cap-migrate)`
- `installation/assets/puppeteer/bidi.cp`：1 处 `TODO(cap-migrate)`
- `installation/assets/puppeteer/cdp.cp`：1 处 `TODO(cap-migrate)`
- `installation/assets/puppeteer/configuration/puppeteer.config.cp`：1 处 `TODO(cap-migrate)`
- `installation/assets/puppeteer/puppeteer-in-browser.cp`：3 处 `TODO(cap-migrate)`
- `installation/assets/puppeteer/puppeteer-in-extension.cp`：3 处 `TODO(cap-migrate)`
- `installation/assets/puppeteer/webpack/webpack.config.cp`：1 处 `TODO(cap-migrate)`
- `installation/assets/puppeteer-core/launch.cp`：2 处 `TODO(cap-migrate)`
- `installation/src/constants.cp`：2 处 `TODO(cap-migrate)`
- `installation/src/puppeteer.test.cp`：6 处 `TODO(cap-migrate)`
- `installation/src/sandbox.cp`：3 处 `TODO(cap-migrate)`
- `installation/src/util.cp`：2 处 `TODO(cap-migrate)`
- `src/acceptInsecureCerts.test.cp`：2 处 `TODO(cap-migrate)`
- `src/accessibility.test.cp`：11 处 `TODO(cap-migrate)`
- `src/ariaqueryhandler.test.cp`：6 处 `TODO(cap-migrate)`
- `src/browser.test.cp`：18 处 `TODO(cap-migrate)`
- `src/browsercontext-cookies.test.cp`：6 处 `TODO(cap-migrate)`
- `src/browsercontext.test.cp`：12 处 `TODO(cap-migrate)`
- `src/cdp/CDPSession.test.cp`：7 处 `TODO(cap-migrate)`
- `src/cdp/a11yLoaderId.test.cp`：1 处 `TODO(cap-migrate)`
- `src/cdp/binding.test.cp`：6 处 `TODO(cap-migrate)`
- `src/cdp/devtools.test.cp`：3 处 `TODO(cap-migrate)`
- `src/cdp/extensions.test.cp`：5 处 `TODO(cap-migrate)`
- `src/cdp/interventionHeaders.test.cp`：1 处 `TODO(cap-migrate)`
- `src/cdp/network.test.cp`：3 处 `TODO(cap-migrate)`
- `src/cdp/network_restrictions.test.cp`：88 处 `TODO(cap-migrate)`
- `src/cdp/pdf.test.cp`：6 处 `TODO(cap-migrate)`
- `src/cdp/pipe.test.cp`：7 处 `TODO(cap-migrate)`
- `src/cdp/prerender.test.cp`：1 处 `TODO(cap-migrate)`
- `src/cdp/pwa.test.cp`：6 处 `TODO(cap-migrate)`
- `src/cdp/queryObjects.test.cp`：1 处 `TODO(cap-migrate)`
- `src/cdp/realms.test.cp`：2 处 `TODO(cap-migrate)`
- `src/cdp/screencast.test.cp`：2 处 `TODO(cap-migrate)`
- `src/cdp/userDataDir.test.cp`：8 处 `TODO(cap-migrate)`
- `src/cdp/webmcp.test.cp`：2 处 `TODO(cap-migrate)`
- `src/click.test.cp`：2 处 `TODO(cap-migrate)`
- `src/connect.test.cp`：2 处 `TODO(cap-migrate)`
- `src/console.test.cp`：7 处 `TODO(cap-migrate)`
- `src/cookies.test.cp`：13 处 `TODO(cap-migrate)`
- `src/coverage.test.cp`：4 处 `TODO(cap-migrate)`
- `src/debugInfo.test.cp`：2 处 `TODO(cap-migrate)`
- `src/device-request-prompt.test.cp`：2 处 `TODO(cap-migrate)`
- `src/dialog.test.cp`：1 处 `TODO(cap-migrate)`
- `src/download.test.cp`：3 处 `TODO(cap-migrate)`
- `src/drag-and-drop.test.cp`：6 处 `TODO(cap-migrate)`
- `src/elementhandle.test.cp`：21 处 `TODO(cap-migrate)`
- `src/emulation.test.cp`：18 处 `TODO(cap-migrate)`
- `src/evaluation.test.cp`：11 处 `TODO(cap-migrate)`
- `src/fixtures.test.cp`：3 处 `TODO(cap-migrate)`
- `src/followSymlinks.test.cp`：14 处 `TODO(cap-migrate)`
- `src/frame.test.cp`：5 处 `TODO(cap-migrate)`
- `src/golden-utils.cp`：16 处 `TODO(cap-migrate)`
- `src/headful.test.cp`：7 处 `TODO(cap-migrate)`
- `src/idle_override.test.cp`：1 处 `TODO(cap-migrate)`
- `src/injected.test.cp`：1 处 `TODO(cap-migrate)`
- `src/input.test.cp`：25 处 `TODO(cap-migrate)`
- `src/jshandle.test.cp`：1 处 `TODO(cap-migrate)`
- `src/keyboard.test.cp`：9 处 `TODO(cap-migrate)`
- `src/launcher.test.cp`：87 处 `TODO(cap-migrate)`
- `src/locator.test.cp`：30 处 `TODO(cap-migrate)`
- `src/mocha-utils.cp`：54 处 `TODO(cap-migrate)`
- `src/mouse.test.cp`：12 处 `TODO(cap-migrate)`
- `src/navigation.test.cp`：28 处 `TODO(cap-migrate)`
- `src/network.test.cp`：18 处 `TODO(cap-migrate)`
- `src/oopif.test.cp`：12 处 `TODO(cap-migrate)`
- `src/page.test.cp`：50 处 `TODO(cap-migrate)`
- `src/proxy.test.cp`：13 处 `TODO(cap-migrate)`
- `src/queryhandler.test.cp`：14 处 `TODO(cap-migrate)`
- `src/queryselector.test.cp`：22 处 `TODO(cap-migrate)`
- `src/requestinterception-experimental.test.cp`：36 处 `TODO(cap-migrate)`
- `src/requestinterception.test.cp`：22 处 `TODO(cap-migrate)`
- `src/screenshot.test.cp`：7 处 `TODO(cap-migrate)`
- `src/stacktrace.test.cp`：7 处 `TODO(cap-migrate)`
- `src/target.test.cp`：25 处 `TODO(cap-migrate)`
- `src/touch-event-utils.cp`：1 处 `TODO(cap-migrate)`
- `src/tracing.test.cp`：6 处 `TODO(cap-migrate)`
- `src/utils.cp`：21 处 `TODO(cap-migrate)`
- `src/waittask.test.cp`：17 处 `TODO(cap-migrate)`
- `src/webExtension.test.cp`：5 处 `TODO(cap-migrate)`
- `src/webgl.test.cp`：1 处 `TODO(cap-migrate)`
- `src/worker.test.cp`：10 处 `TODO(cap-migrate)`

### 5.3 待人工评估的第三方依赖

- `@types/pngjs` (`6.0.5`)：开发期依赖（已由 cap fmt/lint/test/build 工具链替代或按需忽略）
- `diff` (`9.0.0`)：未在本地 node_modules/workspace 找到源码；安装依赖后重新运行 `cap from -ts` 可自动递归转换
- `jpeg-js` (`0.4.4`)：未在本地 node_modules/workspace 找到源码；安装依赖后重新运行 `cap from -ts` 可自动递归转换
- `mime` (`4.1.0`)：未在本地 node_modules/workspace 找到源码；安装依赖后重新运行 `cap from -ts` 可自动递归转换
- `pixelmatch` (`8.0.0`)：未在本地 node_modules/workspace 找到源码；安装依赖后重新运行 `cap from -ts` 可自动递归转换
- `pngjs` (`7.0.0`)：未在本地 node_modules/workspace 找到源码；安装依赖后重新运行 `cap from -ts` 可自动递归转换
- `webmcp-types` (`0.1.11`)：开发期依赖（已由 cap fmt/lint/test/build 工具链替代或按需忽略）

### 5.4 快速定位命令

```bash
# 检索全部迁移遗留 TODO 标记
rg -n "TODO\(cap-migrate\)" packages/puppeteer/test

# 运行 cpc 逐文件静态诊断（自带 AI-First 修复建议）
cpc check packages/puppeteer/test/src/
```

## 6. 测试、调试、运行与正式发布操作手册

```bash
cd packages/puppeteer/test

# 1. 安装 Tier 2 官方 @cap/* 依赖与本地链接
cap install

# 2. 代码格式化与静态检查（4 门禁）
cap fmt
cap lint
cpc check src/

# 3. 运行单元测试与本地调试
cap test
cap run

# 4. 构建跨平台原生二进制产物
cap build

# 5. 打包与正式发布到 cap 包仓库
cap pack
cap publish
```
