# `cap:builtin_modules` — 纯 CP Node.js 与 Bun 内置模块全量注册表与目标解析器

> 参考来源：/data/dev/bun/src/resolve_builtins/HardcodedModule.cp | 吸收日期：2026-11-22

## 概述

`cap:builtin_modules` 吸收自 Bun 模块解析器核心注册表（`src/resolve_builtins/HardcodedModule.cp`），纯 CP 零外部依赖、零堆分配，提供 Node.js 全量核心模块及子路径、Bun 专有内置模块、原生运行时覆盖包以及测试框架重写规则的规范化解析。

## 上游解析规则与行为契约

- **按目标运行环境隔离（`RuntimeTarget`）**：
  - `RuntimeTarget.node`：仅解析 Node.js 核心内置模块（含 `node:` 前缀与裸名，及 `_http_*`、`_stream_*` 等历史内部子模块），不识别 `bun:*` 与 Bun 专属重写；
  - `RuntimeTarget.bun`：在 Node.js 核心模块基础上叠加 Bun 专有模块（`bun`、`bun:test`、`bun:ffi`、`bun:sqlite`、`bun:jsc`、`bun:main`、`bun:wrap`、`bun:app`、`ffi -> bun:ffi`）以及 Bun 原生加速替换的第三方包（如 `ws`、`node-fetch`、`undici`、`@vercel/fetch`、`isomorphic-fetch`、`abort-controller`、`utf-8-validate` 及 `next/dist/compiled/*`）；
  - `RuntimeTarget.browser`：无服务端内置模块，始终返回 `null`。
- **仅限 `node:` 前缀的新内置模块（`node_only_prefix`）**：遵循 Node.js 与 Bun 共同规范，较新的内置模块（如 `node:test`、`node:sqlite`、`node:sea`）只能以带 `node:` 前缀的形式解析，裸名 `"test"` / `"sqlite"` 不会被误判为内置模块（避免与用户 npm 包冲突），解析结果中 `node_only_prefix == true`。
- **历史别名规范化**：
  - `sys` 与 `node:sys` 统一规范化为 `node:util`；
  - `_tls_wrap` 与 `node:_tls_wrap` 统一规范化为 `node:tls`；
  - 裸名核心模块（如 `"fs/promises"`）统一规范化为带前缀的 `"node:fs/promises"`。
- **测试模式重写（`ResolveConfig.rewrite_jest_for_tests`）**：当 `target == .bun` 且 `rewrite_jest_for_tests == true` 时，自动将 `@jest/globals` 等测试框架导入重定向至 `bun:test`。

## 核心类型与 API

### 枚举与结构

- `RuntimeTarget`：`node` / `bun` / `browser`（附 `isNode()` / `isBun()` / `isBrowser()`）
- `BuiltinKind`：`node_builtin` / `bun_builtin` / `runtime_override`
- `ResolveConfig`：`rewrite_jest_for_tests: bool = false`
- `ResolvedBuiltin`：`canonical` / `kind` / `node_builtin` / `node_only_prefix`（附 `isNodeBuiltin()` / `isBunBuiltin()` / `requiresNodePrefix()`）

### 解析与前缀规范化函数

- `resolveBuiltin(specifier: string, target: RuntimeTarget, cfg: ResolveConfig): ResolvedBuiltin?`
- `isBuiltin(specifier: string, target: RuntimeTarget, cfg: ResolveConfig): bool`
- `isNodeBuiltin(specifier: string): bool`
- `hasNodePrefix(specifier: string): bool`
- `hasBunPrefix(specifier: string): bool`
- `stripNodePrefix(specifier: string): string`
- `ensureNodePrefix(buf: []u8, specifier: string): string?`

## 边界注意事项

1. **区分大小写**：所有模块名匹配均严格区分大小写（符合 Node.js / ESM 规范），例如 `"FS"` 或 `"Node:fs"` 不会被识别为内置模块。
2. **`ensureNodePrefix` 缓冲检查**：若目标 `buf` 长度不足以容纳 `"node:"` + `specifier`，返回 `null` 而不会截断或越界；若已带 `"node:"` 前缀则直接拷贝/返回规范形式。
3. **测试重写作用域**：`rewrite_jest_for_tests` 仅在 `RuntimeTarget.bun` 下生效，在 `RuntimeTarget.node` 下不会劫持 `@jest/globals`。

## 示例

```cp
import {
    RuntimeTarget,
    ResolveConfig,
    resolveBuiltin,
    isNodeBuiltin,
    ensureNodePrefix,
} from "cap:builtin_modules";

const fs_mod = resolveBuiltin("fs/promises", RuntimeTarget.node, ResolveConfig{}).?;
// fs_mod.canonical == "node:fs/promises", fs_mod.isNodeBuiltin() == true

// node:test 仅限带 node: 前缀解析
const ok_test = isNodeBuiltin("node:test"); // true
const bad_test = isNodeBuiltin("test");     // false

const jest_mod = resolveBuiltin("@jest/globals", RuntimeTarget.bun, ResolveConfig{ rewrite_jest_for_tests: true }).?;
// jest_mod.canonical == "bun:test"

let buf: [64]u8 = undefined;
const prefixed = ensureNodePrefix(buf[0..], "path/posix"); // "node:path/posix"
* = ok_test;
* = bad_test;
* = prefixed;
```
