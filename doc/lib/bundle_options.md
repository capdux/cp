# `cap:bundle_options` — 纯 CP 打包格式、缓存策略与链接模式枚举引擎（吸收自 Bun `options_types`）

## 概述

`cap:bundle_options`（亦支持 `cap.bundle_options` 动态挂载）从 Bun `src/options_types/` 与 `src/install_types/` 吸收并纯 CP 化，包含模块输出格式 `Format`（`esm`/`cjs`/`iife`）、全局缓存策略 `GlobalCache`、`OfflineMode`、`NodeLinker`（`hoisted`/`isolated`）及 `WindowsOptions`。

## 核心类型与 API

- `Format` + `parseFormat(s)`（`isESM()`、`keepES6ImportExportSyntax()`、`isAlwaysStrictMode()`）
- `GlobalCache` + `parseGlobalCache(s)`（`canUse(has_node_modules)`、`canInstall()`、`isEnabled()`、`allowVersionSpecifier()`）
- `OfflineMode` + `parseOfflineMode(s)` / `NodeLinker` + `parseNodeLinker(s)` / `BundlePackage` / `WindowsOptions`

## 示例

```cp
import { parseFormat, parseGlobalCache } from "cap:bundle_options";

const fmt = parseFormat("esm").?;
const gc = parseGlobalCache("auto").?;
```
