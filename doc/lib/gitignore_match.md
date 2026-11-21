# `cap:gitignore_match` — Git `.gitignore` 规则集解析与路径匹配引擎

> 来源：吸收自 `/data/dev/bun/src/glob.zig`（`.gitignore` Rule Parser & Path Matcher）

`cap:gitignore_match` 提供纯 CP、零外部依赖、零堆分配的 `gitignore(5)` 规则行解析、单条规则匹配及整份 `.gitignore` 文件的顺序覆盖评估引擎。

## 核心类型与函数

### `GitignoreRule`

```cp
pub struct GitignoreRule {
    pattern: []const u8 = "",
    negated: bool = false,
    dir_only: bool = false,
    anchored: bool = false,
}
```

### `parseGitignoreLine`

```cp
pub fn parseGitignoreLine(raw_line: []const u8): GitignoreRule?
```

- 忽略空行与未转义 `#` 注释行；自动剥离尾部未转义空格（保留 `\ ` 转义空格）。
- 识别前导 `!` 否定规则与 `\!`/`\#` 字面量转义。
- 识别尾部 `/` 仅匹配目录（`dir_only = true`），以及前导/中间 `/` 为路径锚定（`anchored = true`）。

### `ruleMatchesPath`

```cp
pub fn ruleMatchesPath(rule: GitignoreRule, rel_path: []const u8, is_dir: bool): bool
```

- 支持单段通配 `*`（不跨 `/`）、`?`（不跨 `/`）、`[a-z]`/`[!0-9]` 字符类与 `\` 转义。
- 支持跨目录通配 `**/foo`、`a/**/b` 与 `abc/**`（仅匹配 `abc` 内部子路径）。
- 未锚定规则自动匹配路径 `basename`；锚定规则从仓库根开始匹配。

### `isIgnored`

```cp
pub fn isIgnored(gitignore_content: []const u8, rel_path: []const u8, is_dir: bool): bool
```

- 按行顺序扫描整份 `.gitignore`（自动剥离可选 UTF-8 BOM），严格遵循后出现规则覆盖前规则的 Git 语义，支持 `!pattern` 反忽略。

## 使用示例

```cp
import { isIgnored } from "cap:gitignore_match";

const rules = "*.log\n!important.log\nbuild/\n";
// isIgnored(rules, "app/debug.log", false) == true
// isIgnored(rules, "important.log", false) == false
// isIgnored(rules, "build", true) == true
// isIgnored(rules, "build", false) == false
```
