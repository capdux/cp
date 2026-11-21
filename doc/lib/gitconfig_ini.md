# `cap:gitconfig_ini` — 纯 CP 零堆分配 Git Config / INI 解析与 URL 重写引擎

## 概述

`cap:gitconfig_ini` 吸收自 Bun `src/install/repository.cp`，纯 CP 零外部依赖、零堆分配，提供固定容量（128 条目）的 Git Config / INI 解析器，支持 `[section]`、`[section "subsection"]`、注释与引号、隐式布尔键、`core.askpass`/`core.sshCommand` 检测及 `url.<base>.insteadOf` 最长前缀 URL 重写。

## 核心类型与 API

- `GitConfigEntry`：`section`、`subsection`、`key`、`value`（零拷贝切片）
- `GitConfig`：
  - `GitConfig.parse(text: []const u8): GitConfig`
  - `get(section, subsection, key): ([]const u8)?`（section/key 大小写无关，subsection 大小写敏感，后定义覆盖前定义）
  - `getBool(section, subsection, key): bool?`（`true`/`yes`/`on`/`1` 与 `false`/`no`/`off`/`0`）
  - `hasCoreAskpass(): bool` / `hasCoreSshCommand(): bool`
  - `rewriteUrl(url: []const u8, out: []u8): ([]const u8)?`（按最长前缀匹配 `url.<base>.insteadOf`）

## 示例

```cp
import { GitConfig } from "cap:gitconfig_ini";

const cfg = GitConfig.parse("[url \"https://github.com/\"]\n  insteadOf = gh:\n");
let buf: [128]u8 = std.mem.zeroes([128]u8);
const u = cfg.rewriteUrl("gh:cp-lang/cp.git", buf[0..]).?; // "https://github.com/cp-lang/cp.git"
```
