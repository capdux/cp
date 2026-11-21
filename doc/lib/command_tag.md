# `cap:command_tag` — 纯 CP CLI 子命令分类与配置加载策略引擎（吸收自 Bun `CommandTag`）

## 概述

`cap:command_tag`（亦支持 `cap.command_tag` 动态挂载）从 Bun `src/options_types/CommandTag.cp` 吸收并纯 CP 化：定义 32 个顶层 CLI 子命令判别枚举 `Tag`，提供遥测单字符码 `char()`、NPM 包管理判定 `isNPMRelated()`、全局配置读取 `readGlobalConfig()`、项目配置按需/强制加载 `loadsConfig()`/`alwaysLoadsConfig()` 及子命令字符串反查 `fromSubcommand(name)`。

## 核心类型与 API

- `Tag`: 32 个子命令枚举（`char()`、`readGlobalConfig()`、`isNPMRelated()`、`loadsConfig()`、`alwaysLoadsConfig()`、`usesGlobalOptions()`）
- `fromSubcommand(name) -> Tag?`

## 示例

```cp
import { fromSubcommand } from "cap:command_tag";

const tag = fromSubcommand("install").?;
const is_npm = tag.isNPMRelated(); // true
```
