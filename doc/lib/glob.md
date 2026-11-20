# `cap:glob` — 高性能通配符状态机与目录扫描引擎

> **上游溯源**: `/data/dev/bun/src/glob/glob.cp` (`matcher.cp`, `GlobWalker.cp`)  
> **模块路径**: `lib/glob/`  
> **导入协议**: `import { Glob, Matcher, GlobMatcher, GlobMatchOptions, GlobWalker, GlobWalkerOptions } from "cap:glob";`

---

## 一、 架构概述与设计原理

`cap:glob` 吸收自 Bun 原生 Glob 引擎（`bun/src/glob`），由**纯内存无锁通配符匹配器 (`Matcher`)** 与**高性能目录树遍历器 (`GlobWalker`)** 两部分组成：

1. **双层回溯状态机架构**：
   - **路径段级（Segment-level）递归与 `**` 跨层回溯**：将模式串与目标路径按分隔符 `/` 切分为路径段（上限 256 段），自动折叠连续冗余的 `**/**` 防止指数回溯，针对 `**`（Globstar）支持匹配 `0` 至 `N` 层子目录。
   - **单段字符级（Character-level）双指针线性回溯**：在单个文件名段内部，采用 `star_p` / `star_s` 双游标记录最近一次 `*` 的回溯锚点，合并连续 `*` 避免指数级回溯（ReDoS 免疫），实现 $O(N)$ 极速匹配；支持 UTF-8 多字节码点对齐的 `?` 与 `[...]` 字符类匹配。
2. **完备花括号递归展开 (`{a,b,c}`)**:
   - 支持多层嵌套花括号（如 `{src,{lib,test}/unit}/*.{cp,zig}`）、空分支（`config{,.local}.json`）、字符类内逗号保护（`{[a,b],c}.txt`）以及反斜杠转义（`\{a,b\}`），在栈上 `4096` 字节缓冲区内零堆分配完成组合求值。
