# `cap:glob_escape` — Glob 通配符转义、静态前缀提取与否定模式分析库

> 来源：吸收自 `/data/dev/bun/src/glob.zig`（Glob Pattern Escaping & Prefix Analyzer）

`cap:glob_escape` 提供纯 CP、零外部依赖、零堆分配的 Glob 元字符识别、转义/反转义、静态目录前缀提取（用于文件系统遍历剪枝）以及 `.gitignore` 风格否定模式判定能力。

## 核心函数

```cp
pub fn isGlobMagic(c: u8): bool
pub fn hasGlobMagic(pattern: []const u8): bool
pub fn escapeGlob(literal: []const u8, out: []u8): ([]const u8)?
pub fn unescapeGlob(escaped: []const u8, out: []u8): ([]const u8)?
pub fn staticBaseDir(pattern: []const u8): []const u8
pub fn isNegatedPattern(pattern: []const u8): bool
pub fn stripNegation(pattern: []const u8): []const u8
```

- `isGlobMagic`：识别 `*`, `?`, `[`, `]`, `{`, `}`, `(`, `)`, `!`, `\`。
- `hasGlobMagic`：检测模式中是否存在未转义的 Glob 通配符（忽略 `\` 转义后的字面量）。
- `escapeGlob` / `unescapeGlob`：零堆分配完成字面量与转义模式的精确往返；`out` 不足时返回 `null`。
- `staticBaseDir`：提取第一个含未转义通配符段之前的静态目录前缀（如 `"src/http/*.cp"` → `"src/http"`、`"*.cp"` → `""`、`"a/b/c.cp"` → `"a/b/c.cp"`、`"/*.cp"` → `"/"`）。
- `isNegatedPattern` / `stripNegation`：识别并剥离前导否定符 `!`，准确区分转义字面量 `\!`。

## 使用示例

```cp
import { escapeGlob, unescapeGlob, staticBaseDir, hasGlobMagic } from "cap:glob_escape";

let buf: [128]u8 = [0; 128];
const esc = escapeGlob("src/[id]/file*.cp", buf[0..]).?; // "src/\\[id\\]/file\\*.cp"
// hasGlobMagic(esc) == false
// staticBaseDir("src/http/*.cp") == "src/http"
```
