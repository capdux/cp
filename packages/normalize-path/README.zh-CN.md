[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# `normalize-path`

> **源包溯源**：`npm:normalize-path` ([jonschlinkert/normalize-path](https://github.com/jonschlinkert/normalize-path)) — 协议：MIT

纯 CP 实现的零堆分配 POSIX / Windows 路径规范化库，支持反斜杠转换、连续斜杠折叠、`.` / `..` 路径段消解，以及 UNC 与 Windows 盘符根目录保护。

## 零成本迁移对照（TS → CP）

直接沿用原包名 `normalize-path`，无需改名：

```typescript
// TypeScript / Node.js (npm: normalize-path)
import normalizePath from "normalize-path";
normalizePath("\\foo\\bar\\baz\\");        // "/foo/bar/baz"
normalizePath("\\foo\\bar\\baz\\", false); // "/foo/bar/baz/"
```

```cp
// CP（零堆分配，直接写入缓冲区）
import { normalizePath, normalizePathDefault } from "normalize-path";

let out: [128]u8 = undefined;
let n1 = normalizePathDefault("\\foo\\bar\\baz\\", out[0..128]); // "/foo/bar/baz"
let n2 = normalizePath("\\foo\\bar\\baz\\", false, out[0..128]); // "/foo/bar/baz/"
```

## 安装配置

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "normalize-path": "1.0.0"
  }
}
```

## 核心 API 速览

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `NormalizePathOptions` | `class { strip_trailing: bool, resolve_dots: bool, fn defaults(): NormalizePathOptions }` | 规范化选项（去尾斜杠、消解 `.` / `..`） |
| `normalizePath` | `(path: string, strip_trailing: bool, out: []u8): usize` | `\` 转 `/`、折叠重复斜杠、保留 UNC `//` 与根 `/` |
| `normalizePathDefault` | `(path: string, out: []u8): usize` | `normalizePath(path, true, out)` 快捷版 |
| `normalizePathWith` | `(path: string, opts: NormalizePathOptions, out: []u8): usize` | 支持可选 `.` / `..` 路径消解的完整规范化 |
| `toPosixPath` | `(path: string, out: []u8): usize` | 将所有 `\` 替换为 `/` |
| `stripTrailingSlash` | `(path: string): string` | 零拷贝去除末尾 `/`（保留 `/` 与盘符根） |
| `hasBackslash` | `(path: string): bool` | 检查路径中是否包含 `\` |
| `isUncPath` | `(path: string): bool` | 判断是否为 UNC 路径（`\\host\share` 或 `//host/share`） |
| `isWindowsDrivePath` | `(path: string): bool` | 判断是否为 Windows 盘符路径（`C:\...` 或 `C:/...`） |
| `npIsSlash` / `npIsAsciiAlpha` | `(ch: u8): bool` | 底层字节判断辅助函数 |

## 实战示例

### 1. 常规路径与 Windows / UNC 规范化

```cp
import { normalizePathDefault, normalizePath, isUncPath, isWindowsDrivePath } from "normalize-path";

let out: [128]u8 = undefined;

let n1 = normalizePathDefault("\\\\server\\share\\\\dir\\", out[0..128]); // "//server/share/dir"
let n2 = normalizePathDefault("C:\\Users\\\\dev\\", out[0..128]);         // "C:/Users/dev"
let n3 = normalizePath("foo/bar/", false, out[0..128]);                   // "foo/bar/"

let unc = isUncPath("\\\\server\\share");   // true
let drv = isWindowsDrivePath("C:\\src");    // true
```

### 2. 消解 `.` / `..` 与零拷贝去尾斜杠

```cp
import { NormalizePathOptions, normalizePathWith, toPosixPath, stripTrailingSlash } from "normalize-path";

let out: [128]u8 = undefined;

let opts = NormalizePathOptions.defaults();
opts.resolve_dots = true;
opts.strip_trailing = true;

let n1 = normalizePathWith("a/b/../c/./d/", opts, out[0..128]); // "a/c/d"
let n2 = toPosixPath("src\\lib\\index.cp", out[0..128]);        // "src/lib/index.cp"
let clean = stripTrailingSlash("/var/log///");                  // "/var/log"
```

## 注意事项与边界说明

- **根目录保护**：即使开启 `strip_trailing`，根路径 `"/"` 与 Windows 盘符根 `"C:/"` 仍会被完整保留。
- **UNC 前缀保护**：`\\server\share` 会规范化为 `//server/share`，不会误折叠为单斜杠。
- **零堆分配**：写入函数返回实际写入字节数 `usize`；`stripTrailingSlash` 为零拷贝切片。
