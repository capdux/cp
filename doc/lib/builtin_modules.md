# `cap:builtin_modules` — 纯 CP Node.js 与 Bun 内置模块全量注册表与目标解析器

## 概述

`cap:builtin_modules` 吸收自 Bun `src/resolve_builtins/HardcodedModule.cp`，纯 CP 零外部依赖、零堆分配，提供 Node.js 全量核心模块及子路径（含 `node:test`/`node:sqlite`/`node:sea` 仅限 `node:` 前缀约束）、Bun 专有内置模块、Jest/Vitest 测试重写开关以及前缀规范化工具。

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
- `hasNodePrefix(specifier)` / `hasBunPrefix(specifier)` / `stripNodePrefix(specifier)` / `ensureNodePrefix(buf, specifier)`

## 示例

```cp
import { RuntimeTarget, ResolveConfig, resolveBuiltin, isNodeBuiltin } from "cap:builtin_modules";

const fs_mod = resolveBuiltin("fs/promises", RuntimeTarget.node, ResolveConfig{}).?;
// fs_mod.canonical == "node:fs/promises"

const jest_mod = resolveBuiltin("@jest/globals", RuntimeTarget.bun, ResolveConfig{ rewrite_jest_for_tests: true }).?;
// jest_mod.canonical == "bun:test"
```
