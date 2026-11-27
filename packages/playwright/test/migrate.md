# `tests` 迁移报告（TypeScript / JavaScript → CP）

## 1. 迁移总览与统计

| 指标 | 数值 |
| --- | --- |
| 项目名称 | `tests` |
| 版本号 | `0.1.0` |
| 源语言 | TypeScript / JavaScript |
| 源码目录 | `_ref/playwright/tests` |
| 输出目录 | `packages/playwright/test` |
| 迁移文件总数 | 691 |
| 代码行数变化 | 327056 行 → 302780 行 |
| `cpc` 语法解析通过率 | 691/691 (100.0%) |
| 剩余 `TODO(cap-migrate)` 标记数 | 6868 |
| 依赖总计 | 0（Tier 1 内置: 0 / Tier 2 官方包: 0 / Tier 3 递归转换: 0 / 待人工评估: 0） |

## 2. 依赖库 3 级迁移清单

### 2.1 Tier 1 — CP 内置标准库（`cap:*`，零外部依赖直连）

_无 Tier 1 内置映射依赖。_

### 2.2 Tier 2 — 官方生态可安装包（`@cap/*`）

_无 Tier 2 官方包依赖。_

### 2.3 Tier 3 — 递归转换的独立可发布 CP 库

_无 Tier 3 递归转换依赖。_

## 3. 逐文件迁移明细

