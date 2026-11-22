# `cap:editorconfig` — 零堆分配 `.editorconfig` 解析与文件属性计算库

> 参考来源：/data/dev/bun/src/cli/cli.zig | 吸收日期：2026-11-22

## 概述

`cap:editorconfig` 吸收自 Bun CLI 配置与 INI/Glob 解析工具体系（`src/cli/cli.zig`），提供纯 CP、零外部依赖、零堆分配的 `.editorconfig` 规范解析器与目标文件属性计算引擎，适用于代码格式化器、LSP 服务器、静态检查器与编辑器插件。

## 规范契约与行为边界

- **节头 Glob 匹配（`matchSectionPattern`）**：
  - 完整支持 `.editorconfig` 节头通配符：`*`（不跨 `/`）、`**`（跨目录）、`?`（单字符）以及 `{a,b,c}` 花括号多选展开；
  - 不含 `/` 的节头（如 `[*.cp]`、`[Makefile]`）自动匹配任意子目录中的对应文件名；含 `/` 的节头按相对路径匹配。
- **级联覆盖与 `unset` 清除语义**：
  - 按文件自上而下顺序应用所有匹配节的属性，后续匹配节覆盖先前同名属性；
  - 属性值设为 `unset`（大小写不敏感）时显式清除先前节设置的对应配置，恢复为未指定状态；
  - 顶层（首个节头之前）的 `root = true` 用于告知调用方停止继续向上级目录查找父 `.editorconfig`。
- **有效制表符宽度推导（`effectiveTabWidth`）**：当未显式指定 `tab_width` 但指定了 `indent_size` 时，按 EditorConfig 规范自动回退至 `indent_size`。

## 导出 API

| 符号 | 说明 |
| :--- | :--- |
| `matchSectionPattern(pattern, rel_path): bool` | 匹配 `.editorconfig` 节头通配符与相对路径 |
| `IndentStyle` | `unset` / `space` / `tab` |
| `EndOfLine` | `unset` / `lf` / `crlf` / `cr` |
| `Charset` | `unset` / `latin1` / `utf8` / `utf8_bom` / `utf16be` / `utf16le` |
| `EditorConfigProps` | 聚合解析后的全部属性，提供 `empty()` 与 `effectiveTabWidth()` |
| `resolveEditorConfig(content, rel_path): EditorConfigProps` | 零堆分配解析并计算 `rel_path` 的最终生效属性 |

## 使用示例

```cp
import { resolveEditorConfig, matchSectionPattern, IndentStyle } from "cap:editorconfig";

fn demo() {
    const cfg = "root = true\n[*]\nindent_style = space\nindent_size = 4\n[Makefile]\nindent_style = tab\n";
    const props = resolveEditorConfig(cfg, "src/main.cp");
    // props.indent_style == IndentStyle.space
    const m = matchSectionPattern("*.{cp,zig}", "lib/core/main.cp"); // true
    * = props;
    * = m;
    * = IndentStyle.tab;
}
```
