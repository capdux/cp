# `cap:semver_query` — 纯 CP 包管理器依赖版本描述符与多协议解析器

## 概述

`cap:semver_query` 吸收自 Bun `src/semver/SemverQuery.cp`，纯 CP 零外部依赖、零堆分配，提供 npm semver、dist-tag、`workspace:`、`catalog:`、`npm:` 别名、`github:`、`git+`、`file:`/`link:`/本地目录、tarball URL 多协议依赖解析与规范化输出，以及标准 npm `PackageName` 校验与作用域拆分。

## 核心类型与 API

- `DependencyKind`：`npm` / `dist_tag` / `workspace` / `catalog` / `file` / `link` / `folder` / `tarball` / `github` / `git` / `npm_alias`
- `WorkspaceSpecKind`（`star`/`caret`/`tilde`/`semver`）与 `WorkspaceSpec`
- `GitSpec`（`url`/`owner`/`repo`/`committish`）
- `NpmAliasSpec`（`alias_name`/`version_spec`）
- `DependencySpec`：`parse(input)`、`isLocal()`、`isWorkspace()`、`isGit()`、`toCanonical(out)`
- `PackageName`：`isValid(input)`、`parse(input)`、`isScoped()`（长度 <= 214，拆分 `scope` 与 `name`，禁用大写/空格/前导 `.` 或 `_`）

## 示例

```cp
import { DependencySpec, PackageName } from "cap:semver_query";

const dep = DependencySpec.parse("npm:@scope/pkg@^2.0.0");
// dep.npm_alias.?.alias_name == "@scope/pkg"
const pkg = PackageName.parse("@cp-lang/core").?;
// pkg.scope.? == "cp-lang", pkg.name == "core"
```