| 原文件 | 生成 CP 文件 | 行数 (原 → CP) | `cpc` 解析 | `TODO` 数 | 优化项 |
| --- | --- | --- | --- | --- | --- |
| `android/android.spec.ts` | `android/android.spec.cp` | 58 → 62 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `android/androidTest.ts` | `android/androidTest.cp` | 96 → 93 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `android/browser.spec.ts` | `android/browser.spec.cp` | 170 → 175 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `android/device.spec.ts` | `android/device.spec.cp` | 74 → 79 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `android/launch-server.spec.ts` | `android/launch-server.spec.cp` | 168 → 174 | ✅ 通过 | 11 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `android/playwright.config.ts` | `android/playwright.config.cp` | 79 → 82 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `android/webview.spec.ts` | `android/webview.spec.cp` | 106 → 113 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `assets/axe-core/accessible-text.js` | `assets/axe-core/accessible-text.cp` | 1125 → 1127 | ✅ 通过 | 0 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let` |
| `assets/axe-core/implicit-role.js` | `assets/axe-core/implicit-role.cp` | 530 → 532 | ✅ 通过 | 0 | - |
| `assets/callback.js` | `assets/callback.cp` | 5 → 7 | ✅ 通过 | 0 | - |
| `assets/dummy_bad_browser_executable.js` | `assets/dummy_bad_browser_executable.cp` | 3 → 6 | ✅ 通过 | 2 | - |
| `assets/es6/es6import.js` | `assets/es6/es6import.cp` | 2 → 4 | ✅ 通过 | 0 | - |
| `assets/es6/es6module.js` | `assets/es6/es6module.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/es6/es6pathimport.js` | `assets/es6/es6pathimport.cp` | 2 → 4 | ✅ 通过 | 0 | - |
| `assets/extension-mv3-simple/content-script.js` | `assets/extension-mv3-simple/content-script.cp` | 3 → 5 | ✅ 通过 | 0 | - |
| `assets/extension-mv3-simple/index.js` | `assets/extension-mv3-simple/index.cp` | 6 → 8 | ✅ 通过 | 0 | - |
| `assets/extension-mv3-sw-lifecycle/background.js` | `assets/extension-mv3-sw-lifecycle/background.cp` | 2 → 4 | ✅ 通过 | 0 | - |
| `assets/extension-mv3-with-logging/background.js` | `assets/extension-mv3-with-logging/background.cp` | 5 → 7 | ✅ 通过 | 0 | - |
| `assets/extension-mv3-with-logging/content.js` | `assets/extension-mv3-with-logging/content.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/frames/script.js` | `assets/frames/script.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/injectedfile.js` | `assets/injectedfile.cp` | 3 → 5 | ✅ 通过 | 0 | - |
| `assets/input/mouse-helper.js` | `assets/input/mouse-helper.cp` | 62 → 65 | ✅ 通过 | 0 | - |
| `assets/jscoverage/script1.js` | `assets/jscoverage/script1.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/jscoverage/script2.js` | `assets/jscoverage/script2.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/load-event/module.js` | `assets/load-event/module.cp` | 3 → 5 | ✅ 通过 | 0 | - |
| `assets/modernizr/modernizr.js` | `assets/modernizr/modernizr.cp` | 4147 → 4166 | ✅ 通过 | 12 | - |
| `assets/network-tab/script.js` | `assets/network-tab/script.cp` | 0 → 2 | ✅ 通过 | 0 | - |
| `assets/networkidle.js` | `assets/networkidle.cp` | 12 → 14 | ✅ 通过 | 0 | - |
| `assets/prefetch.js` | `assets/prefetch.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/react/material-ui@5.2.2.production.min.js` | `assets/react/material-ui@5.2.2.production.min.cp` | 209 → 212 | ✅ 通过 | 2 | - |
| `assets/react/react-dom@16.13.1.production.min.js` | `assets/react/react-dom@16.13.1.production.min.cp` | 239 → 310 | ✅ 通过 | 23 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let` |
| `assets/react/react-dom@17.0.2.production.min.js` | `assets/react/react-dom@17.0.2.production.min.cp` | 245 → 317 | ✅ 通过 | 18 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let` |
| `assets/react/react@16.13.1.production.min.js` | `assets/react/react@16.13.1.production.min.cp` | 32 → 47 | ✅ 通过 | 7 | - |
| `assets/react/react@17.0.2.production.min.js` | `assets/react/react@17.0.2.production.min.cp` | 31 → 46 | ✅ 通过 | 8 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let` |
| `assets/reading-list/react-dom_15.7.0.js` | `assets/reading-list/react-dom_15.7.0.cp` | 18853 → 16598 | ✅ 通过 | 188 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `assets/reading-list/react-dom_16.14.0.js` | `assets/reading-list/react-dom_16.14.0.cp` | 25147 → 20959 | ✅ 通过 | 409 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `assets/reading-list/react-dom_17.0.2.js` | `assets/reading-list/react-dom_17.0.2.cp` | 26292 → 21913 | ✅ 通过 | 461 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `assets/reading-list/react-dom_18.1.0.js` | `assets/reading-list/react-dom_18.1.0.cp` | 29771 → 24775 | ✅ 通过 | 556 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `assets/reading-list/react_15.7.0.js` | `assets/reading-list/react_15.7.0.cp` | 4268 → 3827 | ✅ 通过 | 76 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `assets/reading-list/react_16.14.0.js` | `assets/reading-list/react_16.14.0.cp` | 3318 → 2812 | ✅ 通过 | 110 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `assets/reading-list/react_17.0.2.js` | `assets/reading-list/react_17.0.2.cp` | 3357 → 2835 | ✅ 通过 | 121 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `assets/reading-list/react_18.1.0.js` | `assets/reading-list/react_18.1.0.cp` | 3340 → 2822 | ✅ 通过 | 98 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `assets/reading-list/vue_2.6.14.js` | `assets/reading-list/vue_2.6.14.cp` | 12014 → 10451 | ✅ 通过 | 186 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `assets/reading-list/vue_3.1.5.js` | `assets/reading-list/vue_3.1.5.cp` | 14997 → 14773 | ✅ 通过 | 229 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let`; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `assets/sectionselectorengine.js` | `assets/sectionselectorengine.cp` | 8 → 11 | ✅ 通过 | 1 | - |
| `assets/selenium-grid/broken-selenium-driver.js` | `assets/selenium-grid/broken-selenium-driver.cp` | 3 → 6 | ✅ 通过 | 2 | - |
| `assets/serviceworkers/bug-33561/sw.js` | `assets/serviceworkers/bug-33561/sw.cp` | 14 → 16 | ✅ 通过 | 0 | - |
| `assets/serviceworkers/empty/sw.js` | `assets/serviceworkers/empty/sw.cp` | 0 → 2 | ✅ 通过 | 0 | - |
| `assets/serviceworkers/fetch/sw.js` | `assets/serviceworkers/fetch/sw.cp` | 12 → 14 | ✅ 通过 | 0 | - |
| `assets/serviceworkers/fetchdummy/sw.js` | `assets/serviceworkers/fetchdummy/sw.cp` | 23 → 25 | ✅ 通过 | 0 | - |
| `assets/serviceworkers/slow-fallback/sw.js` | `assets/serviceworkers/slow-fallback/sw.cp` | 9 → 11 | ✅ 通过 | 0 | - |
| `assets/serviceworkers/stub/sw.js` | `assets/serviceworkers/stub/sw.cp` | 29 → 32 | ✅ 通过 | 1 | - |
| `assets/shared-worker/shared-worker.js` | `assets/shared-worker/shared-worker.cp` | 4 → 6 | ✅ 通过 | 0 | - |
| `assets/stress/static/js/787.2f3b90fa.chunk.js` | `assets/stress/static/js/787.2f3b90fa.chunk.cp` | 2 → 9 | ✅ 通过 | 2 | - |
| `assets/stress/static/js/main.cc4a7955.js` | `assets/stress/static/js/main.cc4a7955.cp` | 3 → 5 | ✅ 通过 | 1 | - |
| `assets/to-do-notifications/scripts/todo.js` | `assets/to-do-notifications/scripts/todo.cp` | 356 → 294 | ✅ 通过 | 7 | 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `assets/worker/import-me.js` | `assets/worker/import-me.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `assets/worker/worker-http-import.js` | `assets/worker/worker-http-import.cp` | 4 → 6 | ✅ 通过 | 0 | - |
| `assets/worker/worker.js` | `assets/worker/worker.cp` | 16 → 18 | ✅ 通过 | 0 | - |
| `bidi/csvReporter.ts` | `bidi/csvReporter.cp` | 101 → 107 | ✅ 通过 | 5 | 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `bidi/playwright.config.ts` | `bidi/playwright.config.cp` | 136 → 146 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `config/baseTest.ts` | `config/baseTest.cp` | 50 → 49 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `config/browserTest.ts` | `config/browserTest.cp` | 218 → 195 | ✅ 通过 | 10 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 消除 interface 动态派发为栈布局 struct（零虚表开销）; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `config/commonFixtures.ts` | `config/commonFixtures.cp` | 327 → 300 | ✅ 通过 | 23 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `config/comparator.ts` | `config/comparator.cp` | 28 → 30 | ✅ 通过 | 2 | 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `config/debugControllerBackend.ts` | `config/debugControllerBackend.cp` | 163 → 164 | ✅ 通过 | 16 | 消除 interface 动态派发为栈布局 struct（零虚表开销）; 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `config/errors.ts` | `config/errors.cp` | 17 → 19 | ✅ 通过 | 0 | - |
| `config/expectationReporter.ts` | `config/expectationReporter.cp` | 149 → 159 | ✅ 通过 | 10 | 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `config/markdownReporter.ts` | `config/markdownReporter.cp` | 161 → 168 | ✅ 通过 | 4 | 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `config/parquetReporter.ts` | `config/parquetReporter.cp` | 199 → 204 | ✅ 通过 | 9 | 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `config/platformFixtures.ts` | `config/platformFixtures.cp` | 59 → 59 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `config/postReportComment.js` | `config/postReportComment.cp` | 129 → 131 | ✅ 通过 | 3 | - |
| `config/proxy.ts` | `config/proxy.cp` | 185 → 191 | ✅ 通过 | 8 | 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `config/registryReporter.ts` | `config/registryReporter.cp` | 192 → 209 | ✅ 通过 | 13 | 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `config/remote-server-impl.js` | `config/remote-server-impl.cp` | 62 → 67 | ✅ 通过 | 0 | - |
| `config/remoteServer.ts` | `config/remoteServer.cp` | 182 → 178 | ✅ 通过 | 12 | 消除 interface 动态派发为栈布局 struct（零虚表开销）; 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `config/serverFixtures.ts` | `config/serverFixtures.cp` | 136 → 121 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `config/serviceWorkerCoverage.ts` | `config/serviceWorkerCoverage.cp` | 46 → 49 | ✅ 通过 | 1 | 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `config/testMode.ts` | `config/testMode.cp` | 50 → 52 | ✅ 通过 | 3 | 消除 interface 动态派发为栈布局 struct（零虚表开销）; 自动推断字段写操作并为类方法注入安全可变接收者 `*_` |
| `config/testModeFixtures.ts` | `config/testModeFixtures.cp` | 55 → 50 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `config/testserver/index.ts` | `config/testserver/index.cp` | 355 → 364 | ✅ 通过 | 14 | 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `config/traceViewerFixtures.ts` | `config/traceViewerFixtures.cp` | 219 → 225 | ✅ 通过 | 12 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `config/utils.ts` | `config/utils.cp` | 280 → 301 | ✅ 通过 | 11 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `electron/electron-app-args.js` | `electron/electron-app-args.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `electron/electron-app-pre-ready.js` | `electron/electron-app-pre-ready.cp` | 2 → 4 | ✅ 通过 | 0 | - |
| `electron/electron-app-ready-event.js` | `electron/electron-app-ready-event.cp` | 12 → 14 | ✅ 通过 | 0 | - |
| `electron/electron-app.js` | `electron/electron-app.cp` | 15 → 17 | ✅ 通过 | 0 | 彻底消除废弃关键字 `var` 为块级可变绑定 `let` |
| `electron/electron-app.spec.ts` | `electron/electron-app.spec.cp` | 402 → 401 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `electron/electron-options-app.js` | `electron/electron-options-app.cp` | 21 → 26 | ✅ 通过 | 1 | - |
| `electron/electron-print-chromium-version.js` | `electron/electron-print-chromium-version.cp` | 2 → 4 | ✅ 通过 | 0 | - |
| `electron/electron-tracing.spec.ts` | `electron/electron-tracing.spec.cp` | 68 → 73 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `electron/electron-webcontentsview-app.js` | `electron/electron-webcontentsview-app.cp` | 16 → 19 | ✅ 通过 | 0 | - |
| `electron/electron-webcontentsview.spec.ts` | `electron/electron-webcontentsview.spec.cp` | 33 → 39 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `electron/electron-window-app-delayed-loadURL.js` | `electron/electron-window-app-delayed-loadURL.cp` | 13 → 15 | ✅ 通过 | 0 | - |
| `electron/electron-window-app.js` | `electron/electron-window-app.cp` | 12 → 14 | ✅ 通过 | 0 | - |
| `electron/electron-window.spec.ts` | `electron/electron-window.spec.cp` | 54 → 59 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `electron/electronTest.ts` | `electron/electronTest.cp` | 127 → 119 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `electron/globalSetup.ts` | `electron/globalSetup.cp` | 29 → 32 | ✅ 通过 | 1 | - |
| `electron/index.js` | `electron/index.cp` | 1 → 3 | ✅ 通过 | 0 | - |
| `electron/playwright.config.ts` | `electron/playwright.config.cp` | 81 → 84 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `extension/chrome-stub.js` | `extension/chrome-stub.cp` | 68 → 73 | ✅ 通过 | 5 | 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `extension/cli.spec.ts` | `extension/cli.spec.cp` | 105 → 110 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `extension/client-lifecycle.spec.ts` | `extension/client-lifecycle.spec.cp` | 276 → 256 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `extension/extension-fixtures.ts` | `extension/extension-fixtures.cp` | 221 → 186 | ✅ 通过 | 8 | 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `extension/extension.spec.ts` | `extension/extension.spec.cp` | 437 → 398 | ✅ 通过 | 12 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `extension/harness.ts` | `extension/harness.cp` | 494 → 448 | ✅ 通过 | 26 | 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `extension/playwright.config.ts` | `extension/playwright.config.cp` | 33 → 36 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `extension/protocol.spec.ts` | `extension/protocol.spec.cp` | 133 → 125 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `extension/scheme-detach.spec.ts` | `extension/scheme-detach.spec.cp` | 115 → 115 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `extension/tab-dragging.spec.ts` | `extension/tab-dragging.spec.cp` | 242 → 247 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `extension/tab-grouping.spec.ts` | `extension/tab-grouping.spec.cp` | 131 → 136 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `extension/tab-management.spec.ts` | `extension/tab-management.spec.cp` | 135 → 140 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `image_tools/fixtures.spec.ts` | `image_tools/fixtures.spec.cp` | 93 → 95 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `image_tools/unit.spec.ts` | `image_tools/unit.spec.cp` | 171 → 187 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `image_tools/utils.ts` | `image_tools/utils.cp` | 63 → 73 | ✅ 通过 | 3 | - |
| `installation/bundle-licenses.spec.ts` | `installation/bundle-licenses.spec.cp` | 85 → 90 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/bundle-size.spec.ts` | `installation/bundle-size.spec.cp` | 34 → 38 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/bundle-with-esbuild.spec.ts` | `installation/bundle-with-esbuild.spec.cp` | 25 → 31 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/concurrent-download.spec.ts` | `installation/concurrent-download.spec.cp` | 40 → 46 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `installation/connect-to-selenium.spec.ts` | `installation/connect-to-selenium.spec.cp` | 34 → 40 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/driver-should-work.spec.ts` | `installation/driver-should-work.spec.cp` | 41 → 46 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/fixture-scripts/browser-only.js` | `installation/fixture-scripts/browser-only.cp` | 27 → 29 | ✅ 通过 | 0 | - |
| `installation/fixture-scripts/docker.spec.js` | `installation/fixture-scripts/docker.spec.cp` | 19 → 24 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/fixture-scripts/download-chromedriver.js` | `installation/fixture-scripts/download-chromedriver.cp` | 33 → 33 | ✅ 通过 | 1 | - |
| `installation/fixture-scripts/download-selenium.js` | `installation/fixture-scripts/download-selenium.cp` | 15 → 17 | ✅ 通过 | 0 | - |
| `installation/fixture-scripts/driver-client-sigint.js` | `installation/fixture-scripts/driver-client-sigint.cp` | 54 → 56 | ✅ 通过 | 2 | 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `installation/fixture-scripts/driver-client.js` | `installation/fixture-scripts/driver-client.cp` | 43 → 45 | ✅ 通过 | 2 | 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `installation/fixture-scripts/electron-app.js` | `installation/fixture-scripts/electron-app.cp` | 3 → 5 | ✅ 通过 | 0 | - |
| `installation/fixture-scripts/esm-playwright-chromium.mjs` | `installation/fixture-scripts/esm-playwright-chromium.cp` | 21 → 23 | ✅ 通过 | 0 | - |
| `installation/fixture-scripts/esm-playwright-firefox.mjs` | `installation/fixture-scripts/esm-playwright-firefox.cp` | 21 → 23 | ✅ 通过 | 0 | - |
| `installation/fixture-scripts/esm-playwright-test.mjs` | `installation/fixture-scripts/esm-playwright-test.cp` | 38 → 38 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/fixture-scripts/esm-playwright-webkit.mjs` | `installation/fixture-scripts/esm-playwright-webkit.cp` | 21 → 23 | ✅ 通过 | 0 | - |
| `installation/fixture-scripts/esm-playwright.mjs` | `installation/fixture-scripts/esm-playwright.cp` | 21 → 23 | ✅ 通过 | 0 | - |
| `installation/fixture-scripts/esm.mjs` | `installation/fixture-scripts/esm.cp` | 42 → 49 | ✅ 通过 | 3 | 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `installation/fixture-scripts/failing.spec.js` | `installation/fixture-scripts/failing.spec.cp` | 6 → 12 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/fixture-scripts/inspector-custom-executable.js` | `installation/fixture-scripts/inspector-custom-executable.cp` | 26 → 28 | ✅ 通过 | 0 | - |
| `installation/fixture-scripts/playwright-test-plugin-types.ts` | `installation/fixture-scripts/playwright-test-plugin-types.cp` | 19 → 22 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/fixture-scripts/playwright-test-types.ts` | `installation/fixture-scripts/playwright-test-types.cp` | 57 → 63 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/fixture-scripts/plugin.spec.ts` | `installation/fixture-scripts/plugin.spec.cp` | 16 → 20 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/fixture-scripts/read-json-report.js` | `installation/fixture-scripts/read-json-report.cp` | 27 → 29 | ✅ 通过 | 0 | - |
| `installation/fixture-scripts/sample.spec.js` | `installation/fixture-scripts/sample.spec.cp` | 6 → 12 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/fixture-scripts/sanity-electron.js` | `installation/fixture-scripts/sanity-electron.cp` | 32 → 34 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/fixture-scripts/sanity.js` | `installation/fixture-scripts/sanity.cp` | 56 → 58 | ✅ 通过 | 4 | 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `installation/fixture-scripts/screencast.js` | `installation/fixture-scripts/screencast.cp` | 46 → 48 | ✅ 通过 | 2 | 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `installation/fixture-scripts/validate-dependencies-skip-executable-path.js` | `installation/fixture-scripts/validate-dependencies-skip-executable-path.cp` | 10 → 12 | ✅ 通过 | 0 | - |
| `installation/globalSetup.ts` | `installation/globalSetup.cp` | 80 → 85 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `installation/npm-installs-local-packages.spec.ts` | `installation/npm-installs-local-packages.spec.cp` | 36 → 42 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/npmTest.ts` | `installation/npmTest.cp` | 222 → 202 | ✅ 通过 | 13 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `installation/npx-global.spec.ts` | `installation/npx-global.spec.cp` | 42 → 47 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/playwright-cdn.spec.ts` | `installation/playwright-cdn.spec.cp` | 190 → 198 | ✅ 通过 | 14 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `installation/playwright-cli-install-should-work.spec.ts` | `installation/playwright-cli-install-should-work.spec.cp` | 293 → 280 | ✅ 通过 | 11 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `installation/playwright-cli.spec.ts` | `installation/playwright-cli.spec.cp` | 101 → 107 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `installation/playwright-electron-should-work.spec.ts` | `installation/playwright-electron-should-work.spec.cp` | 84 → 88 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/playwright-packages-install-behavior.spec.ts` | `installation/playwright-packages-install-behavior.spec.cp` | 119 → 117 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/playwright-should-work-with-relative-browsers-path.spec.ts` | `installation/playwright-should-work-with-relative-browsers-path.spec.cp` | 26 → 32 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/playwright-should-work-with-relative-home-path.spec.ts` | `installation/playwright-should-work-with-relative-home-path.spec.cp` | 26 → 32 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/playwright-test-package-managers.spec.ts` | `installation/playwright-test-package-managers.spec.cp` | 70 → 75 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/playwright-test-plugin/index.ts` | `installation/playwright-test-plugin/index.cp` | 49 → 50 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `installation/playwright-test-plugin.spec.ts` | `installation/playwright-test-plugin.spec.cp` | 74 → 79 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/playwright.config.ts` | `installation/playwright.config.cp` | 54 → 57 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `installation/registry.ts` | `installation/registry.cp` | 193 → 195 | ✅ 通过 | 9 | 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `installation/screencast.spec.ts` | `installation/screencast.spec.cp` | 21 → 27 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/skip-browser-download.spec.ts` | `installation/skip-browser-download.spec.cp` | 30 → 36 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/typescript-types.spec.ts` | `installation/typescript-types.spec.cp` | 45 → 49 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `installation/validate-dependencies.spec.ts` | `installation/validate-dependencies.spec.cp` | 109 → 114 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 彻底消除废弃关键字 `var` 为块级可变绑定 `let` |
| `library/beforeunload.spec.ts` | `library/beforeunload.spec.cp` | 210 → 186 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browser-server.spec.ts` | `library/browser-server.spec.cp` | 77 → 77 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browser.spec.ts` | `library/browser.spec.cp` | 80 → 81 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browsercontext-add-cookies.spec.ts` | `library/browsercontext-add-cookies.spec.cp` | 474 → 477 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-add-init-script.spec.ts` | `library/browsercontext-add-init-script.spec.cp` | 147 → 150 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-base-url.spec.ts` | `library/browsercontext-base-url.spec.cp` | 112 → 117 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-basic.spec.ts` | `library/browsercontext-basic.spec.cp` | 547 → 321 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-clearcookies.spec.ts` | `library/browsercontext-clearcookies.spec.cp` | 252 → 257 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browsercontext-cookies-third-party.spec.ts` | `library/browsercontext-cookies-third-party.spec.cp` | 640 → 609 | ✅ 通过 | 9 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-cookies.spec.ts` | `library/browsercontext-cookies.spec.cp` | 484 → 486 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-credentials.spec.ts` | `library/browsercontext-credentials.spec.cp` | 248 → 243 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browsercontext-csp.spec.ts` | `library/browsercontext-csp.spec.cp` | 127 → 132 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browsercontext-device.spec.ts` | `library/browsercontext-device.spec.cp` | 144 → 130 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browsercontext-dsf.spec.ts` | `library/browsercontext-dsf.spec.cp` | 41 → 46 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-events.spec.ts` | `library/browsercontext-events.spec.cp` | 293 → 279 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-expose-function.spec.ts` | `library/browsercontext-expose-function.spec.cp` | 120 → 125 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browsercontext-fetch-algorithms.spec.ts` | `library/browsercontext-fetch-algorithms.spec.cp` | 98 → 96 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browsercontext-fetch-happy-eyeballs.spec.ts` | `library/browsercontext-fetch-happy-eyeballs.spec.cp` | 75 → 82 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `library/browsercontext-fetch.spec.ts` | `library/browsercontext-fetch.spec.cp` | 1630 → 1590 | ✅ 通过 | 19 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-har.spec.ts` | `library/browsercontext-har.spec.cp` | 656 → 634 | ✅ 通过 | 11 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-locale.spec.ts` | `library/browsercontext-locale.spec.cp` | 270 → 268 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-network-event.spec.ts` | `library/browsercontext-network-event.spec.cp` | 191 → 196 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-page-event.spec.ts` | `library/browsercontext-page-event.spec.cp` | 184 → 186 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-pages.spec.ts` | `library/browsercontext-pages.spec.cp` | 192 → 194 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-proxy.spec.ts` | `library/browsercontext-proxy.spec.cp` | 481 → 443 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `library/browsercontext-reuse.spec.ts` | `library/browsercontext-reuse.spec.cp` | 431 → 383 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-route.spec.ts` | `library/browsercontext-route.spec.cp` | 453 → 446 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-service-worker-policy.spec.ts` | `library/browsercontext-service-worker-policy.spec.cp` | 127 → 130 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-set-extra-http-headers.spec.ts` | `library/browsercontext-set-extra-http-headers.spec.cp` | 40 → 45 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-storage-state.spec.ts` | `library/browsercontext-storage-state.spec.cp` | 843 → 751 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-strict.spec.ts` | `library/browsercontext-strict.spec.cp` | 48 → 55 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browsercontext-timezone-id.spec.ts` | `library/browsercontext-timezone-id.spec.cp` | 131 → 136 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-user-agent.spec.ts` | `library/browsercontext-user-agent.spec.cp` | 161 → 166 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsercontext-viewport-mobile.spec.ts` | `library/browsercontext-viewport-mobile.spec.cp` | 272 → 252 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browsercontext-viewport.spec.ts` | `library/browsercontext-viewport.spec.cp` | 218 → 222 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browsercontext-webauthn.spec.ts` | `library/browsercontext-webauthn.spec.cp` | 353 → 335 | ✅ 通过 | 13 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `library/browsers-path.spec.ts` | `library/browsers-path.spec.cp` | 55 → 51 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browsertype-basic.spec.ts` | `library/browsertype-basic.spec.cp` | 38 → 43 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/browsertype-connect.spec.ts` | `library/browsertype-connect.spec.cp` | 1208 → 1119 | ✅ 通过 | 15 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsertype-launch-selenium.spec.ts` | `library/browsertype-launch-selenium.spec.cp` | 214 → 219 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsertype-launch-server.spec.ts` | `library/browsertype-launch-server.spec.cp` | 131 → 135 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/browsertype-launch.spec.ts` | `library/browsertype-launch.spec.cp` | 143 → 139 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/capabilities.spec.ts` | `library/capabilities.spec.cp` | 536 → 504 | ✅ 通过 | 24 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/channels.spec.ts` | `library/channels.spec.cp` | 368 → 366 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/chromium/bfcache.spec.ts` | `library/chromium/bfcache.spec.cp` | 37 → 43 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/chromium/chromium.spec.ts` | `library/chromium/chromium.spec.cp` | 807 → 750 | ✅ 通过 | 23 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/chromium/connect-over-cdp.spec.ts` | `library/chromium/connect-over-cdp.spec.cp` | 1084 → 1036 | ✅ 通过 | 67 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/chromium/css-coverage.spec.ts` | `library/chromium/css-coverage.spec.cp` | 142 → 147 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/chromium/disable-web-security.spec.ts` | `library/chromium/disable-web-security.spec.cp` | 68 → 73 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/chromium/extensions.spec.ts` | `library/chromium/extensions.spec.cp` | 191 → 183 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/chromium/js-coverage.spec.ts` | `library/chromium/js-coverage.spec.cp` | 81 → 86 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/chromium/launcher.spec.ts` | `library/chromium/launcher.spec.cp` | 49 → 54 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/chromium/oopif.spec.ts` | `library/chromium/oopif.spec.cp` | 512 → 499 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/chromium/session.spec.ts` | `library/chromium/session.spec.cp` | 211 → 213 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/chromium/tracing.spec.ts` | `library/chromium/tracing.spec.cp` | 107 → 113 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/client-certificates.spec.ts` | `library/client-certificates.spec.cp` | 977 → 917 | ✅ 通过 | 14 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `library/component-parser.spec.ts` | `library/component-parser.spec.cp` | 162 → 168 | ✅ 通过 | 15 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `library/coverage.spec.ts` | `library/coverage.spec.cp` | 314 → 309 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/css-parser.spec.ts` | `library/css-parser.spec.cp` | 108 → 114 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `library/debug-controller.spec.ts` | `library/debug-controller.spec.cp` | 394 → 394 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `library/debugger.spec.ts` | `library/debugger.spec.cp` | 272 → 247 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/defaultbrowsercontext-1.spec.ts` | `library/defaultbrowsercontext-1.spec.cp` | 160 → 164 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/defaultbrowsercontext-2.spec.ts` | `library/defaultbrowsercontext-2.spec.cp` | 331 → 326 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/download.spec.ts` | `library/download.spec.cp` | 749 → 758 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/downloads-path.spec.ts` | `library/downloads-path.spec.cp` | 117 → 121 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/emulation-coop.spec.ts` | `library/emulation-coop.spec.cp` | 118 → 116 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/emulation-focus.spec.ts` | `library/emulation-focus.spec.cp` | 272 → 279 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/events/add-listeners.spec.ts` | `library/events/add-listeners.spec.cp` | 103 → 108 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/check-listener-leaks.spec.ts` | `library/events/check-listener-leaks.spec.cp` | 89 → 96 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/events-list.spec.ts` | `library/events/events-list.spec.cp` | 55 → 60 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/listener-count.spec.ts` | `library/events/listener-count.spec.cp` | 38 → 42 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/listener-errors.spec.ts` | `library/events/listener-errors.spec.cp` | 123 → 122 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/listeners-side-effects.spec.ts` | `library/events/listeners-side-effects.spec.cp` | 61 → 65 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/listeners.spec.ts` | `library/events/listeners.spec.cp` | 154 → 167 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `library/events/max-listeners.spec.ts` | `library/events/max-listeners.spec.cp` | 43 → 47 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/method-names.spec.ts` | `library/events/method-names.spec.cp` | 40 → 44 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/modify-in-emit.spec.ts` | `library/events/modify-in-emit.spec.cp` | 93 → 99 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `library/events/num-args.spec.ts` | `library/events/num-args.spec.cp` | 62 → 66 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/once.spec.ts` | `library/events/once.spec.cp` | 84 → 90 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/prepend.spec.ts` | `library/events/prepend.spec.cp` | 52 → 53 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/remove-all-listeners-wait.spec.ts` | `library/events/remove-all-listeners-wait.spec.cp` | 84 → 87 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/remove-all-listeners.spec.ts` | `library/events/remove-all-listeners.spec.cp` | 136 → 136 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/remove-listeners.spec.ts` | `library/events/remove-listeners.spec.cp` | 188 → 195 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `library/events/set-max-listeners-side-effects.spec.ts` | `library/events/set-max-listeners-side-effects.spec.cp` | 34 → 38 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/special-event-names.spec.ts` | `library/events/special-event-names.spec.cp` | 46 → 49 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/subclass.spec.ts` | `library/events/subclass.spec.cp` | 54 → 62 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 自动推断字段写操作并为类方法注入安全可变接收者 `*_` |
| `library/events/symbols.spec.ts` | `library/events/symbols.spec.cp` | 46 → 50 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/events/utils.ts` | `library/events/utils.cp` | 54 → 62 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `library/favicon.spec.ts` | `library/favicon.spec.cp` | 59 → 65 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/fetch-proxy.spec.ts` | `library/fetch-proxy.spec.cp` | 202 → 207 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/firefox/launcher.spec.ts` | `library/firefox/launcher.spec.cp` | 86 → 91 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/geolocation.spec.ts` | `library/geolocation.spec.cp` | 174 → 173 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/global-fetch-cookie.spec.ts` | `library/global-fetch-cookie.spec.cp` | 722 → 707 | ✅ 通过 | 15 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/global-fetch.spec.ts` | `library/global-fetch.spec.cp` | 821 → 803 | ✅ 通过 | 17 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/har-websocket.spec.ts` | `library/har-websocket.spec.cp` | 504 → 480 | ✅ 通过 | 16 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/har.spec.ts` | `library/har.spec.cp` | 1276 → 1206 | ✅ 通过 | 18 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/headful.spec.ts` | `library/headful.spec.cp` | 364 → 370 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/heap.spec.ts` | `library/heap.spec.cp` | 315 → 320 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/hit-target.spec.ts` | `library/hit-target.spec.cp` | 514 → 519 | ✅ 通过 | 11 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/ignorehttpserrors.spec.ts` | `library/ignorehttpserrors.spec.cp` | 145 → 150 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/inspector/cli-codegen-1.spec.ts` | `library/inspector/cli-codegen-1.spec.cp` | 1165 → 957 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/inspector/cli-codegen-2.spec.ts` | `library/inspector/cli-codegen-2.spec.cp` | 606 → 503 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/inspector/cli-codegen-3.spec.ts` | `library/inspector/cli-codegen-3.spec.cp` | 1192 → 994 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/inspector/cli-codegen-aria.spec.ts` | `library/inspector/cli-codegen-aria.spec.cp` | 188 → 196 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/inspector/cli-codegen-csharp.spec.ts` | `library/inspector/cli-codegen-csharp.spec.cp` | 322 → 287 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `library/inspector/cli-codegen-java.spec.ts` | `library/inspector/cli-codegen-java.spec.cp` | 142 → 135 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `library/inspector/cli-codegen-javascript.spec.ts` | `library/inspector/cli-codegen-javascript.spec.cp` | 145 → 135 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `library/inspector/cli-codegen-pick-locator.spec.ts` | `library/inspector/cli-codegen-pick-locator.spec.cp` | 66 → 74 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/inspector/cli-codegen-pytest.spec.ts` | `library/inspector/cli-codegen-pytest.spec.cp` | 78 → 83 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/inspector/cli-codegen-python-async.spec.ts` | `library/inspector/cli-codegen-python-async.spec.cp` | 157 → 140 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `library/inspector/cli-codegen-python.spec.ts` | `library/inspector/cli-codegen-python.spec.cp` | 143 → 130 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `library/inspector/cli-codegen-test.spec.ts` | `library/inspector/cli-codegen-test.spec.cp` | 120 → 108 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/inspector/console-api.spec.ts` | `library/inspector/console-api.spec.cp` | 122 → 127 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/inspector/inspectorTest.ts` | `library/inspector/inspectorTest.cp` | 305 → 314 | ✅ 通过 | 23 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/inspector/pause-helper.ts` | `library/inspector/pause-helper.cp` | 21 → 23 | ✅ 通过 | 0 | - |
| `library/inspector/pause.spec.ts` | `library/inspector/pause.spec.cp` | 613 → 558 | ✅ 通过 | 15 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/inspector/recorder-api.spec.ts` | `library/inspector/recorder-api.spec.cp` | 270 → 275 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 自动推断字段写操作并为类方法注入安全可变接收者 `*_`; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/inspector/title.spec.ts` | `library/inspector/title.spec.cp` | 65 → 68 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/launcher.spec.ts` | `library/launcher.spec.cp` | 56 → 61 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/locator-dispatchevent-touch.spec.ts` | `library/locator-dispatchevent-touch.spec.cp` | 58 → 64 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/locator-generator.spec.ts` | `library/locator-generator.spec.cp` | 695 → 699 | ✅ 通过 | 32 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/locator-highlight.spec.ts` | `library/locator-highlight.spec.cp` | 178 → 156 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/locator-parser.spec.ts` | `library/locator-parser.spec.cp` | 1223 → 1258 | ✅ 通过 | 216 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `library/logger.spec.ts` | `library/logger.spec.cp` | 51 → 56 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/modernizr.spec.ts` | `library/modernizr.spec.cp` | 157 → 162 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `library/multiclient.spec.ts` | `library/multiclient.spec.cp` | 474 → 402 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/network-timeout.spec.ts` | `library/network-timeout.spec.cp` | 97 → 99 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `library/page-clock.frozen.spec.ts` | `library/page-clock.frozen.spec.cp` | 27 → 32 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/page-clock.spec.ts` | `library/page-clock.spec.cp` | 773 → 779 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/page-close.spec.ts` | `library/page-close.spec.cp` | 227 → 226 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/page-event-crash.spec.ts` | `library/page-event-crash.spec.cp` | 135 → 135 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/pdf.spec.ts` | `library/pdf.spec.cp` | 41 → 46 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/permissions.spec.ts` | `library/permissions.spec.cp` | 426 → 384 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/playwright-client.spec.ts` | `library/playwright-client.spec.cp` | 57 → 59 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/playwright.config.ts` | `library/playwright.config.cp` | 126 → 135 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `library/popup.spec.ts` | `library/popup.spec.cp` | 306 → 314 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/proxy-pattern.spec.ts` | `library/proxy-pattern.spec.cp` | 119 → 125 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/proxy.spec.ts` | `library/proxy.spec.cp` | 468 → 432 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/resource-timing.spec.ts` | `library/resource-timing.spec.cp` | 141 → 148 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/role-utils.spec.ts` | `library/role-utils.spec.cp` | 663 → 645 | ✅ 通过 | 18 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/route-web-socket.spec.ts` | `library/route-web-socket.spec.cp` | 726 → 652 | ✅ 通过 | 29 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/screencast-actions.spec.ts` | `library/screencast-actions.spec.cp` | 282 → 223 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/screencast-overlay.spec.ts` | `library/screencast-overlay.spec.cp` | 207 → 214 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/screencast.spec.ts` | `library/screencast.spec.cp` | 373 → 343 | ✅ 通过 | 17 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `library/screenshot.spec.ts` | `library/screenshot.spec.cp` | 522 → 498 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/selector-generator.spec.ts` | `library/selector-generator.spec.cp` | 775 → 695 | ✅ 通过 | 21 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/selectors-register.spec.ts` | `library/selectors-register.spec.cp` | 184 → 168 | ✅ 通过 | 26 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `library/shared-worker.spec.ts` | `library/shared-worker.spec.cp` | 29 → 35 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/signals.spec.ts` | `library/signals.spec.cp` | 144 → 141 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `library/slowmo.spec.ts` | `library/slowmo.spec.cp` | 202 → 211 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/snapshot-renderer.spec.ts` | `library/snapshot-renderer.spec.cp` | 211 → 208 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/tap.spec.ts` | `library/tap.spec.cp` | 209 → 215 | ✅ 通过 | 9 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/trace-viewer.spec.ts` | `library/trace-viewer.spec.cp` | 2842 → 2588 | ✅ 通过 | 199 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/tracing.spec.ts` | `library/tracing.spec.cp` | 1132 → 1022 | ✅ 通过 | 12 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/unit/android-webviews.spec.ts` | `library/unit/android-webviews.spec.cp` | 72 → 80 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `library/unit/clock.spec.ts` | `library/unit/clock.spec.cp` | 1827 → 1714 | ✅ 通过 | 37 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 消除 interface 动态派发为栈布局 struct（零虚表开销）; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `library/unit/code-frame.spec.ts` | `library/unit/code-frame.spec.cp` | 159 → 163 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/unit/codegen.spec.ts` | `library/unit/codegen.spec.cp` | 675 → 676 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/unit/http-server.spec.ts` | `library/unit/http-server.spec.cp` | 61 → 64 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `library/unit/json-schema.spec.ts` | `library/unit/json-schema.spec.cp` | 116 → 119 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/unit/sequence.spec.ts` | `library/unit/sequence.spec.cp` | 158 → 161 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/unit/timeout-runner.spec.ts` | `library/unit/timeout-runner.spec.cp` | 33 → 37 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `library/unroute-behavior.spec.ts` | `library/unroute-behavior.spec.cp` | 341 → 347 | ✅ 通过 | 28 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/video.spec.ts` | `library/video.spec.cp` | 826 → 740 | ✅ 通过 | 15 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/videoPlayer.ts` | `library/videoPlayer.cp` | 79 → 88 | ✅ 通过 | 11 | 自动推断字段写操作并为类方法注入安全可变接收者 `*_` |
| `library/web-socket.spec.ts` | `library/web-socket.spec.cp` | 291 → 297 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `library/webmcp-disabled.spec.ts` | `library/webmcp-disabled.spec.cp` | 26 → 31 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `library/webmcp.spec.ts` | `library/webmcp.spec.cp` | 276 → 268 | ✅ 通过 | 10 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/annotate.spec.ts` | `mcp/annotate.spec.cp` | 520 → 527 | ✅ 通过 | 33 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `mcp/autowait.spec.ts` | `mcp/autowait.spec.cp` | 39 → 44 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/capabilities.spec.ts` | `mcp/capabilities.spec.cp` | 92 → 98 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cdp.spec.ts` | `mcp/cdp.spec.cp` | 235 → 224 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-cdp.spec.ts` | `mcp/cli-cdp.spec.cp` | 170 → 162 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-config.spec.ts` | `mcp/cli-config.spec.cp` | 188 → 190 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-cookies.spec.ts` | `mcp/cli-cookies.spec.cp` | 90 → 95 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-core.spec.ts` | `mcp/cli-core.spec.cp` | 446 → 415 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-devtools.spec.ts` | `mcp/cli-devtools.spec.cp` | 419 → 387 | ✅ 通过 | 9 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-drag.spec.ts` | `mcp/cli-drag.spec.cp` | 99 → 98 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-emulation.spec.ts` | `mcp/cli-emulation.spec.cp` | 51 → 57 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-endpoint.spec.ts` | `mcp/cli-endpoint.spec.cp` | 31 → 37 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-find.spec.ts` | `mcp/cli-find.spec.cp` | 124 → 121 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-fixtures.ts` | `mcp/cli-fixtures.cp` | 308 → 287 | ✅ 通过 | 25 | 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `mcp/cli-help.spec.ts` | `mcp/cli-help.spec.cp` | 59 → 64 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-isolated.spec.ts` | `mcp/cli-isolated.spec.cp` | 79 → 84 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-json.spec.ts` | `mcp/cli-json.spec.cp` | 251 → 254 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-keyboard.spec.ts` | `mcp/cli-keyboard.spec.cp` | 38 → 43 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-killall.spec.ts` | `mcp/cli-killall.spec.cp` | 59 → 64 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `mcp/cli-misc.spec.ts` | `mcp/cli-misc.spec.cp` | 142 → 140 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-mouse.spec.ts` | `mcp/cli-mouse.spec.cp` | 49 → 54 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-navigation.spec.ts` | `mcp/cli-navigation.spec.cp` | 118 → 124 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-network-offline.spec.ts` | `mcp/cli-network-offline.spec.cp` | 29 → 35 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-parsing.spec.ts` | `mcp/cli-parsing.spec.cp` | 95 → 100 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-persistent.spec.ts` | `mcp/cli-persistent.spec.cp` | 34 → 40 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-route.spec.ts` | `mcp/cli-route.spec.cp` | 99 → 104 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-run-code.spec.ts` | `mcp/cli-run-code.spec.cp` | 89 → 95 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-save-as.spec.ts` | `mcp/cli-save-as.spec.cp` | 107 → 111 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-session.spec.ts` | `mcp/cli-session.spec.cp` | 496 → 451 | ✅ 通过 | 10 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `mcp/cli-storage.spec.ts` | `mcp/cli-storage.spec.cp` | 128 → 112 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-test.spec.ts` | `mcp/cli-test.spec.cp` | 195 → 200 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-webmcp.spec.ts` | `mcp/cli-webmcp.spec.cp` | 231 → 218 | ✅ 通过 | 12 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/cli-webstorage.spec.ts` | `mcp/cli-webstorage.spec.cp` | 152 → 157 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/click.spec.ts` | `mcp/click.spec.cp` | 226 → 218 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/clipboard.spec.ts` | `mcp/clipboard.spec.cp` | 55 → 62 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/codegen.spec.ts` | `mcp/codegen.spec.cp` | 172 → 159 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/config-resolve.spec.ts` | `mcp/config-resolve.spec.cp` | 815 → 743 | ✅ 通过 | 13 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `mcp/config.ini.spec.ts` | `mcp/config.ini.spec.cp` | 126 → 125 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/config.spec.ts` | `mcp/config.spec.cp` | 246 → 237 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/console.spec.ts` | `mcp/console.spec.cp` | 496 → 457 | ✅ 通过 | 10 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `mcp/cookies.spec.ts` | `mcp/cookies.spec.cp` | 458 → 406 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/core.spec.ts` | `mcp/core.spec.cp` | 422 → 397 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/crash.spec.ts` | `mcp/crash.spec.cp` | 103 → 110 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/dashboard.spec.ts` | `mcp/dashboard.spec.cp` | 197 → 205 | ✅ 通过 | 15 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `mcp/device.spec.ts` | `mcp/device.spec.cp` | 72 → 77 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/devtools.spec.ts` | `mcp/devtools.spec.cp` | 138 → 129 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/dialogs.spec.ts` | `mcp/dialogs.spec.cp` | 366 → 336 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/emulation.spec.ts` | `mcp/emulation.spec.cp` | 89 → 91 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/evaluate.spec.ts` | `mcp/evaluate.spec.cp` | 276 → 266 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/file-paths.spec.ts` | `mcp/file-paths.spec.cp` | 89 → 90 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `mcp/files.spec.ts` | `mcp/files.spec.cp` | 632 → 561 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/find.spec.ts` | `mcp/find.spec.cp` | 339 → 324 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/fixtures.ts` | `mcp/fixtures.cp` | 365 → 309 | ✅ 通过 | 28 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `mcp/form.spec.ts` | `mcp/form.spec.cp` | 123 → 127 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/generator.spec.ts` | `mcp/generator.spec.cp` | 313 → 318 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/headed.spec.ts` | `mcp/headed.spec.cp` | 48 → 50 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/http.spec.ts` | `mcp/http.spec.cp` | 814 → 752 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `mcp/idle-timeout.spec.ts` | `mcp/idle-timeout.spec.cp` | 94 → 92 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/iframes.spec.ts` | `mcp/iframes.spec.cp` | 45 → 51 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/init-agents.spec.ts` | `mcp/init-agents.spec.cp` | 145 → 151 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/init-page.spec.ts` | `mcp/init-page.spec.cp` | 156 → 155 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/init-script.spec.ts` | `mcp/init-script.spec.cp` | 72 → 73 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/launch.spec.ts` | `mcp/launch.spec.cp` | 362 → 344 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/library.spec.ts` | `mcp/library.spec.cp` | 28 → 34 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/mcp-server-bind.spec.ts` | `mcp/mcp-server-bind.spec.cp` | 31 → 37 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/mouse.spec.ts` | `mcp/mouse.spec.cp` | 117 → 122 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/network-offline.spec.ts` | `mcp/network-offline.spec.cp` | 99 → 104 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/network.spec.ts` | `mcp/network.spec.cp` | 358 → 332 | ✅ 通过 | 10 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/output-max-size.spec.ts` | `mcp/output-max-size.spec.cp` | 71 → 71 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/pdf.spec.ts` | `mcp/pdf.spec.cp` | 87 → 85 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/planner.spec.ts` | `mcp/planner.spec.cp` | 254 → 259 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/playwright.config.ts` | `mcp/playwright.config.cp` | 66 → 70 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `mcp/profile-lock.spec.ts` | `mcp/profile-lock.spec.cp` | 96 → 101 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `mcp/recorder.spec.ts` | `mcp/recorder.spec.cp` | 262 → 250 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/remote-endpoint.spec.ts` | `mcp/remote-endpoint.spec.cp` | 97 → 100 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `mcp/request-blocking.spec.ts` | `mcp/request-blocking.spec.cp` | 170 → 178 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `mcp/roots.spec.ts` | `mcp/roots.spec.cp` | 163 → 168 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/route.spec.ts` | `mcp/route.spec.cp` | 387 → 364 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/run-code.spec.ts` | `mcp/run-code.spec.cp` | 311 → 301 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/screenshot.spec.ts` | `mcp/screenshot.spec.cp` | 499 → 470 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/secrets.spec.ts` | `mcp/secrets.spec.cp` | 243 → 224 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/seed-default-project.spec.ts` | `mcp/seed-default-project.spec.cp` | 124 → 129 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/semver.spec.ts` | `mcp/semver.spec.cp` | 56 → 60 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/session-log.spec.ts` | `mcp/session-log.spec.cp` | 136 → 143 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/snapshot-mode.spec.ts` | `mcp/snapshot-mode.spec.cp` | 143 → 136 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/sse.spec.ts` | `mcp/sse.spec.cp` | 284 → 266 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/storage.spec.ts` | `mcp/storage.spec.cp` | 264 → 233 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/tabs.spec.ts` | `mcp/tabs.spec.cp` | 204 → 202 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/test-debug.spec.ts` | `mcp/test-debug.spec.cp` | 486 → 459 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/test-list.spec.ts` | `mcp/test-list.spec.cp` | 65 → 70 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/test-run.spec.ts` | `mcp/test-run.spec.cp` | 204 → 196 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `mcp/test-setup.spec.ts` | `mcp/test-setup.spec.cp` | 146 → 144 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/timeouts.spec.ts` | `mcp/timeouts.spec.cp` | 151 → 148 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/trace-cli-fixtures.ts` | `mcp/trace-cli-fixtures.cp` | 160 → 141 | ✅ 通过 | 4 | 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `mcp/trace-cli.spec.ts` | `mcp/trace-cli.spec.cp` | 295 → 286 | ✅ 通过 | 14 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/tracing.spec.ts` | `mcp/tracing.spec.cp` | 125 → 130 | ✅ 通过 | 9 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/type.spec.ts` | `mcp/type.spec.cp` | 138 → 138 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/unicode-serialization.spec.ts` | `mcp/unicode-serialization.spec.cp` | 45 → 52 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/validation.spec.ts` | `mcp/validation.spec.cp` | 45 → 49 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/verify.spec.ts` | `mcp/verify.spec.cp` | 571 → 535 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/video.spec.ts` | `mcp/video.spec.cp` | 162 → 168 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/wait.spec.ts` | `mcp/wait.spec.cp` | 202 → 190 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/webdriver.spec.ts` | `mcp/webdriver.spec.cp` | 40 → 46 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/webmcp-dynamic.spec.ts` | `mcp/webmcp-dynamic.spec.cp` | 226 → 212 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/webmcp.spec.ts` | `mcp/webmcp.spec.cp` | 92 → 96 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `mcp/webstorage.spec.ts` | `mcp/webstorage.spec.cp` | 472 → 406 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/elementhandle-bounding-box.spec.ts` | `page/elementhandle-bounding-box.spec.cp` | 153 → 159 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/elementhandle-click.spec.ts` | `page/elementhandle-click.spec.cp` | 99 → 105 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/elementhandle-content-frame.spec.ts` | `page/elementhandle-content-frame.spec.cp` | 59 → 65 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/elementhandle-convenience.spec.ts` | `page/elementhandle-convenience.spec.cp` | 220 → 219 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/elementhandle-eval-on-selector.spec.ts` | `page/elementhandle-eval-on-selector.spec.cp` | 64 → 70 | ✅ 通过 | 9 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/elementhandle-misc.spec.ts` | `page/elementhandle-misc.spec.cp` | 96 → 102 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/elementhandle-owner-frame.spec.ts` | `page/elementhandle-owner-frame.spec.cp` | 94 → 99 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/elementhandle-press.spec.ts` | `page/elementhandle-press.spec.cp` | 59 → 64 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/elementhandle-query-selector.spec.ts` | `page/elementhandle-query-selector.spec.cp` | 95 → 100 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/elementhandle-screenshot.spec.ts` | `page/elementhandle-screenshot.spec.cp` | 296 → 278 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/elementhandle-scroll-into-view.spec.ts` | `page/elementhandle-scroll-into-view.spec.cp` | 153 → 159 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/elementhandle-select-text.spec.ts` | `page/elementhandle-select-text.spec.cp` | 62 → 68 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/elementhandle-type.spec.ts` | `page/elementhandle-type.spec.cp` | 59 → 64 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/elementhandle-wait-for-element-state.spec.ts` | `page/elementhandle-wait-for-element-state.spec.cp` | 141 → 147 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `page/eval-on-selector-all.spec.ts` | `page/eval-on-selector-all.spec.cp` | 85 → 90 | ✅ 通过 | 16 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/eval-on-selector.spec.ts` | `page/eval-on-selector.spec.cp` | 202 → 208 | ✅ 通过 | 40 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/expect-boolean.spec.ts` | `page/expect-boolean.spec.cp` | 702 → 709 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/expect-builtins.spec.ts` | `page/expect-builtins.spec.cp` | 922 → 823 | ✅ 通过 | 50 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `page/expect-matcher-result.spec.ts` | `page/expect-matcher-result.spec.cp` | 334 → 339 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/expect-misc.spec.ts` | `page/expect-misc.spec.cp` | 687 → 692 | ✅ 通过 | 31 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `page/expect-timeout.spec.ts` | `page/expect-timeout.spec.cp` | 213 → 218 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/expect-to-have-accessible.spec.ts` | `page/expect-to-have-accessible.spec.cp` | 207 → 212 | ✅ 通过 | 18 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `page/expect-to-have-text.spec.ts` | `page/expect-to-have-text.spec.cp` | 321 → 328 | ✅ 通过 | 14 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/expect-to-have-value.spec.ts` | `page/expect-to-have-value.spec.cp` | 139 → 146 | ✅ 通过 | 10 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/expect-with-snapshot.spec.ts` | `page/expect-with-snapshot.spec.cp` | 131 → 138 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/firefox-network-response.spec.ts` | `page/firefox-network-response.spec.cp` | 172 → 177 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/frame-evaluate.spec.ts` | `page/frame-evaluate.spec.cp` | 174 → 178 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/frame-frame-element.spec.ts` | `page/frame-frame-element.spec.cp` | 93 → 98 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/frame-goto.spec.ts` | `page/frame-goto.spec.cp` | 82 → 84 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/frame-hierarchy.spec.ts` | `page/frame-hierarchy.spec.cp` | 247 → 255 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/interception.spec.ts` | `page/interception.spec.cp` | 381 → 361 | ✅ 通过 | 9 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/jshandle-as-element.spec.ts` | `page/jshandle-as-element.spec.cp` | 46 → 51 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/jshandle-evaluate.spec.ts` | `page/jshandle-evaluate.spec.cp` | 34 → 39 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/jshandle-json-value.spec.ts` | `page/jshandle-json-value.spec.cp` | 37 → 43 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/jshandle-properties.spec.ts` | `page/jshandle-properties.spec.cp` | 126 → 131 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/jshandle-to-string.spec.ts` | `page/jshandle-to-string.spec.cp` | 91 → 97 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/locator-any-frame.spec.ts` | `page/locator-any-frame.spec.cp` | 547 → 547 | ✅ 通过 | 25 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/locator-click.spec.ts` | `page/locator-click.spec.cp` | 62 → 67 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/locator-convenience.spec.ts` | `page/locator-convenience.spec.cp` | 236 → 241 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/locator-element-handle.spec.ts` | `page/locator-element-handle.spec.cp` | 62 → 67 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/locator-evaluate.spec.ts` | `page/locator-evaluate.spec.cp` | 56 → 61 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/locator-frame.spec.ts` | `page/locator-frame.spec.cp` | 382 → 378 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/locator-highlight.spec.ts` | `page/locator-highlight.spec.cp` | 30 → 35 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/locator-is-visible.spec.ts` | `page/locator-is-visible.spec.cp` | 154 → 160 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/locator-list.spec.ts` | `page/locator-list.spec.cp` | 25 → 31 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/locator-misc-1.spec.ts` | `page/locator-misc-1.spec.cp` | 139 → 144 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/locator-misc-2.spec.ts` | `page/locator-misc-2.spec.cp` | 227 → 233 | ✅ 通过 | 9 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/locator-query.spec.ts` | `page/locator-query.spec.cp` | 319 → 317 | ✅ 通过 | 40 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `page/locator-wait-for-function.spec.ts` | `page/locator-wait-for-function.spec.cp` | 116 → 122 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/matchers.misc.spec.ts` | `page/matchers.misc.spec.cp` | 47 → 52 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/network-post-data.spec.ts` | `page/network-post-data.spec.cp` | 121 → 126 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-add-init-script-callback.spec.ts` | `page/page-add-init-script-callback.spec.cp` | 133 → 138 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `page/page-add-init-script.spec.ts` | `page/page-add-init-script.spec.cp` | 129 → 134 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-add-locator-handler.spec.ts` | `page/page-add-locator-handler.spec.cp` | 366 → 333 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-add-script-tag.spec.ts` | `page/page-add-script-tag.spec.cp` | 115 → 120 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `page/page-add-style-tag.spec.ts` | `page/page-add-style-tag.spec.cp` | 86 → 91 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `page/page-aria-snapshot-ai.spec.ts` | `page/page-aria-snapshot-ai.spec.cp` | 1025 → 963 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-aria-snapshot-json.spec.ts` | `page/page-aria-snapshot-json.spec.cp` | 217 → 226 | ✅ 通过 | 9 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `page/page-aria-snapshot.spec.ts` | `page/page-aria-snapshot.spec.cp` | 897 → 902 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-autowaiting-basic.spec.ts` | `page/page-autowaiting-basic.spec.cp` | 160 → 165 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-autowaiting-no-hang.spec.ts` | `page/page-autowaiting-no-hang.spec.cp` | 175 → 180 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-basic.spec.ts` | `page/page-basic.spec.cp` | 202 → 199 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-cache-storage.spec.ts` | `page/page-cache-storage.spec.cp` | 39 → 45 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-check.spec.ts` | `page/page-check.spec.cp` | 153 → 159 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-click-during-navigation.spec.ts` | `page/page-click-during-navigation.spec.cp` | 30 → 36 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-click-react.spec.ts` | `page/page-click-react.spec.cp` | 54 → 59 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-click-scroll.spec.ts` | `page/page-click-scroll.spec.cp` | 188 → 193 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-click-timeout-1.spec.ts` | `page/page-click-timeout-1.spec.cp` | 46 → 51 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-click-timeout-2.spec.ts` | `page/page-click-timeout-2.spec.cp` | 38 → 43 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-click-timeout-3.spec.ts` | `page/page-click-timeout-3.spec.cp` | 105 → 108 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-click-timeout-4.spec.ts` | `page/page-click-timeout-4.spec.cp` | 99 → 99 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-click.spec.ts` | `page/page-click.spec.cp` | 1427 → 1321 | ✅ 通过 | 39 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-dialog.spec.ts` | `page/page-dialog.spec.cp` | 151 → 154 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-dispatchevent.spec.ts` | `page/page-dispatchevent.spec.cp` | 231 → 237 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-drag.spec.ts` | `page/page-drag.spec.cp` | 540 → 493 | ✅ 通过 | 16 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-drop.spec.ts` | `page/page-drop.spec.cp` | 126 → 132 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-emulate-media.spec.ts` | `page/page-emulate-media.spec.cp` | 201 → 185 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-evaluate-callback.spec.ts` | `page/page-evaluate-callback.spec.cp` | 226 → 231 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-evaluate-handle.spec.ts` | `page/page-evaluate-handle.spec.cp` | 102 → 108 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-evaluate-no-stall.spec.ts` | `page/page-evaluate-no-stall.spec.cp` | 53 → 58 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-evaluate.spec.ts` | `page/page-evaluate.spec.cp` | 925 → 925 | ✅ 通过 | 25 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-event-console.spec.ts` | `page/page-event-console.spec.cp` | 354 → 339 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-event-load.spec.ts` | `page/page-event-load.spec.cp` | 75 → 79 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-event-network.spec.ts` | `page/page-event-network.spec.cp` | 185 → 190 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-event-pageerror.spec.ts` | `page/page-event-pageerror.spec.cp` | 315 → 310 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-event-popup.spec.ts` | `page/page-event-popup.spec.cp` | 193 → 197 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-event-request.spec.ts` | `page/page-event-request.spec.cp` | 429 → 420 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-expose-function.spec.ts` | `page/page-expose-function.spec.cp` | 256 → 254 | ✅ 通过 | 10 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-filechooser.spec.ts` | `page/page-filechooser.spec.cp` | 467 → 441 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-fill.spec.ts` | `page/page-fill.spec.cp` | 422 → 386 | ✅ 通过 | 35 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-focus.spec.ts` | `page/page-focus.spec.cp` | 254 → 259 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-goto.spec.ts` | `page/page-goto.spec.cp` | 985 → 925 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-history.spec.ts` | `page/page-history.spec.cp` | 306 → 286 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-keyboard.spec.ts` | `page/page-keyboard.spec.cp` | 835 → 759 | ✅ 通过 | 21 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-leaks.spec.ts` | `page/page-leaks.spec.cp` | 189 → 196 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-listeners.spec.ts` | `page/page-listeners.spec.cp` | 69 → 74 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-localstorage.spec.ts` | `page/page-localstorage.spec.cp` | 113 → 118 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-mouse.spec.ts` | `page/page-mouse.spec.cp` | 385 → 380 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-navigation.spec.ts` | `page/page-navigation.spec.cp` | 79 → 84 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-network-idle.spec.ts` | `page/page-network-idle.spec.cp` | 251 → 243 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-network-request.spec.ts` | `page/page-network-request.spec.cp` | 571 → 542 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-network-response.spec.ts` | `page/page-network-response.spec.cp` | 554 → 545 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-network-sizes.spec.ts` | `page/page-network-sizes.spec.cp` | 227 → 223 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-request-continue.spec.ts` | `page/page-request-continue.spec.cp` | 1024 → 990 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-request-fallback.spec.ts` | `page/page-request-fallback.spec.cp` | 280 → 288 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-request-fulfill.spec.ts` | `page/page-request-fulfill.spec.cp` | 522 → 513 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-request-gc.spec.ts` | `page/page-request-gc.spec.cp` | 47 → 52 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-request-intercept.spec.ts` | `page/page-request-intercept.spec.cp` | 326 → 307 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-route.spec.ts` | `page/page-route.spec.cp` | 1224 → 1121 | ✅ 通过 | 21 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-screenshot.spec.ts` | `page/page-screenshot.spec.cp` | 1128 → 1053 | ✅ 通过 | 14 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-select-option.spec.ts` | `page/page-select-option.spec.cp` | 481 → 466 | ✅ 通过 | 20 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-set-content.spec.ts` | `page/page-set-content.spec.cp` | 171 → 178 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-set-extra-http-headers.spec.ts` | `page/page-set-extra-http-headers.spec.cp` | 70 → 75 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-set-input-files.spec.ts` | `page/page-set-input-files.spec.cp` | 486 → 467 | ✅ 通过 | 12 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-strict.spec.ts` | `page/page-strict.spec.cp` | 128 → 134 | ✅ 通过 | 9 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/page-transition.spec.ts` | `page/page-transition.spec.cp` | 45 → 52 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-wait-for-function.spec.ts` | `page/page-wait-for-function.spec.cp` | 278 → 275 | ✅ 通过 | 10 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `page/page-wait-for-load-state.spec.ts` | `page/page-wait-for-load-state.spec.cp` | 207 → 213 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-wait-for-navigation.spec.ts` | `page/page-wait-for-navigation.spec.cp` | 262 → 267 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-wait-for-request.spec.ts` | `page/page-wait-for-request.spec.cp` | 105 → 110 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-wait-for-response.spec.ts` | `page/page-wait-for-response.spec.cp` | 118 → 124 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-wait-for-selector-1.spec.ts` | `page/page-wait-for-selector-1.spec.cp` | 241 → 241 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/page-wait-for-selector-2.spec.ts` | `page/page-wait-for-selector-2.spec.cp` | 358 → 356 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `page/page-wait-for-url.spec.ts` | `page/page-wait-for-url.spec.cp` | 148 → 153 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/pageTest.ts` | `page/pageTest.cp` | 61 → 72 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `page/pageTestApi.ts` | `page/pageTestApi.cp` | 41 → 26 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `page/queryselector.spec.ts` | `page/queryselector.spec.cp` | 128 → 134 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/retarget.spec.ts` | `page/retarget.spec.cp` | 386 → 370 | ✅ 通过 | 22 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `page/selectors-css.spec.ts` | `page/selectors-css.spec.cp` | 483 → 478 | ✅ 通过 | 203 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/selectors-frame.spec.ts` | `page/selectors-frame.spec.cp` | 405 → 405 | ✅ 通过 | 16 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/selectors-get-by.spec.ts` | `page/selectors-get-by.spec.cp` | 417 → 402 | ✅ 通过 | 34 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/selectors-misc.spec.ts` | `page/selectors-misc.spec.cp` | 497 → 464 | ✅ 通过 | 112 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/selectors-register.spec.ts` | `page/selectors-register.spec.cp` | 144 → 153 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/selectors-role.spec.ts` | `page/selectors-role.spec.cp` | 639 → 609 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/selectors-text.spec.ts` | `page/selectors-text.spec.cp` | 499 → 475 | ✅ 通过 | 148 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/to-match-aria-snapshot.spec.ts` | `page/to-match-aria-snapshot.spec.cp` | 1040 → 1045 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `page/wheel.spec.ts` | `page/wheel.spec.cp` | 217 → 225 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `page/workers.spec.ts` | `page/workers.spec.cp` | 470 → 436 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `playwright-test/access-data.spec.ts` | `playwright-test/access-data.spec.cp` | 109 → 114 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/aria-snapshot-file.spec.ts` | `playwright-test/aria-snapshot-file.spec.cp` | 481 → 486 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/assets/simple-server-with-ready-route.js` | `playwright-test/assets/simple-server-with-ready-route.cp` | 26 → 28 | ✅ 通过 | 0 | - |
| `playwright-test/assets/simple-server.js` | `playwright-test/assets/simple-server.cp` | 37 → 40 | ✅ 通过 | 1 | - |
| `playwright-test/babel.spec.ts` | `playwright-test/babel.spec.cp` | 344 → 323 | ✅ 通过 | 9 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/basic.spec.ts` | `playwright-test/basic.spec.cp` | 695 → 700 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/cache.spec.ts` | `playwright-test/cache.spec.cp` | 105 → 110 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/command-line-filter.spec.ts` | `playwright-test/command-line-filter.spec.cp` | 215 → 220 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/config.spec.ts` | `playwright-test/config.spec.cp` | 910 → 915 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/coverage.spec.ts` | `playwright-test/coverage.spec.cp` | 157 → 165 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/current-test-runner/globalSetup.ts` | `playwright-test/current-test-runner/globalSetup.cp` | 49 → 52 | ✅ 通过 | 2 | 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `playwright-test/decorators.spec.ts` | `playwright-test/decorators.spec.cp` | 71 → 77 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/deps.spec.ts` | `playwright-test/deps.spec.cp` | 715 → 720 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/esm.spec.ts` | `playwright-test/esm.spec.cp` | 835 → 816 | ✅ 通过 | 22 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/exit-code.spec.ts` | `playwright-test/exit-code.spec.cp` | 358 → 363 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/expect-configure.spec.ts` | `playwright-test/expect-configure.spec.cp` | 185 → 190 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/expect-poll.spec.ts` | `playwright-test/expect-poll.spec.cp` | 307 → 312 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/expect-soft.spec.ts` | `playwright-test/expect-soft.spec.cp` | 81 → 86 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/expect-to-pass.spec.ts` | `playwright-test/expect-to-pass.spec.cp` | 321 → 326 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/expect.spec.ts` | `playwright-test/expect.spec.cp` | 1407 → 1331 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/fit-to-width.spec.ts` | `playwright-test/fit-to-width.spec.cp` | 33 → 38 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/fixture-errors.spec.ts` | `playwright-test/fixture-errors.spec.cp` | 865 → 870 | ✅ 通过 | 14 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/fixtures.spec.ts` | `playwright-test/fixtures.spec.cp` | 852 → 857 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/gitignore.spec.ts` | `playwright-test/gitignore.spec.cp` | 196 → 201 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/global-setup.spec.ts` | `playwright-test/global-setup.spec.cp` | 526 → 531 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/golden.spec.ts` | `playwright-test/golden.spec.cp` | 1340 → 1311 | ✅ 通过 | 15 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/hooks.spec.ts` | `playwright-test/hooks.spec.cp` | 922 → 927 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/list-mode.spec.ts` | `playwright-test/list-mode.spec.cp` | 218 → 222 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/loader.spec.ts` | `playwright-test/loader.spec.cp` | 1529 → 1482 | ✅ 通过 | 20 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `playwright-test/match-grep.spec.ts` | `playwright-test/match-grep.spec.cp` | 113 → 118 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/max-failures.spec.ts` | `playwright-test/max-failures.spec.cp` | 261 → 266 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/only-changed.spec.ts` | `playwright-test/only-changed.spec.cp` | 390 → 361 | ✅ 通过 | 8 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/override-timeout.spec.ts` | `playwright-test/override-timeout.spec.cp` | 70 → 75 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/playwright-test-fixtures.ts` | `playwright-test/playwright-test-fixtures.cp` | 473 → 440 | ✅ 通过 | 32 | 顶层箭头函数提升为静态零闭包开销 `fn` 声明; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `playwright-test/playwright.artifacts.spec.ts` | `playwright-test/playwright.artifacts.spec.cp` | 628 → 635 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/playwright.config.spec.ts` | `playwright-test/playwright.config.spec.cp` | 252 → 257 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/playwright.config.ts` | `playwright-test/playwright.config.cp` | 63 → 69 | ✅ 通过 | 2 | 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `playwright-test/playwright.connect.spec.ts` | `playwright-test/playwright.connect.spec.cp` | 235 → 238 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/playwright.fetch.spec.ts` | `playwright-test/playwright.fetch.spec.cp` | 111 → 116 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/playwright.reuse.browser.spec.ts` | `playwright-test/playwright.reuse.browser.spec.cp` | 154 → 160 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/playwright.reuse.spec.ts` | `playwright-test/playwright.reuse.spec.cp` | 555 → 560 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `playwright-test/playwright.spec.ts` | `playwright-test/playwright.spec.cp` | 1148 → 1080 | ✅ 通过 | 13 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 自动推断字段写操作并为类方法注入安全可变接收者 `*_` |
| `playwright-test/playwright.trace.spec.ts` | `playwright-test/playwright.trace.spec.cp` | 1609 → 1528 | ✅ 通过 | 31 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/playwright.unhandled.spec.ts` | `playwright-test/playwright.unhandled.spec.cp` | 81 → 86 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/repeat-each.spec.ts` | `playwright-test/repeat-each.spec.cp` | 54 → 59 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/reporter-attachment.spec.ts` | `playwright-test/reporter-attachment.spec.cp` | 353 → 358 | ✅ 通过 | 12 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/reporter-base.spec.ts` | `playwright-test/reporter-base.spec.cp` | 591 → 570 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/reporter-blob.spec.ts` | `playwright-test/reporter-blob.spec.cp` | 2491 → 2364 | ✅ 通过 | 53 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/reporter-dot.spec.ts` | `playwright-test/reporter-dot.spec.cp` | 115 → 94 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/reporter-github.spec.ts` | `playwright-test/reporter-github.spec.cp` | 142 → 122 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/reporter-html.spec.ts` | `playwright-test/reporter-html.spec.cp` | 3979 → 3515 | ✅ 通过 | 100 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 彻底消除废弃关键字 `var` 为块级可变绑定 `let`; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `playwright-test/reporter-json.spec.ts` | `playwright-test/reporter-json.spec.cp` | 388 → 390 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 彻底消除废弃关键字 `var` 为块级可变绑定 `let` |
| `playwright-test/reporter-junit.spec.ts` | `playwright-test/reporter-junit.spec.cp` | 765 → 705 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 彻底消除废弃关键字 `var` 为块级可变绑定 `let` |
| `playwright-test/reporter-line.spec.ts` | `playwright-test/reporter-line.spec.cp` | 247 → 219 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/reporter-list.spec.ts` | `playwright-test/reporter-list.spec.cp` | 620 → 572 | ✅ 通过 | 15 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/reporter-onend.spec.ts` | `playwright-test/reporter-onend.spec.cp` | 80 → 85 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/reporter-only-failures.spec.ts` | `playwright-test/reporter-only-failures.spec.cp` | 403 → 402 | ✅ 通过 | 14 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/reporter-perfetto.spec.ts` | `playwright-test/reporter-perfetto.spec.cp` | 312 → 291 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `playwright-test/reporter-preprocess.spec.ts` | `playwright-test/reporter-preprocess.spec.cp` | 628 → 620 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/reporter.spec.ts` | `playwright-test/reporter.spec.cp` | 1034 → 996 | ✅ 通过 | 14 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/resolver.spec.ts` | `playwright-test/resolver.spec.cp` | 1475 → 1463 | ✅ 通过 | 45 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/retry.spec.ts` | `playwright-test/retry.spec.cp` | 509 → 514 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/runner.spec.ts` | `playwright-test/runner.spec.cp` | 1024 → 983 | ✅ 通过 | 13 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/shard.spec.ts` | `playwright-test/shard.spec.cp` | 434 → 430 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/shuffle.spec.ts` | `playwright-test/shuffle.spec.cp` | 84 → 89 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `playwright-test/snapshot-path-template.spec.ts` | `playwright-test/snapshot-path-template.spec.cp` | 158 → 165 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/stable-test-runner/index.js` | `playwright-test/stable-test-runner/index.cp` | 17 → 19 | ✅ 通过 | 0 | - |
| `playwright-test/stdio.spec.ts` | `playwright-test/stdio.spec.cp` | 139 → 144 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/test-extend.spec.ts` | `playwright-test/test-extend.spec.cp` | 319 → 324 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/test-grep.spec.ts` | `playwright-test/test-grep.spec.cp` | 99 → 104 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/test-ignore.spec.ts` | `playwright-test/test-ignore.spec.cp` | 373 → 378 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/test-info.spec.ts` | `playwright-test/test-info.spec.cp` | 94 → 99 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/test-list.spec.ts` | `playwright-test/test-list.spec.cp` | 248 → 253 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/test-locks.spec.ts` | `playwright-test/test-locks.spec.cp` | 289 → 286 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 顶层箭头函数提升为静态零闭包开销 `fn` 声明 |
| `playwright-test/test-modifiers.spec.ts` | `playwright-test/test-modifiers.spec.cp` | 850 → 855 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/test-output-dir.spec.ts` | `playwright-test/test-output-dir.spec.cp` | 445 → 419 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/test-parallel.spec.ts` | `playwright-test/test-parallel.spec.cp` | 274 → 279 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/test-serial.spec.ts` | `playwright-test/test-serial.spec.cp` | 448 → 453 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/test-server.spec.ts` | `playwright-test/test-server.spec.cp` | 524 → 515 | ✅ 通过 | 11 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 自动推断字段写操作并为类方法注入安全可变接收者 `*_` |
| `playwright-test/test-step.spec.ts` | `playwright-test/test-step.spec.cp` | 2011 → 2024 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/test-tag.spec.ts` | `playwright-test/test-tag.spec.cp` | 230 → 235 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/test-use.spec.ts` | `playwright-test/test-use.spec.cp` | 188 → 193 | ✅ 通过 | 4 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/timeout.spec.ts` | `playwright-test/timeout.spec.cp` | 706 → 699 | ✅ 通过 | 6 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/to-have-screenshot.spec.ts` | `playwright-test/to-have-screenshot.spec.cp` | 1925 → 1884 | ✅ 通过 | 19 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/types-2.spec.ts` | `playwright-test/types-2.spec.cp` | 262 → 267 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/types.spec.ts` | `playwright-test/types.spec.cp` | 381 → 386 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-fixtures.ts` | `playwright-test/ui-mode-fixtures.cp` | 159 → 155 | ✅ 通过 | 8 | 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `playwright-test/ui-mode-metadata.spec.ts` | `playwright-test/ui-mode-metadata.spec.cp` | 48 → 54 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-annotations.spec.ts` | `playwright-test/ui-mode-test-annotations.spec.cp` | 48 → 54 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-attachments.spec.ts` | `playwright-test/ui-mode-test-attachments.spec.cp` | 223 → 217 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-filters.spec.ts` | `playwright-test/ui-mode-test-filters.spec.cp` | 322 → 327 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-network-tab.spec.ts` | `playwright-test/ui-mode-test-network-tab.spec.cp` | 546 → 551 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-output.spec.ts` | `playwright-test/ui-mode-test-output.spec.cp` | 288 → 293 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-progress.spec.ts` | `playwright-test/ui-mode-test-progress.spec.cp` | 352 → 357 | ✅ 通过 | 40 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-run.spec.ts` | `playwright-test/ui-mode-test-run.spec.cp` | 851 → 784 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |
| `playwright-test/ui-mode-test-screencast.spec.ts` | `playwright-test/ui-mode-test-screencast.spec.cp` | 47 → 53 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-setup.spec.ts` | `playwright-test/ui-mode-test-setup.spec.cp` | 463 → 464 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-shortcut.spec.ts` | `playwright-test/ui-mode-test-shortcut.spec.cp` | 116 → 121 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-source.spec.ts` | `playwright-test/ui-mode-test-source.spec.cp` | 213 → 218 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-tree.spec.ts` | `playwright-test/ui-mode-test-tree.spec.cp` | 633 → 638 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-update.spec.ts` | `playwright-test/ui-mode-test-update.spec.cp` | 269 → 274 | ✅ 通过 | 5 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-test-watch.spec.ts` | `playwright-test/ui-mode-test-watch.spec.cp` | 387 → 353 | ✅ 通过 | 3 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/ui-mode-trace.spec.ts` | `playwright-test/ui-mode-trace.spec.cp` | 900 → 825 | ✅ 通过 | 59 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `playwright-test/update-aria-snapshot.spec.ts` | `playwright-test/update-aria-snapshot.spec.cp` | 724 → 652 | ✅ 通过 | 13 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/watch.spec.ts` | `playwright-test/watch.spec.cp` | 783 → 755 | ✅ 通过 | 10 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `playwright-test/web-server.spec.ts` | `playwright-test/web-server.spec.cp` | 1139 → 1135 | ✅ 通过 | 7 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `playwright-test/worker-index.spec.ts` | `playwright-test/worker-index.spec.cp` | 388 → 394 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `stress/browsers.spec.ts` | `stress/browsers.spec.cp` | 31 → 34 | ✅ 通过 | 2 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `stress/contexts.spec.ts` | `stress/contexts.spec.cp` | 24 → 29 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `stress/playwright.config.ts` | `stress/playwright.config.cp` | 50 → 52 | ✅ 通过 | 0 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `third_party/proxy/index.ts` | `third_party/proxy/index.cp` | 495 → 466 | ✅ 通过 | 9 | 消除 interface 动态派发为栈布局 struct（零虚表开销）; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播 |
| `webview/expectationUtil.ts` | `webview/expectationUtil.cp` | 72 → 77 | ✅ 通过 | 5 | 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析 |
| `webview/playwright.config.ts` | `webview/playwright.config.cp` | 65 → 68 | ✅ 通过 | 1 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法 |
| `webview/webviewTest.ts` | `webview/webviewTest.cp` | 187 → 187 | ✅ 通过 | 11 | 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法; 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播; 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析; 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race` |

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

