# `cap:cssmodules` — 纯 CP 零分配 CSS Modules 局部类名哈希与 `composes` 解析库

> 参考来源：`/data/dev/bun/src/css/css_modules.cp`、`/data/dev/bun/docs/bundler/css.mdx` | 吸收日期：2026-11-22

100% 纯 CP、零外部依赖、零堆分配的 CSS Modules 命名模板编译、确定性 CSS 合法标识符哈希与 `composes` 依赖引用解析库，吸收自 Bun 原生 CSS 引擎的 `css_modules.cp` 模块。

## 导入方式

```cp
import {
    CssModuleConfig,
    PatternKind,
    PatternSegment,
    Pattern,
    ReferenceKind,
    CssModuleRef,
    isValidCssIdent,
    extractFileStemSanitized,
    computeModuleHash,
    scopeClassName,
    parseComposesValue,
} from "cap:cssmodules";
```

## API 契约

### 1. 配置与命名模板（`CssModuleConfig` / `Pattern`）

- `CssModuleConfig`：`dashed_idents`（默认 `false`）、`animation`（`true`）、`grid`（`true`）、`custom_idents`（`true`）；提供 `CssModuleConfig.default()`
- `PatternKind` (`literal`, `name`, `local`, `hash`)、`PatternSegment`
- `Pattern`（固定最大 16 段零分配模板）：
  - `Pattern.default()`：对齐 Bun 默认模板 `[local]_[hash]`
  - `Pattern.parse(template): Pattern?`：支持 `[name]`、`[local]`、`[hash]` 与任意字面量组合
  - `startsWithHash(): bool`：判断是否以 `[hash]` 开头（用于决定是否启用首数字 `_` 保护）
  - `format(hash_str, file_path, local, out: *[]u8): ([]const u8)?`

### 2. 确定性哈希与作用域编译

- `computeModuleHash(input, at_start, out: *[]u8): ([]const u8)?`
  基于 Wyhash (32-bit little-endian) + 无填充 URL-safe Base64（6 字符）；当 `at_start == true` 且原始哈希以数字或 `-<digit>` 开头时自动前置 `_`（7 字符）。
- `scopeClassName(pattern, file_path, local, out: *[]u8): ([]const u8)?`
  一键零分配编译局部类名。
- `extractFileStemSanitized(path, out: *[]u8): ([]const u8)?`
  提取基础文件名 stem（兼容 `/` 与 `\`），并将剩余 `.` 替换为 `-`（如 `src/Button.module.css` -> `Button-module`）。
- `isValidCssIdent(ident): bool`：校验合法无转义 ASCII CSS 标识符。

### 3. `composes` 声明解析

- `ReferenceKind` (`local`, `global`, `dependency`)、`CssModuleRef` (`kind`, `name`, `specifier`, `eql`)
- `parseComposesValue(value, out: *[]CssModuleRef): usize?`
  零拷贝解析 `composes:` 值：本地引用（`btn primary`）、全局引用（`reset from global`）及跨文件依赖引用（`card shadow from './base.module.css'`）。

## 上游行为契约与边界注意事项

1. **CSS 标识符首字符合法性保护（`at_start`）**：
   - 与 Bun `css_modules.hash(..., at_start)` 完全一致：CSS 规范禁止未转义类选择器以数字或 `-<digit>` 开头，因此当模板以 `[hash]` 起始时自动前置 `_`，保证输出恒满足 `isValidCssIdent(...) == true`。
2. **`[name]` 占位符中的点号替换**：
   - 文件名如 `Button.module.css` 剥离最终 `.css` 后仍含 `.`，`extractFileStemSanitized` 与 Bun `replace_dots` 一致将其清洗为 `Button-module`，防止 `.` 被 CSS 解析器误判为链式类选择器。
3. **严格 `composes` 语法校验**：
   - 拒绝空声明、`from` 后缺失目标、未闭合引号、非法非 `global` 裸标识符或尾部多余垃圾字符，并严格检查调用方 `out` 切片容量。

## 示例

```cp
import { Pattern, scopeClassName, parseComposesValue, CssModuleRef } from "cap:cssmodules";

fn demo() {
    if (pat := Pattern.parse("[name]__[local]--[hash]")) {
        let out_buf: [128]u8 = [0; 128];
        let out_s: []u8 = out_buf[0..];
        if (scoped := scopeClassName(pat, "src/Button.module.css", "primary", *out_s)) {
            * = scoped; // e.g. "Button-module__primary--<6-char-hash>"
        }
    }

    let refs_buf: [4]CssModuleRef = [{ kind: .local, name: "", specifier: "" }; 4];
    let refs: []CssModuleRef = refs_buf[0..];
    if (n := parseComposesValue("card shadow from './base.module.css'", *refs)) {
        * = n; // 2
    }
}
```
