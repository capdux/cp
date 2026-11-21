# `cap:feature_flags` — 纯 CP 特性开关与环境变量强类型解析引擎（吸收自 Bun `bun_core`）

## 概述

`cap:feature_flags`（亦支持 `cap.feature_flags` 动态挂载）从 Bun `src/bun_core/{feature_flags,env_var}.cp` 吸收并纯 CP 化：提供编译期/运行时默认特性开关集 `FeatureFlags` 与容错环境变量布尔/无符号整数解析器（`parseBoolEnv`、`parseUintEnv`、`parseBoolOr`、`parseUintOr`）。

## 核心类型与 API

- `FeatureFlags`: 编译期与运行时开关结构体
- `parseBoolEnv(raw) -> bool?`（支持 `1`/`true`/`yes`/`on` 与 `0`/`false`/`no`/`off`）
- `parseUintEnv(raw) -> u64?`
- `parseBoolOr(raw, default_val)` / `parseUintOr(raw, default_val)`

## 示例

```cp
import { FeatureFlags, parseBoolOr, parseUintOr } from "cap:feature_flags";

const flags = FeatureFlags{};
const keepalive = parseBoolOr("true", flags.enable_keepalive);
const ttl = parseUintOr("30", 60);
```
