[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# `resolve-from`

> **源包溯源**：`npm:resolve-from` ([sindresorhus/resolve-from](https://github.com/sindresorhus/resolve-from)) — 协议：MIT

纯 CP 实现的零堆分配模块路径解析器，支持从指定基准目录解析相对路径（`./`、`../`）、绝对路径（`/`）、裸包名与作用域包（`@scope/pkg`），自动向上回溯探测 `node_modules` 与 `cap_modules`。

## 零成本迁移对照（TS → CP）

直接沿用原包名 `resolve-from`，无需改名：

```typescript
// TypeScript / Node.js (npm: resolve-from)
import resolveFrom from "resolve-from";
const hit = resolveFrom.silent("/app/src", "./utils");
```

```cp
// CP（零堆分配，纯内存文件列表探测，无隐式 I/O）
import { resolveFrom, resolveFromSilent } from "resolve-from";

let files: [2]string = ["/app/src/utils.cp", "/app/cap_modules/pad/index.cp"];
let out: [256]u8 = undefined;
let n_opt = resolveFromSilent("/app/src", "./utils", files[0..2], out[0..256]);
if (n_opt != null) {
    let resolved = out[0..(n_opt ?? 0)] as string; // "/app/src/utils.cp"
}
```

## 安装配置

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "resolve-from": "1.0.0"
  }
}
```

## 核心 API 速览

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `ResolveResult` | `class { found: bool, len: usize, err_code: i32 }` | 结构化解析结果（`0`=成功，`1`=空输入，`2`=未找到，`3`=缓冲区不足） |
| `ResolveOptions` | `class { extensions: [8]string, ext_count: usize, include_node_modules: bool, include_cap_modules: bool, fn defaults(): ResolveOptions }` | 自定义后缀探测表与 `node_modules`/`cap_modules` 开关 |
| `resolveFrom` | `(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize?` | 从 `from_dir` 解析模块，成功返回写入字节数，失败返回 `null` |
| `resolveFromSilent` | `(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize?` | 对齐 npm `resolveFrom.silent` 的静默解析接口 |
| `resolveFromWithOptions` | `(from_dir: string, module_id: string, existing_files: []string, opts: ResolveOptions, out: []u8): ResolveResult` | 带配置与错误码的完整解析接口 |
| `isRelativeSpecifier` | `(spec: string): bool` | 判断是否为 `.`、`..`、`./*`、`../*` 相对导入 |
| `isScopedSpecifier` | `(spec: string): bool` | 判断是否为 `@scope/pkg` 作用域包 |
| `isBareSpecifier` | `(spec: string): bool` | 判断是否为裸包或作用域包（非空、非相对、非绝对） |
| `rfExists` / `rfProbeBase` / `rfProbeBaseOpts` / `rfResolveRelativeBase` | 底层辅助函数 | 路径存在性检查、后缀探测与相对基准拼接 |

## 实战示例

### 1. 解析相对路径、裸包与作用域包

```cp
import { resolveFrom, isRelativeSpecifier, isScopedSpecifier, isBareSpecifier } from "resolve-from";

let files: [3]string = [
    "/repo/apps/web/src/helper.cp",
    "/repo/node_modules/@acme/ui/index.js",
    "/repo/cap_modules/pad/index.cp",
];
let out: [256]u8 = undefined;

let r1 = resolveFrom("/repo/apps/web/src", "./helper", files[0..3], out[0..256]);
let r2 = resolveFrom("/repo/apps/web/src", "@acme/ui", files[0..3], out[0..256]);
let r3 = resolveFrom("/repo/apps/web/src", "pad", files[0..3], out[0..256]);

let rel = isRelativeSpecifier("../shared"); // true
let scp = isScopedSpecifier("@acme/ui");    // true
let bare = isBareSpecifier("pad");          // true
```

### 2. 自定义探测配置与错误码诊断

```cp
import { ResolveOptions, resolveFromWithOptions } from "resolve-from";

let files: [1]string = ["/repo/cap_modules/foo/index.cp"];
let opts = ResolveOptions.defaults();
opts.include_node_modules = false;
opts.include_cap_modules = true;

let out: [256]u8 = undefined;
let res = resolveFromWithOptions("/repo/src", "foo", files[0..1], opts, out[0..256]);
if (res.found) {
    let resolved = out[0..res.len] as string;
}
```

## 注意事项与边界说明

- **纯函数可确定性**：通过传入 `existing_files: []string` 进行探测，不执行隐藏文件 I/O，适用于打包器、编译器与沙箱环境。
- **默认后缀顺序**：`ResolveOptions.defaults()` 依次探测 `""`、`".cp"`、`".ts"`、`".js"`、`".json"`、`"/index.cp"`、`"/index.ts"`、`"/index.js"`。
- **缓冲区校验**：`resolveFromWithOptions` 在 `out` 容量不足时返回 `found: false, err_code: 3`。