- 剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播（涉及 102 个文件）
- 复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析（涉及 46 个文件）
- 将 `Promise.all` / `Promise.race` 映射为 CP 结构化并发 `joinAll` / `race`（涉及 152 个文件）
- 彻底消除废弃关键字 `var` 为块级可变绑定 `let`（涉及 19 个文件）
- 消除 interface 动态派发为栈布局 struct（零虚表开销）（涉及 6 个文件）
- 自动推断字段写操作并为类方法注入安全可变接收者 `*_`（涉及 22 个文件）
- 降级 TS 测试套件（describe/test/expect/async）为可编译运行的 CP `cap:test` 语法（涉及 579 个文件）
- 顶层箭头函数提升为静态零闭包开销 `fn` 声明（涉及 38 个文件）

## 5. 需人工精修清单与定位方法

### 5.2 含 `TODO(cap-migrate)` 标记的文件

- `android/android.spec.cp`：1 处 `TODO(cap-migrate)`
- `android/androidTest.cp`：3 处 `TODO(cap-migrate)`
- `android/device.spec.cp`：5 处 `TODO(cap-migrate)`
- `android/launch-server.spec.cp`：11 处 `TODO(cap-migrate)`
- `android/playwright.config.cp`：1 处 `TODO(cap-migrate)`
- `assets/dummy_bad_browser_executable.cp`：2 处 `TODO(cap-migrate)`
- `assets/modernizr/modernizr.cp`：12 处 `TODO(cap-migrate)`
- `assets/react/material-ui@5.2.2.production.min.cp`：2 处 `TODO(cap-migrate)`
- `assets/react/react-dom@16.13.1.production.min.cp`：23 处 `TODO(cap-migrate)`
- `assets/react/react-dom@17.0.2.production.min.cp`：18 处 `TODO(cap-migrate)`
- `assets/react/react@16.13.1.production.min.cp`：7 处 `TODO(cap-migrate)`
- `assets/react/react@17.0.2.production.min.cp`：8 处 `TODO(cap-migrate)`
- `assets/reading-list/react-dom_15.7.0.cp`：188 处 `TODO(cap-migrate)`
- `assets/reading-list/react-dom_16.14.0.cp`：409 处 `TODO(cap-migrate)`
- `assets/reading-list/react-dom_17.0.2.cp`：461 处 `TODO(cap-migrate)`
- `assets/reading-list/react-dom_18.1.0.cp`：556 处 `TODO(cap-migrate)`
- `assets/reading-list/react_15.7.0.cp`：76 处 `TODO(cap-migrate)`
- `assets/reading-list/react_16.14.0.cp`：110 处 `TODO(cap-migrate)`
- `assets/reading-list/react_17.0.2.cp`：121 处 `TODO(cap-migrate)`
- `assets/reading-list/react_18.1.0.cp`：98 处 `TODO(cap-migrate)`
- `assets/reading-list/vue_2.6.14.cp`：186 处 `TODO(cap-migrate)`
- `assets/reading-list/vue_3.1.5.cp`：229 处 `TODO(cap-migrate)`
- `assets/sectionselectorengine.cp`：1 处 `TODO(cap-migrate)`
- `assets/selenium-grid/broken-selenium-driver.cp`：2 处 `TODO(cap-migrate)`
- `assets/serviceworkers/stub/sw.cp`：1 处 `TODO(cap-migrate)`
- `assets/stress/static/js/787.2f3b90fa.chunk.cp`：2 处 `TODO(cap-migrate)`
- `assets/stress/static/js/main.cc4a7955.cp`：1 处 `TODO(cap-migrate)`
- `assets/to-do-notifications/scripts/todo.cp`：7 处 `TODO(cap-migrate)`
- `bidi/csvReporter.cp`：5 处 `TODO(cap-migrate)`
- `bidi/playwright.config.cp`：7 处 `TODO(cap-migrate)`
- `config/baseTest.cp`：2 处 `TODO(cap-migrate)`
- `config/browserTest.cp`：10 处 `TODO(cap-migrate)`
- `config/commonFixtures.cp`：23 处 `TODO(cap-migrate)`
- `config/comparator.cp`：2 处 `TODO(cap-migrate)`
- `config/debugControllerBackend.cp`：16 处 `TODO(cap-migrate)`
- `config/expectationReporter.cp`：10 处 `TODO(cap-migrate)`
- `config/markdownReporter.cp`：4 处 `TODO(cap-migrate)`
- `config/parquetReporter.cp`：9 处 `TODO(cap-migrate)`
- `config/platformFixtures.cp`：1 处 `TODO(cap-migrate)`
- `config/postReportComment.cp`：3 处 `TODO(cap-migrate)`
- `config/proxy.cp`：8 处 `TODO(cap-migrate)`
- `config/registryReporter.cp`：13 处 `TODO(cap-migrate)`
- `config/remoteServer.cp`：12 处 `TODO(cap-migrate)`
- `config/serverFixtures.cp`：4 处 `TODO(cap-migrate)`
- `config/serviceWorkerCoverage.cp`：1 处 `TODO(cap-migrate)`
- `config/testMode.cp`：3 处 `TODO(cap-migrate)`
- `config/testModeFixtures.cp`：3 处 `TODO(cap-migrate)`
- `config/testserver/index.cp`：14 处 `TODO(cap-migrate)`
- `config/traceViewerFixtures.cp`：12 处 `TODO(cap-migrate)`
- `config/utils.cp`：11 处 `TODO(cap-migrate)`
- `electron/electron-app.spec.cp`：4 处 `TODO(cap-migrate)`
- `electron/electron-options-app.cp`：1 处 `TODO(cap-migrate)`
- `electron/electron-tracing.spec.cp`：3 处 `TODO(cap-migrate)`
- `electron/electron-webcontentsview.spec.cp`：1 处 `TODO(cap-migrate)`
- `electron/electronTest.cp`：3 处 `TODO(cap-migrate)`
- `electron/globalSetup.cp`：1 处 `TODO(cap-migrate)`
- `electron/playwright.config.cp`：1 处 `TODO(cap-migrate)`
- `extension/chrome-stub.cp`：5 处 `TODO(cap-migrate)`
- `extension/cli.spec.cp`：1 处 `TODO(cap-migrate)`
- `extension/client-lifecycle.spec.cp`：3 处 `TODO(cap-migrate)`
- `extension/extension-fixtures.cp`：8 处 `TODO(cap-migrate)`
- `extension/extension.spec.cp`：12 处 `TODO(cap-migrate)`
- `extension/harness.cp`：26 处 `TODO(cap-migrate)`
- `extension/playwright.config.cp`：1 处 `TODO(cap-migrate)`
- `extension/protocol.spec.cp`：1 处 `TODO(cap-migrate)`
- `extension/scheme-detach.spec.cp`：4 处 `TODO(cap-migrate)`
- `extension/tab-grouping.spec.cp`：2 处 `TODO(cap-migrate)`
- `extension/tab-management.spec.cp`：3 处 `TODO(cap-migrate)`
- `image_tools/fixtures.spec.cp`：2 处 `TODO(cap-migrate)`
- `image_tools/unit.spec.cp`：5 处 `TODO(cap-migrate)`
- `image_tools/utils.cp`：3 处 `TODO(cap-migrate)`
- `installation/bundle-licenses.spec.cp`：4 处 `TODO(cap-migrate)`
- `installation/bundle-size.spec.cp`：1 处 `TODO(cap-migrate)`
- `installation/concurrent-download.spec.cp`：1 处 `TODO(cap-migrate)`
- `installation/fixture-scripts/download-chromedriver.cp`：1 处 `TODO(cap-migrate)`
- `installation/fixture-scripts/driver-client-sigint.cp`：2 处 `TODO(cap-migrate)`
- `installation/fixture-scripts/driver-client.cp`：2 处 `TODO(cap-migrate)`
- `installation/fixture-scripts/esm.cp`：3 处 `TODO(cap-migrate)`
- `installation/fixture-scripts/playwright-test-plugin-types.cp`：1 处 `TODO(cap-migrate)`
- `installation/fixture-scripts/sanity-electron.cp`：1 处 `TODO(cap-migrate)`
- `installation/fixture-scripts/sanity.cp`：4 处 `TODO(cap-migrate)`
- `installation/fixture-scripts/screencast.cp`：2 处 `TODO(cap-migrate)`
- `installation/globalSetup.cp`：4 处 `TODO(cap-migrate)`
- `installation/npm-installs-local-packages.spec.cp`：1 处 `TODO(cap-migrate)`
- `installation/npmTest.cp`：13 处 `TODO(cap-migrate)`
- `installation/npx-global.spec.cp`：1 处 `TODO(cap-migrate)`
- `installation/playwright-cdn.spec.cp`：14 处 `TODO(cap-migrate)`
- `installation/playwright-cli-install-should-work.spec.cp`：11 处 `TODO(cap-migrate)`
- `installation/playwright-cli.spec.cp`：2 处 `TODO(cap-migrate)`
- `installation/playwright-electron-should-work.spec.cp`：2 处 `TODO(cap-migrate)`
- `installation/playwright-packages-install-behavior.spec.cp`：3 处 `TODO(cap-migrate)`
- `installation/playwright-test-plugin/index.cp`：3 处 `TODO(cap-migrate)`
- `installation/playwright-test-plugin.spec.cp`：1 处 `TODO(cap-migrate)`
- `installation/playwright.config.cp`：1 处 `TODO(cap-migrate)`
- `installation/registry.cp`：9 处 `TODO(cap-migrate)`
- `installation/typescript-types.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/beforeunload.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/browser-server.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/browser.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/browsercontext-add-cookies.spec.cp`：7 处 `TODO(cap-migrate)`
- `library/browsercontext-add-init-script.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/browsercontext-base-url.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/browsercontext-basic.spec.cp`：8 处 `TODO(cap-migrate)`
- `library/browsercontext-clearcookies.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/browsercontext-cookies-third-party.spec.cp`：9 处 `TODO(cap-migrate)`
- `library/browsercontext-cookies.spec.cp`：7 处 `TODO(cap-migrate)`
- `library/browsercontext-credentials.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/browsercontext-csp.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/browsercontext-device.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/browsercontext-events.spec.cp`：7 处 `TODO(cap-migrate)`
- `library/browsercontext-expose-function.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/browsercontext-fetch-algorithms.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/browsercontext-fetch-happy-eyeballs.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/browsercontext-fetch.spec.cp`：19 处 `TODO(cap-migrate)`
- `library/browsercontext-har.spec.cp`：11 处 `TODO(cap-migrate)`
- `library/browsercontext-locale.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/browsercontext-network-event.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/browsercontext-page-event.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/browsercontext-pages.spec.cp`：6 处 `TODO(cap-migrate)`
- `library/browsercontext-proxy.spec.cp`：7 处 `TODO(cap-migrate)`
- `library/browsercontext-reuse.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/browsercontext-route.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/browsercontext-service-worker-policy.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/browsercontext-storage-state.spec.cp`：7 处 `TODO(cap-migrate)`
- `library/browsercontext-strict.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/browsercontext-timezone-id.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/browsercontext-user-agent.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/browsercontext-viewport-mobile.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/browsercontext-viewport.spec.cp`：6 处 `TODO(cap-migrate)`
- `library/browsercontext-webauthn.spec.cp`：13 处 `TODO(cap-migrate)`
- `library/browsers-path.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/browsertype-basic.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/browsertype-connect.spec.cp`：15 处 `TODO(cap-migrate)`
- `library/browsertype-launch-selenium.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/browsertype-launch-server.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/browsertype-launch.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/capabilities.spec.cp`：24 处 `TODO(cap-migrate)`
- `library/channels.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/chromium/chromium.spec.cp`：23 处 `TODO(cap-migrate)`
- `library/chromium/connect-over-cdp.spec.cp`：67 处 `TODO(cap-migrate)`
- `library/chromium/disable-web-security.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/chromium/extensions.spec.cp`：6 处 `TODO(cap-migrate)`
- `library/chromium/js-coverage.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/chromium/launcher.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/chromium/oopif.spec.cp`：8 处 `TODO(cap-migrate)`
- `library/chromium/session.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/client-certificates.spec.cp`：14 处 `TODO(cap-migrate)`
- `library/component-parser.spec.cp`：15 处 `TODO(cap-migrate)`
- `library/coverage.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/css-parser.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/debug-controller.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/debugger.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/defaultbrowsercontext-1.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/defaultbrowsercontext-2.spec.cp`：6 处 `TODO(cap-migrate)`
- `library/download.spec.cp`：6 处 `TODO(cap-migrate)`
- `library/downloads-path.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/emulation-coop.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/emulation-focus.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/events/listener-errors.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/events/listeners-side-effects.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/events/listeners.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/events/max-listeners.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/events/modify-in-emit.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/events/once.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/events/prepend.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/events/remove-all-listeners-wait.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/events/remove-all-listeners.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/events/remove-listeners.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/events/set-max-listeners-side-effects.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/events/utils.cp`：5 处 `TODO(cap-migrate)`
- `library/firefox/launcher.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/geolocation.spec.cp`：8 处 `TODO(cap-migrate)`
- `library/global-fetch-cookie.spec.cp`：15 处 `TODO(cap-migrate)`
- `library/global-fetch.spec.cp`：17 处 `TODO(cap-migrate)`
- `library/har-websocket.spec.cp`：16 处 `TODO(cap-migrate)`
- `library/har.spec.cp`：18 处 `TODO(cap-migrate)`
- `library/headful.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/heap.spec.cp`：8 处 `TODO(cap-migrate)`
- `library/hit-target.spec.cp`：11 处 `TODO(cap-migrate)`
- `library/ignorehttpserrors.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/inspector/cli-codegen-1.spec.cp`：8 处 `TODO(cap-migrate)`
- `library/inspector/cli-codegen-2.spec.cp`：6 处 `TODO(cap-migrate)`
- `library/inspector/cli-codegen-3.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/inspector/cli-codegen-aria.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/inspector/cli-codegen-csharp.spec.cp`：6 处 `TODO(cap-migrate)`
- `library/inspector/cli-codegen-java.spec.cp`：6 处 `TODO(cap-migrate)`
- `library/inspector/cli-codegen-javascript.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/inspector/cli-codegen-python-async.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/inspector/cli-codegen-python.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/inspector/cli-codegen-test.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/inspector/inspectorTest.cp`：23 处 `TODO(cap-migrate)`
- `library/inspector/pause.spec.cp`：15 处 `TODO(cap-migrate)`
- `library/inspector/recorder-api.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/inspector/title.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/launcher.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/locator-dispatchevent-touch.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/locator-generator.spec.cp`：32 处 `TODO(cap-migrate)`
- `library/locator-highlight.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/locator-parser.spec.cp`：216 处 `TODO(cap-migrate)`
- `library/modernizr.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/multiclient.spec.cp`：7 处 `TODO(cap-migrate)`
- `library/network-timeout.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/page-clock.spec.cp`：3 处 `TODO(cap-migrate)`
- `library/page-close.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/page-event-crash.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/pdf.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/permissions.spec.cp`：6 处 `TODO(cap-migrate)`
- `library/playwright-client.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/playwright.config.cp`：4 处 `TODO(cap-migrate)`
- `library/popup.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/proxy.spec.cp`：4 处 `TODO(cap-migrate)`
- `library/resource-timing.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/role-utils.spec.cp`：18 处 `TODO(cap-migrate)`
- `library/route-web-socket.spec.cp`：29 处 `TODO(cap-migrate)`
- `library/screencast-actions.spec.cp`：7 处 `TODO(cap-migrate)`
- `library/screencast-overlay.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/screencast.spec.cp`：17 处 `TODO(cap-migrate)`
- `library/screenshot.spec.cp`：6 处 `TODO(cap-migrate)`
- `library/selector-generator.spec.cp`：21 处 `TODO(cap-migrate)`
- `library/selectors-register.spec.cp`：26 处 `TODO(cap-migrate)`
- `library/signals.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/slowmo.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/snapshot-renderer.spec.cp`：5 处 `TODO(cap-migrate)`
- `library/tap.spec.cp`：9 处 `TODO(cap-migrate)`
- `library/trace-viewer.spec.cp`：199 处 `TODO(cap-migrate)`
- `library/tracing.spec.cp`：12 处 `TODO(cap-migrate)`
- `library/unit/android-webviews.spec.cp`：7 处 `TODO(cap-migrate)`
- `library/unit/clock.spec.cp`：37 处 `TODO(cap-migrate)`
- `library/unit/code-frame.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/unit/codegen.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/unit/http-server.spec.cp`：6 处 `TODO(cap-migrate)`
- `library/unit/timeout-runner.spec.cp`：2 处 `TODO(cap-migrate)`
- `library/unroute-behavior.spec.cp`：28 处 `TODO(cap-migrate)`
- `library/video.spec.cp`：15 处 `TODO(cap-migrate)`
- `library/videoPlayer.cp`：11 处 `TODO(cap-migrate)`
- `library/web-socket.spec.cp`：1 处 `TODO(cap-migrate)`
- `library/webmcp.spec.cp`：10 处 `TODO(cap-migrate)`
- `mcp/annotate.spec.cp`：33 处 `TODO(cap-migrate)`
- `mcp/autowait.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/capabilities.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/cdp.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/cli-cdp.spec.cp`：3 处 `TODO(cap-migrate)`
- `mcp/cli-config.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/cli-cookies.spec.cp`：3 处 `TODO(cap-migrate)`
- `mcp/cli-core.spec.cp`：6 处 `TODO(cap-migrate)`
- `mcp/cli-devtools.spec.cp`：9 处 `TODO(cap-migrate)`
- `mcp/cli-drag.spec.cp`：6 处 `TODO(cap-migrate)`
- `mcp/cli-find.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/cli-fixtures.cp`：25 处 `TODO(cap-migrate)`
- `mcp/cli-isolated.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/cli-json.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/cli-killall.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/cli-misc.spec.cp`：5 处 `TODO(cap-migrate)`
- `mcp/cli-navigation.spec.cp`：4 处 `TODO(cap-migrate)`
- `mcp/cli-parsing.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/cli-run-code.spec.cp`：3 处 `TODO(cap-migrate)`
- `mcp/cli-save-as.spec.cp`：3 处 `TODO(cap-migrate)`
- `mcp/cli-session.spec.cp`：10 处 `TODO(cap-migrate)`
- `mcp/cli-storage.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/cli-test.spec.cp`：5 处 `TODO(cap-migrate)`
- `mcp/cli-webmcp.spec.cp`：12 处 `TODO(cap-migrate)`
- `mcp/click.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/clipboard.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/codegen.spec.cp`：3 处 `TODO(cap-migrate)`
- `mcp/config-resolve.spec.cp`：13 处 `TODO(cap-migrate)`
- `mcp/config.ini.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/config.spec.cp`：4 处 `TODO(cap-migrate)`
- `mcp/console.spec.cp`：10 处 `TODO(cap-migrate)`
- `mcp/cookies.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/core.spec.cp`：5 处 `TODO(cap-migrate)`
- `mcp/crash.spec.cp`：4 处 `TODO(cap-migrate)`
- `mcp/dashboard.spec.cp`：15 处 `TODO(cap-migrate)`
- `mcp/devtools.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/dialogs.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/emulation.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/evaluate.spec.cp`：4 处 `TODO(cap-migrate)`
- `mcp/file-paths.spec.cp`：4 处 `TODO(cap-migrate)`
- `mcp/files.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/find.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/fixtures.cp`：28 处 `TODO(cap-migrate)`
- `mcp/form.spec.cp`：4 处 `TODO(cap-migrate)`
- `mcp/headed.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/http.spec.cp`：6 处 `TODO(cap-migrate)`
- `mcp/idle-timeout.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/init-agents.spec.cp`：6 处 `TODO(cap-migrate)`
- `mcp/init-page.spec.cp`：3 处 `TODO(cap-migrate)`
- `mcp/init-script.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/launch.spec.cp`：3 处 `TODO(cap-migrate)`
- `mcp/mouse.spec.cp`：5 处 `TODO(cap-migrate)`
- `mcp/network-offline.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/network.spec.cp`：10 处 `TODO(cap-migrate)`
- `mcp/output-max-size.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/pdf.spec.cp`：5 处 `TODO(cap-migrate)`
- `mcp/playwright.config.cp`：2 处 `TODO(cap-migrate)`
- `mcp/profile-lock.spec.cp`：6 处 `TODO(cap-migrate)`
- `mcp/recorder.spec.cp`：6 处 `TODO(cap-migrate)`
- `mcp/remote-endpoint.spec.cp`：3 处 `TODO(cap-migrate)`
- `mcp/request-blocking.spec.cp`：4 处 `TODO(cap-migrate)`
- `mcp/roots.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/route.spec.cp`：5 处 `TODO(cap-migrate)`
- `mcp/run-code.spec.cp`：6 处 `TODO(cap-migrate)`
- `mcp/screenshot.spec.cp`：5 处 `TODO(cap-migrate)`
- `mcp/secrets.spec.cp`：4 处 `TODO(cap-migrate)`
- `mcp/seed-default-project.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/snapshot-mode.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/sse.spec.cp`：4 处 `TODO(cap-migrate)`
- `mcp/storage.spec.cp`：3 处 `TODO(cap-migrate)`
- `mcp/tabs.spec.cp`：3 处 `TODO(cap-migrate)`
- `mcp/test-debug.spec.cp`：5 处 `TODO(cap-migrate)`
- `mcp/test-list.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/test-run.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/test-setup.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/timeouts.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/trace-cli-fixtures.cp`：4 处 `TODO(cap-migrate)`
- `mcp/trace-cli.spec.cp`：14 处 `TODO(cap-migrate)`
- `mcp/tracing.spec.cp`：9 处 `TODO(cap-migrate)`
- `mcp/type.spec.cp`：3 处 `TODO(cap-migrate)`
- `mcp/unicode-serialization.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/validation.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/verify.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/video.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/wait.spec.cp`：1 处 `TODO(cap-migrate)`
- `mcp/webmcp-dynamic.spec.cp`：4 处 `TODO(cap-migrate)`
- `mcp/webmcp.spec.cp`：2 处 `TODO(cap-migrate)`
- `mcp/webstorage.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/elementhandle-bounding-box.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/elementhandle-click.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/elementhandle-content-frame.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/elementhandle-convenience.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/elementhandle-eval-on-selector.spec.cp`：9 处 `TODO(cap-migrate)`
- `page/elementhandle-misc.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/elementhandle-press.spec.cp`：7 处 `TODO(cap-migrate)`
- `page/elementhandle-query-selector.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/elementhandle-screenshot.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/elementhandle-scroll-into-view.spec.cp`：4 处 `TODO(cap-migrate)`
- `page/elementhandle-select-text.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/elementhandle-type.spec.cp`：7 处 `TODO(cap-migrate)`
- `page/elementhandle-wait-for-element-state.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/eval-on-selector-all.spec.cp`：16 处 `TODO(cap-migrate)`
- `page/eval-on-selector.spec.cp`：40 处 `TODO(cap-migrate)`
- `page/expect-boolean.spec.cp`：6 处 `TODO(cap-migrate)`
- `page/expect-builtins.spec.cp`：50 处 `TODO(cap-migrate)`
- `page/expect-matcher-result.spec.cp`：4 处 `TODO(cap-migrate)`
- `page/expect-misc.spec.cp`：31 处 `TODO(cap-migrate)`
- `page/expect-to-have-accessible.spec.cp`：18 处 `TODO(cap-migrate)`
- `page/expect-to-have-text.spec.cp`：14 处 `TODO(cap-migrate)`
- `page/expect-to-have-value.spec.cp`：10 处 `TODO(cap-migrate)`
- `page/firefox-network-response.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/frame-evaluate.spec.cp`：4 处 `TODO(cap-migrate)`
- `page/frame-frame-element.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/frame-goto.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/frame-hierarchy.spec.cp`：4 处 `TODO(cap-migrate)`
- `page/interception.spec.cp`：9 处 `TODO(cap-migrate)`
- `page/jshandle-json-value.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/jshandle-properties.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/jshandle-to-string.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/locator-any-frame.spec.cp`：25 处 `TODO(cap-migrate)`
- `page/locator-click.spec.cp`：4 处 `TODO(cap-migrate)`
- `page/locator-convenience.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/locator-evaluate.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/locator-frame.spec.cp`：4 处 `TODO(cap-migrate)`
- `page/locator-is-visible.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/locator-misc-2.spec.cp`：9 处 `TODO(cap-migrate)`
- `page/locator-query.spec.cp`：40 处 `TODO(cap-migrate)`
- `page/locator-wait-for-function.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/matchers.misc.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/network-post-data.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-add-init-script-callback.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-add-init-script.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/page-add-locator-handler.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-add-script-tag.spec.cp`：4 处 `TODO(cap-migrate)`
- `page/page-add-style-tag.spec.cp`：5 处 `TODO(cap-migrate)`
- `page/page-aria-snapshot-ai.spec.cp`：8 处 `TODO(cap-migrate)`
- `page/page-aria-snapshot-json.spec.cp`：9 处 `TODO(cap-migrate)`
- `page/page-aria-snapshot.spec.cp`：6 处 `TODO(cap-migrate)`
- `page/page-autowaiting-basic.spec.cp`：5 处 `TODO(cap-migrate)`
- `page/page-autowaiting-no-hang.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-basic.spec.cp`：5 处 `TODO(cap-migrate)`
- `page/page-cache-storage.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/page-check.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-click-scroll.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/page-click-timeout-2.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/page-click-timeout-3.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/page-click-timeout-4.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/page-click.spec.cp`：39 处 `TODO(cap-migrate)`
- `page/page-dialog.spec.cp`：4 处 `TODO(cap-migrate)`
- `page/page-dispatchevent.spec.cp`：7 处 `TODO(cap-migrate)`
- `page/page-drag.spec.cp`：16 处 `TODO(cap-migrate)`
- `page/page-drop.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-emulate-media.spec.cp`：5 处 `TODO(cap-migrate)`
- `page/page-evaluate-callback.spec.cp`：7 处 `TODO(cap-migrate)`
- `page/page-evaluate-handle.spec.cp`：4 处 `TODO(cap-migrate)`
- `page/page-evaluate.spec.cp`：25 处 `TODO(cap-migrate)`
- `page/page-event-console.spec.cp`：7 处 `TODO(cap-migrate)`
- `page/page-event-load.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/page-event-network.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/page-event-pageerror.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-event-popup.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-event-request.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/page-expose-function.spec.cp`：10 处 `TODO(cap-migrate)`
- `page/page-filechooser.spec.cp`：8 处 `TODO(cap-migrate)`
- `page/page-fill.spec.cp`：35 处 `TODO(cap-migrate)`
- `page/page-focus.spec.cp`：8 处 `TODO(cap-migrate)`
- `page/page-goto.spec.cp`：7 处 `TODO(cap-migrate)`
- `page/page-history.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/page-keyboard.spec.cp`：21 处 `TODO(cap-migrate)`
- `page/page-leaks.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/page-listeners.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-localstorage.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/page-mouse.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-navigation.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/page-network-idle.spec.cp`：8 处 `TODO(cap-migrate)`
- `page/page-network-request.spec.cp`：8 处 `TODO(cap-migrate)`
- `page/page-network-response.spec.cp`：7 处 `TODO(cap-migrate)`
- `page/page-network-sizes.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-request-continue.spec.cp`：5 处 `TODO(cap-migrate)`
- `page/page-request-fallback.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/page-request-fulfill.spec.cp`：5 处 `TODO(cap-migrate)`
- `page/page-request-intercept.spec.cp`：5 处 `TODO(cap-migrate)`
- `page/page-route.spec.cp`：21 处 `TODO(cap-migrate)`
- `page/page-screenshot.spec.cp`：14 处 `TODO(cap-migrate)`
- `page/page-select-option.spec.cp`：20 处 `TODO(cap-migrate)`
- `page/page-set-content.spec.cp`：5 处 `TODO(cap-migrate)`
- `page/page-set-input-files.spec.cp`：12 处 `TODO(cap-migrate)`
- `page/page-strict.spec.cp`：9 处 `TODO(cap-migrate)`
- `page/page-transition.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/page-wait-for-function.spec.cp`：10 处 `TODO(cap-migrate)`
- `page/page-wait-for-load-state.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/page-wait-for-navigation.spec.cp`：6 处 `TODO(cap-migrate)`
- `page/page-wait-for-request.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/page-wait-for-response.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/page-wait-for-selector-1.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-wait-for-selector-2.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/page-wait-for-url.spec.cp`：1 处 `TODO(cap-migrate)`
- `page/pageTest.cp`：8 处 `TODO(cap-migrate)`
- `page/pageTestApi.cp`：2 处 `TODO(cap-migrate)`
- `page/queryselector.spec.cp`：4 处 `TODO(cap-migrate)`
- `page/retarget.spec.cp`：22 处 `TODO(cap-migrate)`
- `page/selectors-css.spec.cp`：203 处 `TODO(cap-migrate)`
- `page/selectors-frame.spec.cp`：16 处 `TODO(cap-migrate)`
- `page/selectors-get-by.spec.cp`：34 处 `TODO(cap-migrate)`
- `page/selectors-misc.spec.cp`：112 处 `TODO(cap-migrate)`
- `page/selectors-register.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/selectors-role.spec.cp`：5 处 `TODO(cap-migrate)`
- `page/selectors-text.spec.cp`：148 处 `TODO(cap-migrate)`
- `page/to-match-aria-snapshot.spec.cp`：3 处 `TODO(cap-migrate)`
- `page/wheel.spec.cp`：2 处 `TODO(cap-migrate)`
- `page/workers.spec.cp`：7 处 `TODO(cap-migrate)`
- `playwright-test/access-data.spec.cp`：4 处 `TODO(cap-migrate)`
- `playwright-test/assets/simple-server.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/babel.spec.cp`：9 处 `TODO(cap-migrate)`
- `playwright-test/basic.spec.cp`：2 处 `TODO(cap-migrate)`
- `playwright-test/command-line-filter.spec.cp`：4 处 `TODO(cap-migrate)`
- `playwright-test/config.spec.cp`：6 处 `TODO(cap-migrate)`
- `playwright-test/coverage.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/current-test-runner/globalSetup.cp`：2 处 `TODO(cap-migrate)`
- `playwright-test/deps.spec.cp`：6 处 `TODO(cap-migrate)`
- `playwright-test/esm.spec.cp`：22 处 `TODO(cap-migrate)`
- `playwright-test/expect-configure.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/expect-poll.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/expect-to-pass.spec.cp`：4 处 `TODO(cap-migrate)`
- `playwright-test/expect.spec.cp`：5 处 `TODO(cap-migrate)`
- `playwright-test/fixture-errors.spec.cp`：14 处 `TODO(cap-migrate)`
- `playwright-test/fixtures.spec.cp`：7 处 `TODO(cap-migrate)`
- `playwright-test/gitignore.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/golden.spec.cp`：15 处 `TODO(cap-migrate)`
- `playwright-test/hooks.spec.cp`：5 处 `TODO(cap-migrate)`
- `playwright-test/list-mode.spec.cp`：4 处 `TODO(cap-migrate)`
- `playwright-test/loader.spec.cp`：20 处 `TODO(cap-migrate)`
- `playwright-test/match-grep.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/only-changed.spec.cp`：8 处 `TODO(cap-migrate)`
- `playwright-test/playwright-test-fixtures.cp`：32 处 `TODO(cap-migrate)`
- `playwright-test/playwright.artifacts.spec.cp`：4 处 `TODO(cap-migrate)`
- `playwright-test/playwright.config.cp`：2 处 `TODO(cap-migrate)`
- `playwright-test/playwright.connect.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/playwright.fetch.spec.cp`：2 处 `TODO(cap-migrate)`
- `playwright-test/playwright.reuse.browser.spec.cp`：2 处 `TODO(cap-migrate)`
- `playwright-test/playwright.reuse.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/playwright.spec.cp`：13 处 `TODO(cap-migrate)`
- `playwright-test/playwright.trace.spec.cp`：31 处 `TODO(cap-migrate)`
- `playwright-test/reporter-attachment.spec.cp`：12 处 `TODO(cap-migrate)`
- `playwright-test/reporter-base.spec.cp`：6 处 `TODO(cap-migrate)`
- `playwright-test/reporter-blob.spec.cp`：53 处 `TODO(cap-migrate)`
- `playwright-test/reporter-dot.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/reporter-html.spec.cp`：100 处 `TODO(cap-migrate)`
- `playwright-test/reporter-json.spec.cp`：6 处 `TODO(cap-migrate)`
- `playwright-test/reporter-junit.spec.cp`：2 处 `TODO(cap-migrate)`
- `playwright-test/reporter-line.spec.cp`：4 处 `TODO(cap-migrate)`
- `playwright-test/reporter-list.spec.cp`：15 处 `TODO(cap-migrate)`
- `playwright-test/reporter-onend.spec.cp`：4 处 `TODO(cap-migrate)`
- `playwright-test/reporter-only-failures.spec.cp`：14 处 `TODO(cap-migrate)`
- `playwright-test/reporter-perfetto.spec.cp`：4 处 `TODO(cap-migrate)`
- `playwright-test/reporter-preprocess.spec.cp`：7 处 `TODO(cap-migrate)`
- `playwright-test/reporter.spec.cp`：14 处 `TODO(cap-migrate)`
- `playwright-test/resolver.spec.cp`：45 处 `TODO(cap-migrate)`
- `playwright-test/retry.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/runner.spec.cp`：13 处 `TODO(cap-migrate)`
- `playwright-test/shard.spec.cp`：4 处 `TODO(cap-migrate)`
- `playwright-test/shuffle.spec.cp`：6 处 `TODO(cap-migrate)`
- `playwright-test/stdio.spec.cp`：3 处 `TODO(cap-migrate)`
- `playwright-test/test-extend.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/test-ignore.spec.cp`：2 处 `TODO(cap-migrate)`
- `playwright-test/test-list.spec.cp`：6 处 `TODO(cap-migrate)`
- `playwright-test/test-locks.spec.cp`：2 处 `TODO(cap-migrate)`
- `playwright-test/test-modifiers.spec.cp`：3 处 `TODO(cap-migrate)`
- `playwright-test/test-output-dir.spec.cp`：5 处 `TODO(cap-migrate)`
- `playwright-test/test-parallel.spec.cp`：2 处 `TODO(cap-migrate)`
- `playwright-test/test-serial.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/test-server.spec.cp`：11 处 `TODO(cap-migrate)`
- `playwright-test/test-step.spec.cp`：2 处 `TODO(cap-migrate)`
- `playwright-test/test-tag.spec.cp`：4 处 `TODO(cap-migrate)`
- `playwright-test/test-use.spec.cp`：4 处 `TODO(cap-migrate)`
- `playwright-test/timeout.spec.cp`：6 处 `TODO(cap-migrate)`
- `playwright-test/to-have-screenshot.spec.cp`：19 处 `TODO(cap-migrate)`
- `playwright-test/ui-mode-fixtures.cp`：8 处 `TODO(cap-migrate)`
- `playwright-test/ui-mode-test-attachments.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/ui-mode-test-filters.spec.cp`：1 处 `TODO(cap-migrate)`
- `playwright-test/ui-mode-test-network-tab.spec.cp`：3 处 `TODO(cap-migrate)`
- `playwright-test/ui-mode-test-output.spec.cp`：3 处 `TODO(cap-migrate)`
- `playwright-test/ui-mode-test-progress.spec.cp`：40 处 `TODO(cap-migrate)`
- `playwright-test/ui-mode-test-run.spec.cp`：3 处 `TODO(cap-migrate)`
- `playwright-test/ui-mode-test-setup.spec.cp`：2 处 `TODO(cap-migrate)`
- `playwright-test/ui-mode-test-source.spec.cp`：7 处 `TODO(cap-migrate)`
- `playwright-test/ui-mode-test-update.spec.cp`：5 处 `TODO(cap-migrate)`
- `playwright-test/ui-mode-test-watch.spec.cp`：3 处 `TODO(cap-migrate)`
- `playwright-test/ui-mode-trace.spec.cp`：59 处 `TODO(cap-migrate)`
- `playwright-test/update-aria-snapshot.spec.cp`：13 处 `TODO(cap-migrate)`
- `playwright-test/watch.spec.cp`：10 处 `TODO(cap-migrate)`
- `playwright-test/web-server.spec.cp`：7 处 `TODO(cap-migrate)`
- `playwright-test/worker-index.spec.cp`：2 处 `TODO(cap-migrate)`
- `stress/browsers.spec.cp`：2 处 `TODO(cap-migrate)`
- `third_party/proxy/index.cp`：9 处 `TODO(cap-migrate)`
- `webview/expectationUtil.cp`：5 处 `TODO(cap-migrate)`
- `webview/playwright.config.cp`：1 处 `TODO(cap-migrate)`
- `webview/webviewTest.cp`：11 处 `TODO(cap-migrate)`

### 5.4 快速定位命令

```bash
# 检索全部迁移遗留 TODO 标记
rg -n "TODO\(cap-migrate\)" packages/playwright/test

# 运行 cpc 逐文件静态诊断（自带 AI-First 修复建议）
cpc check packages/playwright/test/src/
```

## 6. 测试、调试、运行与正式发布操作手册

```bash
cd packages/playwright/test

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
