# `cap:small_string` — 纯 CP 8 字节短串内联 (SSO) 与去重驻留池（吸收自 Bun `install_types`）

## 概述

`cap:small_string`（亦支持 `cap.small_string` 动态挂载）从 Bun `src/install_types/{SemverString,ExternalString,SlicedString}.cp` 吸收并纯 CP 化：提供 8 字节 `extern struct SmallString`（`<= 8` 字节零堆内联，长串存 `(offset, len)`）、带 64 位 Wyhash 指纹的 `ExternalString` 以及自动去重驻留池 `StringInternPool`。

## 核心类型与 API

- `SmallString`: `initInline(s)`、`initOffset(off, len)`、`isInline()`、`len()`、`slice(arena_buf)`、`order(...)`、`eql(...)`
- `ExternalString`: `{ value: SmallString, hash: u64 }`
- `StringInternPool`: `init(alloc)`、`deinit(&self)`、`intern(&self, s) -> !ExternalString`、`buffer(self)`

## 示例

```cp
import { StringInternPool } from "cap:small_string";

let pool = StringInternPool.init(alloc);
defer pool.deinit();
const s1 = try pool.intern("react");
const s2 = try pool.intern("@typescript-eslint/parser");
```
