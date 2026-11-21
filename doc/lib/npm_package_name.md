# `cap:npm_package_name` — npm / Node.js 包名与模块说明符解析库

> 来源：吸收自 `/data/dev/bun/src/install/PackageManager.zig`（npm Package Name & Module Specifier Parser）

`cap:npm_package_name` 提供纯 CP、零外部依赖、零堆分配的 npm 包名合法性校验（`validate-npm-package-name` 规范）、作用域/子路径拆分与相对/裸模块说明符判定能力。

## 核心类型与函数

### `PackageSpecifier`

```cp
pub struct PackageSpecifier {
    scope: ([]const u8)? = null,
    name: []const u8 = "",
    full_name: []const u8 = "",
    subpath: ([]const u8)? = null,
}
```

### 核心函数

```cp
pub fn parsePackageSpecifier(spec: []const u8): PackageSpecifier?
pub fn isValidPackageName(pkg_name: []const u8): bool
pub fn isRelativeSpecifier(spec: []const u8): bool
pub fn isBareSpecifier(spec: []const u8): bool
```

- `parsePackageSpecifier`：解析 `@scope/name`、`@scope/name/a/b`、`pkg`、`pkg/a/b`；拒绝相对路径、绝对路径、协议前缀（`node:`/`bun:`/`cap:`）、裸 `@scope` 与空路径段。
- `isValidPackageName`：校验总长 `1..=214`、全小写无大写、不以 `.` 或 `_` 开头、无首尾空白与非 URL 安全字符（`~'!*()` 等），并禁止黑名单 `node_modules` 与 `favicon.ico`。
- `isRelativeSpecifier`：识别 `"."`、`".."`、`"./..."`、`"../..."`。
- `isBareSpecifier`：判断非空、非相对、非绝对且非协议前缀的裸模块说明符。

## 使用示例

```cp
import { parsePackageSpecifier, isValidPackageName } from "cap:npm_package_name";

const s = parsePackageSpecifier("@types/node/fs/promises").?;
// s.scope.? == "@types", s.name == "node", s.full_name == "@types/node", s.subpath.? == "fs/promises"
// isValidPackageName(s.full_name) == true
```
