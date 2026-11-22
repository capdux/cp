# `cap:known_globals` — 纯 CP ECMAScript / Web / Node / Bun 已知全局符号与无副作用判定库

> 参考来源：/data/dev/bun/src/js_parser/ast/KnownGlobal.cp | 吸收日期：2026-11-22

## 概述

`cap:known_globals` 吸收自 Bun JS/TS 解析器与压缩器核心模块（`src/js_parser/ast/KnownGlobal.cp`），纯 CP 零外部依赖、零堆分配，为编译器、打包器与 Tree-Shaker 提供已知全局符号命名空间分类、纯无参构造判定、`new` 关键字安全消除判定以及静态成员读写与纯函数调用分析。

## 上游优化背景与行为契约

- **安全消除 `new` 关键字（`canDropNewKeyword`）**：在 Bun `KnownGlobal.minifyGlobalConstructor` 中，对于调用与 `new` 行为完全等价的内建构造器（如 `Error`、`TypeError`、`SyntaxError`、`RangeError`、`ReferenceError`、`EvalError`、`URIError`、`AggregateError`、`Object`、`Array`、`RegExp` 等），压缩器可将 `new TypeError(msg)` 安全重写为 `TypeError(msg)` 以节省字节；而对必须使用 `new` 的构造器（如 `Map`、`Set`、`WeakMap`、`WeakSet`、`Headers`、`TextEncoder`）则严格禁止省略 `new`。
- **纯无参构造与 Tree-Shaking（`isPureZeroArgConstructor`）**：判定无参构造（如 `new WeakMap()`、`new Set()`、`new Map()`、`new TextEncoder()` 等）在结果未被使用时是否无观察得到的副作用，供死代码消除（DCE）安全移除。
- **静态属性与纯函数分析**：支持识别无副作用的只读全局属性（如 `Math.PI`、`Number.MAX_SAFE_INTEGER`、`Symbol.iterator`）与纯函数调用（如 `Math.floor`、`Object.keys` 等）。
- **未绑定符号前提**：与上游一致，所有纯度与重写判定仅在目标标识符为**未遮蔽（unbound）全局符号**时成立；若局部作用域重新声明了同名变量（如 `const Math = ...`），调用方不应应用全局纯度优化。

## 核心类型与 API

### 枚举与结构

- `GlobalNamespace`：`ecmascript` / `web_api` / `node_runtime` / `bun_runtime`
- `GlobalInfo`：`name`、`namespace`、`can_drop_new`、`pure_zero_arg_new`、`pure_read`
  - 辅助方法：`isEcmaScript()` / `isWebApi()` / `isNodeRuntime()` / `isBunRuntime()`

### 查询与纯度分析函数

- `lookupGlobal(name: string): GlobalInfo?` — 严格大小写敏感查询全局符号元数据。
- `isKnownGlobal(name: string): bool` — 判断是否为已知全局符号。
- `isPureZeroArgConstructor(name: string): bool` — 判断无参 `new Name()` 是否无副作用。
- `canDropNewKeyword(name: string): bool` — 判断 `new Name(...)` 是否可安全降级为 `Name(...)`。
- `isPurePropertyAccess(global_name: string, prop: string): bool` — 判断 `Global.prop` 属性读取是否无副作用。
- `isPureGlobalCall(global_name: string, prop: string): bool` — 判断 `Global.prop(...)` 静态方法调用是否纯函数。

## 边界注意事项

1. **严格大小写敏感**：`"math"`、`"typeerror"` 等非规范大小写均返回 `null` / `false`。
2. **有参构造差异**：`isPureZeroArgConstructor` 仅保证无参形式无副作用；传入对象参数可能触发 `Symbol.iterator`、`valueOf()` 或 `toString()` 等用户 getter 副作用，调用方需另行校验参数纯度。

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
const pure_wm = isPureZeroArgConstructor("WeakMap"); // true
const pure_pi = isPurePropertyAccess("Math", "PI");  // true
const pure_fl = isPureGlobalCall("Math", "floor");   // true
* = err_info;
* = pure_wm;
* = pure_pi;
* = pure_fl;
```
