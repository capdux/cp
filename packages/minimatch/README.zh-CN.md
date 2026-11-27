[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# minimatch

> **源包溯源**：Clean-Room MIT 纯 CP 实现，对标 [`npm:minimatch`](https://github.com/isaacs/minimatch)（ISC）| CP 包版本：`1.0.0` | 依赖：`micromatch`、`picomatch`

零堆分配的 CP 原生 Glob 路径匹配工具库。支持递归通配（`**`）、通配符（`*`、`?`）、字符集、`#` 注释行、`!` 否定规则、`flip_negate`、`match_base`、忽略大小写以及批量路径过滤。

## 零成本迁移对照（TypeScript / JS → CP）

直接沿用原包名 `"minimatch"`，无需改名即可完成迁移：

| TypeScript / Node.js (`npm:minimatch`) | CP (`minimatch`) |
| :--- | :--- |
| `import { minimatch } from "minimatch";` | `import { minimatch, minimatchWithOptions, MinimatchOptions } from "minimatch";` |
| `minimatch("src/app.ts", "**/*.ts")` | `minimatch("src/app.ts", "**/*.ts")` |
| `minimatch("README.md", "*.MD", { nocase: true })` | `minimatchWithOptions("README.md", "*.MD", opts)` |
| `paths.filter(p => minimatch(p, pat))` | `minimatchList(paths, pat, opts, out_indices)` |

## 核心 API

### `MinimatchOptions`

```cp
class MinimatchOptions {
    dot: bool,         // 是否匹配以 '.' 开头的隐藏文件（默认 false）
    nocase: bool,      // 是否忽略 ASCII 大小写（默认 false）
    match_base: bool,  // 无 '/' 模式时仅匹配文件名（默认 false）
    noglobstar: bool,  // 禁用 '**' 跨目录匹配（默认 false）
    nonegate: bool,    // 将前导 '!' 视为普通字符（默认 false）
    nocomment: bool,   // 将前导 '#' 视为普通字符（默认 false）
    flip_negate: bool, // 反转否定模式的布尔返回值（默认 false）
    fn defaults(): MinimatchOptions;
}
```

### 函数列表

| 签名 | 说明 |
| :--- | :--- |
| `minimatch(path: string, pattern: string): bool` | 使用默认选项对单条路径执行 Glob 匹配。 |
| `minimatchWithOptions(path: string, pattern: string, opts: MinimatchOptions): bool` | 使用自定义 `MinimatchOptions` 匹配单条路径。 |
| `isCommentPattern(pattern: string, opts: MinimatchOptions): bool` | 判断是否为注释模式（`#` 开头且 `!opts.nocomment`）。 |
| `isNegatedPattern(pattern: string, opts: MinimatchOptions): bool` | 判断是否为否定模式（`!` 开头且 `!opts.nonegate`）。 |
| `minimatchList(paths: &[]string, pattern: string, opts: MinimatchOptions, out_indices: []usize): usize` | 单模式批量过滤，将命中下标写入 `out_indices` 并返回命中数。 |
| `minimatchFilterAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize` | 多模式联合过滤（复用 `micromatch::mmFilter`）。 |
| `minimatchRejectAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize` | 多模式反向排除（复用 `micromatch::mmNot`）。 |

## 实战示例

### 基础匹配与选项控制

```cp
import { minimatch, minimatchWithOptions, MinimatchOptions } from "minimatch";

fn demoBasic() {
    const ok1 = minimatch("src/net/http.cp", "**/*.cp"); // true
    const ok2 = minimatch("src/net/http.cp", "!*.js");   // true

    let opts = MinimatchOptions.defaults();
    opts.nocase = true;
    opts.dot = true;
    const ok3 = minimatchWithOptions(".Env", "*.env", opts); // true
    * = ok1; * = ok2; * = ok3;
}
```

### 批量过滤与注释/否定规则

```cp
import {
    MinimatchOptions,
    minimatchList,
    minimatchFilterAll,
    isCommentPattern,
} from "minimatch";

fn demoBatch() {
    const opts = MinimatchOptions.defaults();
    const is_comment = isCommentPattern("# ignore rule", opts); // true

    const paths = ["src/a.cp", "src/b.txt", "test/a.cp"];
    let out: [8]usize = undefined;
    const n = minimatchList(paths[0..], "**/*.cp", opts, out[0..]);
    // n == 2, out[0] == 0, out[1] == 2

    const pats = ["**/*.cp", "!test/**"];
    const m = minimatchFilterAll(paths[0..], pats[0..], opts, out[0..]);
    // m == 1, out[0] == 0 ("src/a.cp")
    * = is_comment; * = n; * = m;
}
```

## 性能与安全亮点

- **零堆分配**：全程仅借用 `&[]string` 输入切片，批量结果以索引写入调用方 `out_indices: []usize`，无中间数组分配。
- **线性匹配引擎**：底层复用 `picomatch`/`micromatch` 确定性状态机，无需编译正则，免疫回溯爆炸。
- **借用与越界安全**：所有输出写入严格受 `out_indices.len` 保护，无越界风险。

## 许可协议

MIT