3. **路径规范化与匹配选项 (`GlobMatchOptions`)**：
   - 内置 `Glob.normalizePath`：自动消除 `./`、`//` 与 `seg/..` 相对回溯，并校验绝对路径与相对路径的匹配一致性；可选支持 `windows_paths`（将 `\` 规范化为 `/`）、`case_sensitive`（ASCII 大小写不敏感匹配）与 `dot`（控制前导点文件匹配）。
4. **系统级目录遍历 (`GlobWalker` & `Glob.scanSync`)**：
   - 基于底层 POSIX `opendir` / `readdir` 与 `d_type`（含 `DT_UNKNOWN` 与软链接回退探测）遍历目录树，支持 `dot`、`onlyFiles`（及兼容别名 `only_files`）、`absolute`、`case_sensitive`、`follow_symlinks`、`max_depth` 与 `cwd` 选项。

---

## 二、 Glob 语法支持矩阵

| 语法符号 | 名称 | 匹配语义说明 | 示例模式 | 匹配示例 | 不匹配示例 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `*` | 单层通配符 | 匹配单层目录内的任意个字符（不跨越 `/`） | `src/*.cp` | `src/main.cp` | `src/core/main.cp` |
| `**` | 跨层通配符 (Globstar) | 匹配零层、一层或多层任意子目录 | `lib/**/*.cp` | `lib/index.cp`, `lib/a/b/c.cp` | `lib/readme.md` |
| `?` | 单字符通配符 | 精确匹配除 `/` 以外的任意单个 Unicode 字符 | `??.txt` | `ab.txt`, `中文.txt` | `abc.txt`, `a.txt` |
| `[abc]` | 字符集匹配 | 匹配方括号中列举的任意单个字符（支持 `[]a]` 与 `[\]]`） | `test_[abc].cp` | `test_b.cp` | `test_d.cp` |
| `[a-z0-9]` | 字符范围匹配 | 匹配闭区间内的任意单个字符（受 `case_sensitive` 控制） | `v[0-9].txt` | `v1.txt`, `v9.txt` | `va.txt` |
| `[!a-z]` / `[^a-z]` | 反向字符集 | 匹配不在指定集合或区间内的任意单个字符 | `log_[!0-9].txt` | `log_a.txt` | `log_5.txt` |
| `{a,b,c}` | 花括号展开（支持嵌套） | 匹配逗号分隔的任意分支，支持嵌套 `{a,{b,c}}` 与空分支 | `cfg{,.local}.json` | `cfg.json`, `cfg.local.json` | `cfg.dev.json` |
| `!pattern` | 前导取反匹配 | 对整体匹配结果取反（支持 `!!` 双重取反与 `!{a,b}`） | `!*.{tmp,bak}` | `main.cp` | `cache.bak` |
| `\x` | 转义字符 | 将 `*`, `?`, `[`, `{`, `\` 等元字符作为普通字面量匹配 | `literal\*.txt` | `literal*.txt` | `literal_abc.txt` |

---

## 三、 `cap:glob` 与 `Bun.Glob` API 对照表

| 功能描述 | `cap:glob` API | `Bun.Glob` (JS/TS) 对照 | 说明 |
| :--- | :--- | :--- | :--- |
| **单次路径匹配** | `Glob.match(pattern, path)` | `new Bun.Glob(pattern).match(path)` | 默认启用路径规范化，零堆分配返回 `bool` |
| **带选项路径匹配** | `Glob.matchWithOptions(pattern, path, opts)` | `GlobMatchOptions` | 支持 `case_sensitive`, `dot`, `normalize`, `windows_paths` |
| **路径规范化** | `Glob.normalizePath(path, out_buf, win)` | `path.posix.normalize` | 消除 `.` / `..` / `//` |
| **检测是否含通配符** | `Glob.hasMagic(pattern)` | `bun.glob.detectGlobSyntax(pattern)` | 检测未转义的 `*`, `?`, `[`, `{` 及前导 `!` |
| **一键目录扫描** | `Glob.scan(allocator, pattern, base_dir)` | `Array.from(glob.scanSync({ cwd }))` | 默认 `dot: false, onlyFiles: true` |
| **带选项同步目录扫描** | `Glob.scanSync(allocator, pattern, base_dir, opts)` | `glob.scanSync({ cwd, dot, onlyFiles, absolute })` | 支持 `dot`, `onlyFiles`, `absolute`, `case_sensitive`, `follow_symlinks`, `max_depth` |
| **自定义扫描器实例** | `GlobWalker.initWithOptions(pat, dir, opts)` | `new Bun.Glob(pat)` | 支持 `walker.matchPath(rel)` 与 `walker.walk(alloc)` |

---

## 四、 代码示例

### 1. 纯内存通配符匹配、嵌套花括号与选项控制 (`Glob.match` & `Glob.matchWithOptions`)

```typescript
import { Glob, GlobMatchOptions } from "cap:glob";

fn demoGlobMatch() {
    // 1. 嵌套花括号、空分支与反转字符集
    let m1 = Glob.match("{src,{lib,test}/unit}/*.{cp,zig}", "lib/unit/a.cp"); // true
    let m2 = Glob.match("config{,.local}.json", "config.local.json");        // true
    let m3 = Glob.match("chunk_[0-9][^a-z].bin", "chunk_4_.bin");            // true
    let m4 = Glob.match("!*.{tmp,bak}", "data.json");                        // true

    // 2. 路径规范化与忽略大小写 / Windows 路径分隔符兼容
    let m5 = Glob.match("src/core/*.cp", "./src//sub/../core/./main.cp");   // true
    let opts = GlobMatchOptions{
        case_sensitive: false,
        dot: false,
        normalize: true,
        windows_paths: true,
    };
    let m6 = Glob.matchWithOptions("SRC/**/*.CP", "src\\sub\\Main.cp", opts); // true
    let m7 = Glob.matchWithOptions("*.cp", ".secret.cp", opts);               // false (dot=false)
}
```

### 2. 真实目录树扫描 (`Glob.scanSync` & `GlobWalkerOptions`)

```typescript
const std = @import("std");
import { Glob, GlobWalkerOptions } from "cap:glob";

fn demoDirectoryScan(): ! {
    const allocator = std.heap.page_allocator;

    // 同步扫描：包含隐藏文件与目录本身，并输出绝对路径
    const items = try Glob.scanSync(allocator, "**/*", "lib/glob", GlobWalkerOptions{
        dot: true,
        onlyFiles: false,
        absolute: true,
        case_sensitive: false,
        max_depth: 8,
    });
    defer {
        for (p of items) allocator.free(p);
        allocator.free(items);
    }
}
```
