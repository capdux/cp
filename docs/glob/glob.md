# `cap:glob` — 高性能通配符状态机与目录扫描引擎

> **上游溯源**: `/data/dev/bun/src/glob/glob.cp` (`matcher.cp`, `GlobWalker.cp`)  
> **模块路径**: `lib/glob/`  
> **导入协议**: `import { Glob, Matcher, GlobMatcher, GlobWalker, GlobWalkerOptions } from "cap:glob";`

---

## 一、 架构概述与设计原理

`cap:glob` 吸收自 Bun 原生 Glob 引擎（`bun/src/glob`），由**纯内存无锁通配符匹配器 (`Matcher`)** 与**高性能目录树遍历器 (`GlobWalker`)** 两部分组成：

1. **双层回溯状态机架构**：
   - **路径段级（Segment-level）递归与 `**` 跨层回溯**：将模式串与目标路径按分隔符 `/` 切分为路径段，针对跨目录通配符 `**`（Globstar）支持匹配 `0` 至 `N` 层子目录。
   - **单段字符级（Character-level）双指针线性回溯**：在单个文件名段内部，采用 `star_p` / `star_s` 双游标记录最近一次 `*` 的回溯锚点，合并连续 `*` 避免指数级回溯（ReDoS 免疫），实现 $O(N)$ 级极速匹配。
2. **花括号递归展开 (`{a,b,c}`)**: 支持嵌套与多分支集合展开（如 `lib/{path,glob,semver,which}/*.cp`），在栈上 `1024` 字节缓冲区内零堆分配完成组合求值。
3. **系统级目录遍历 (`GlobWalker`)**：直接基于底层 POSIX `opendir` / `readdir` 与 `d_type` (`DT_DIR`) 快筛遍历目录树，支持最大深度限制（`max_depth`）、隐藏文件过滤（`dot`）与仅文件模式（`only_files`）。

---

## 二、 Glob 语法支持矩阵

| 语法符号 | 名称 | 匹配语义说明 | 示例模式 | 匹配示例 | 不匹配示例 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `*` | 单层通配符 | 匹配单层目录内的任意个字符（不跨越 `/`） | `src/*.cp` | `src/main.cp` | `src/core/main.cp` |
| `**` | 跨层通配符 (Globstar) | 匹配零层、一层或多层任意子目录 | `lib/**/*.cp` | `lib/index.cp`, `lib/a/b/c.cp` | `lib/readme.md` |
| `?` | 单字符通配符 | 精确匹配除 `/` 以外的任意单个字符 | `file?.cp` | `file1.cp`, `fileA.cp` | `file12.cp`, `file.cp` |
| `[abc]` | 字符集匹配 | 匹配方括号中列举的任意单个字符 | `test_[abc].cp` | `test_a.cp`, `test_c.cp` | `test_d.cp` |
| `[a-z0-9]` | 字符范围匹配 | 匹配闭区间内的任意单个 ASCII 字符 | `v[0-9].txt` | `v1.txt`, `v9.txt` | `va.txt` |
| `[!a-z]` / `[^a-z]` | 反向字符集 | 匹配不在指定集合或区间内的任意单个字符 | `log_[!0-9].txt` | `log_a.txt` | `log_5.txt` |
| `{a,b,c}` | 花括号分支展开 | 匹配逗号分隔的任意一个子模式分支 | `*.{cp,zig,md}` | `main.cp`, `build.zig` | `cargo.toml` |
| `!pattern` | 前导取反匹配 | 对整体匹配结果取反（支持 `!!` 双重取反） | `!*.zig` | `main.cp` | `main.zig` |
| `\x` | 转义字符 | 将紧随其后的元字符作为普通字面量匹配 | `literal\*.txt` | `literal*.txt` | `literal_abc.txt` |

---

## 三、 `cap:glob` 与 `Bun.Glob` API 对照表

| 功能描述 | `cap:glob` API | `Bun.Glob` (JS/TS) 对照 | 说明 |
| :--- | :--- | :--- | :--- |
| **单次路径匹配** | `Glob.match(pattern, path)` | `new Bun.Glob(pattern).match(path)` | 纯函数零分配，直接返回 `bool` |
| **检测是否含通配符** | `Glob.hasMagic(pattern)` | `bun.glob.detectGlobSyntax(pattern)` | 检测是否包含未转义的 `*`, `?`, `[`, `{`, `!` |
| **一键目录扫描** | `Glob.scan(allocator, pattern, base_dir)` | `Array.fromAsync(glob.scan({ cwd }))` | 返回匹配的相对路径切片 `[][]const u8!` |
| **自定义配置扫描器** | `GlobWalker.initWithOptions(pat, dir, opts)` | `glob.scan({ cwd, dot, onlyFiles })` | 支持配置 `dot`, `only_files`, `max_depth` |

---

## 四、 代码示例

### 1. 纯内存通配符匹配 (`Glob.match` & `Glob.hasMagic`)

```typescript
import { Glob } from "cap:glob";

fn demoGlobMatch(): void {
    // 检测是否包含通配魔法字符
    let is_magic1 = Glob.hasMagic("src/main.cp");       // false
    let is_magic2 = Glob.hasMagic("src/**/*.{cp,zig}"); // true

    // 跨层目录与花括号组合匹配
    let m1 = Glob.match("lib/{path,glob,semver,which}/*.cp", "lib/semver/version.cp"); // true
    let m2 = Glob.match("lib/**/index.cp", "lib/index.cp");                            // true
    let m3 = Glob.match("lib/**/index.cp", "lib/a/b/index.cp");                        // true

    // 字符范围与反向字符类
    let m4 = Glob.match("chunk_[0-9][!a-z].bin", "chunk_4_.bin");                      // true
    let m5 = Glob.match("!*.tmp", "data.json");                                        // true
}
```

### 2. 文件系统目录树扫描 (`Glob.scan` & `GlobWalker`)

```typescript
const std = @import("std");
import { Glob, GlobWalker, GlobWalkerOptions } from "cap:glob";

fn demoDirectoryScan(): !void {
    const allocator = std.heap.page_allocator;

    // 方式一：快捷扫描指定目录下的匹配文件
    const files = try Glob.scan(allocator, "**/*.cp", "lib/path");
    defer {
        for (f in files) {
            allocator.free(f);
        }
        allocator.free(files);
    }

    // 方式二：使用自定义选项（包含隐藏文件、限制递归深度）
    let opts = GlobWalkerOptions{
        dot: true,
        only_files: true,
        max_depth: 4,
    };
    let walker = GlobWalker.initWithOptions("*.{cp,md}", "lib/glob", opts);
    const custom_files = try walker.walk(allocator);
    defer {
        for (f in custom_files) {
            allocator.free(f);
        }
        allocator.free(custom_files);
    }
}
```
