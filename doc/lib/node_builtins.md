# `cap:node_builtins` — 纯 CP Node.js & Bun 内建模块映射注册表（吸收自 Bun `resolve_builtins`）

## 概述

`cap:node_builtins`（亦支持 `cap.node_builtins` 动态挂载）从 Bun `src/resolve_builtins/HardcodedModule.cp` 吸收并纯 CP 化：提供编译期完美哈希 `StaticStringMap` 查找表，覆盖全部 `node:*` 与裸模块名、`sys -> node:util` / `_tls_wrap -> node:tls` 历史别名、`node:test`/`node:sqlite` 前缀强制策略，以及 `bun:*` 与 Jest/Vitest 测试重写规则。

## 核心类型与 API

- `RuntimeTarget`: `.Node` | `.Bun` | `.Browser`
- `ModuleTag`: `.builtin` | `.bun`
- `BuiltinAlias`: `{ path, tag, node_builtin, node_only_prefix }`
- `resolveAlias(name, target, rewrite_jest_for_tests) -> OptBuiltinAlias`
- `isBuiltin(name, target) -> bool`
- `canonicalizeNode(name) -> []const u8?`

## 示例

```cp
import { resolveAlias, canonicalizeNode } from "cap:node_builtins";

const path = canonicalizeNode("sys").?; // "node:util"
const jest = resolveAlias("vitest", .Bun, true).?; // "bun:test"
```
