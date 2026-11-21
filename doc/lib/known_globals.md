# `cap:known_globals` — 纯 CP ECMAScript / Web / Node / Bun 已知全局符号与无副作用判定库

## 概述

`cap:known_globals` 吸收自 Bun `src/js_parser/ast/KnownGlobal.cp`，纯 CP 零外部依赖、零堆分配，为 JS/TS 编译器、打包器与 Tree-Shaker 提供已知全局符号元数据、纯无参构造判定、`new` 关键字安全消除判定以及静态成员读写与纯函数调用分析。

## 核心类型与 API

### 枚举与结构

- `GlobalNamespace`：`ecmascript` / `web_api` / `node_runtime` / `bun_runtime`
- `GlobalInfo`：`name`、`namespace`、`can_drop_new`、`pure_zero_arg_new`、`pure_read`（附 `isEcmaScript()` / `isWebApi()` / `isNodeRuntime()` / `isBunRuntime()`）

### 查询与纯度分析函数

- `lookupGlobal(name: string): GlobalInfo?`（严格大小写敏感）
- `isKnownGlobal(name: string): bool`
- `isPureZeroArgConstructor(name: string): bool`
- `canDropNewKeyword(name: string): bool`
- `isPurePropertyAccess(global_name: string, prop: string): bool`
- `isPureGlobalCall(global_name: string, prop: string): bool`

## 示例

```cp
import {
    lookupGlobal,
    isPureZeroArgConstructor,
    canDropNewKeyword,
    isPurePropertyAccess,
    isPureGlobalCall,
} from "cap:known_globals";

const err_info = lookupGlobal("TypeError").?;
// err_info.isEcmaScript() == true && canDropNewKeyword("TypeError") == true
// isPureZeroArgConstructor("WeakMap") == true
// isPurePropertyAccess("Math", "PI") == true
// isPureGlobalCall("Math", "floor") == true
```
