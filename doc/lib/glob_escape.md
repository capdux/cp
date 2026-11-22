# `cap:glob_escape` — Glob 通配符转义、静态前缀提取与否定模式分析库

> 参考来源：/data/dev/bun/docs/runtime/glob.mdx, /data/dev/bun/src/glob/glob.cp | 吸收日期：2026-11-22

## 概述

`cap:glob_escape` 吸收自 Bun 原生 Glob 引擎与文档规范（`docs/runtime/glob.mdx` 与 `src/glob/glob.cp`），提供纯 CP、零外部依赖、零堆分配的 Glob 元字符识别、转义/反转义、静态目录前缀提取（用于文件系统遍历剪枝）以及 `.gitignore` / Bun `Glob` 风格否定模式判定能力。

## 上游规范与行为契约

- **完整 Bun Glob 元字符集合**：与 Bun 官方文档一致，覆盖单字符 `?`、单段通配 `*`、跨目录 `**`、字符类 `[ab]`/`[^ab]`、大括号展开 `{a,b,c}`、分组 `()`、前导否定 `!` 以及反斜杠转义 `\`。
- **否定模式 vs 转义字面量**：
  - 前导 `!` 对匹配结果取反（例如 `!index.ts` 排除 `index.ts`）；
  - 前导 `\!` 表示匹配字面量感叹号开头的文件名（例如 `\!index.ts` 匹配 `!index.ts`）；`isNegatedPattern` 与 `stripNegation` 严格区分两者，不误将 `\!` 视为否定。
- **目录遍历静态前缀剪枝（`staticBaseDir`）**：在扫描大型项目树前，提取首个含未转义通配符路径段之前的静态目录路径，使文件系统遍历器可直接从该子目录起步（如 `"src/http/*.cp"` → `"src/http"`、`"/*.cp"` → `"/"`、`"*.cp"` → `""`、无通配符 `"a/b/c.cp"` → `"a/b/c.cp"`）。

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
- `escapeGlob` / `unescapeGlob`：零堆分配完成字面量与转义模式的精确往返；输出缓冲 `out` 不足时安全返回 `null`。
- `staticBaseDir`：提取第一个含未转义通配符段之前的静态目录前缀。
- `isNegatedPattern` / `stripNegation`：识别并剥离前导否定符 `!`，准确区分转义字面量 `\!`。

## 边界注意事项

1. **缓冲容量规划**：最坏情况下每个字符都是元字符，`escapeGlob` 最多需要 `2 * literal.len` 字节；容量不足返回 `null` 而不截断。
2. **孤立末尾反斜杠**：`hasGlobMagic` 与 `unescapeGlob` 对末尾悬空 `\` 做边界安全处理，不会越界读取下一字节。

## 使用示例

```cp
import { escapeGlob, unescapeGlob, staticBaseDir, hasGlobMagic, isNegatedPattern } from "cap:glob_escape";

let buf: [128]u8 = [0; 128];
const esc = escapeGlob("src/[id]/file*.cp", buf[0..]).?; // "src/\\[id\\]/file\\*.cp"
const has_magic = hasGlobMagic(esc);                     // false
const base = staticBaseDir("src/http/*.cp");             // "src/http"
const neg = isNegatedPattern("!index.cp");               // true
const lit = isNegatedPattern("\\!index.cp");             // false
* = unescapeGlob;
* = has_magic; * = base; * = neg; * = lit;
```
