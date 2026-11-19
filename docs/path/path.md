# `cap:path` — 跨平台工业级路径解析与缓冲区池标准库

> **上游溯源**: `/data/dev/bun/src/paths/` (`paths.cp`, `Path.cp`, `resolve_path.cp`, `EnvPath.cp`, `path_buffer_pool.cp`)  
> **模块路径**: `lib/path/`  
> **导入协议**: `import { Path, PosixPath, WindowsPath, ParsedPath, EnvPath, PathBufferPool } from "cap:path";`

---

## 一、 架构概述与设计亮点

`cap:path` 吸收自 Bun 高性能路径引擎（`bun/src/paths`），采用**零动态内存分配（Zero-Allocation Caller-Buffer / Preallocated Buffer Pool）**与**POSIX / Windows 双轨确定性状态机**架构：

1. **双轨独立引擎 (`PosixPath` & `WindowsPath`)**：无需在目标宿主系统上运行，即可在任意平台上跨平台执行 POSIX（`/` 分隔符）与 Windows（`\` 与 `/` 混合分隔符、盘符 `C:`、UNC 网络路径 `\\server\share`、大小写不敏感比较）路径计算。
2. **宿主自适应门面 (`Path`)**：根据编译期目标操作系统（`@import("builtin").os.tag`）零开销静态分发至 `PosixPath` 或 `WindowsPath`，同时挂载 `Path.posix` 与 `Path.win32` 子命名空间。
3. **预分配缓冲区池 (`PathBufferPool`)**：提供 `4096` 字节（`MAX_PATH_BYTES`）的固定槽位环形/位图缓冲池，避免高频路径拼接时的堆分配与碎片开销。
4. **`PATH` 环境变量引擎 (`EnvPath`)**：支持跨平台分隔符（POSIX `:` / Windows `;`）的环境变量构建、修剪与零拷贝迭代器切分。

---

## 二、 核心 API 矩阵与 Node.js / Bun 对照表

| 功能分类 | `cap:path` API | Node.js `node:path` / Bun 对照 | 内存模型与说明 |
| :--- | :--- | :--- | :--- |
| **分隔符常量** | `Path.sep`, `Path.sep_str`, `Path.delimiter` | `path.sep`, `path.delimiter` | POSIX 为 `'/'`, `':'`；Windows 为 `'\\'`, `';'` |
| **绝对路径判定** | `Path.isAbsolute(p)` | `path.isAbsolute(p)` | 零分配切片扫描；Windows 支持 `C:\` 与 `\\UNC\share` |
| **获取目录名** | `Path.dirname(p)` | `path.dirname(p)` | 零拷贝返回原字符串子切片，空路径或无目录返回 `"."` |
| **获取文件名** | `Path.basename(p, ext?)` | `path.basename(p[, ext])` | 零拷贝返回子切片；Windows 下 `ext` 剥离大小写不敏感 |
| **获取扩展名** | `Path.extname(p)` | `path.extname(p)` | 零拷贝返回子切片；`.bashrc`、`..`、`.` 均返回 `""` |
| **路径规范化** | `Path.normalize(p, buf)` | `path.normalize(p)` | 消解 `.`、`..` 与连续分隔符，写入调用方 `buf` |
| **路径拼接** | `Path.join(paths, buf)` | `path.join(...paths)` | 拼接多段路径并自动执行 `normalize` |
| **绝对路径解析** | `Path.resolve(paths, buf)` | `path.resolve(...paths)` | 从右向左寻找首个绝对路径锚点并规范化 |
| **相对路径计算** | `Path.relative(from, to, buf)` | `path.relative(from, to)` | 计算从 `from` 到 `to` 的最短相对跳转路径 |
| **最长公共前缀** | `Path.longestCommonPath(paths)` | `bun.path.longestCommonPath` | 提取多条路径的公共父目录切片 |
| **结构化解构** | `Path.parse(p)` | `path.parse(p)` | 返回 `ParsedPath { root, dir, base, ext, name }` |
| **结构化重组** | `Path.format(parsed, buf)` | `path.format(pathObject)` | 将 `ParsedPath` 序列化回完整路径字符串 |

---

## 三、 模块文件拓扑

```text
lib/path/
├── index.cp         # 模块统一对外导出出口 (cap:path)
├── path.cp          # 跨平台自适应门面 Path 与 ParsedPath 定义
├── posix.cp         # POSIX 规范路径实现 (PosixPath)
├── windows.cp       # Windows (Win32/UNC) 规范路径实现 (WindowsPath)
├── resolve.cp       # 核心算法实现：normalize, join, resolve, relative, longestCommonPath
├── env_path.cp      # PATH 环境变量构建器与零拷贝分词迭代器 (EnvPath, EnvPathIterator)
└── buffer_pool.cp   # 4KB 预分配路径缓冲池 (PathBufferPool, MAX_PATH_BYTES)
```

---

## 四、 代码示例

### 1. POSIX 与 Windows 双轨路径解析

```typescript
const std = @import("std");
import { Path, PosixPath, WindowsPath } from "cap:path";

fn demoDualTrack(): void {
    let buf: [1024]u8 = undefined;

    // 1. POSIX 路径操作
    let p_norm = PosixPath.normalize("/data/dev//cp/../bun/./src", &buf);
    // p_norm => "/data/dev/bun/src"

    let parts: [3][]const u8 = [ "/usr/local", "lib", "../bin/cap" ];
    let p_join = PosixPath.join(parts[0..3], &buf);
    // p_join => "/usr/local/bin/cap"

    let p_rel = PosixPath.relative("/data/dev/cp/lib/path", "/data/dev/cp/docs/path", &buf);
    // p_rel => "../../docs/path"

    // 2. Windows 路径操作（在 Linux/macOS 上同样可精确计算 Windows 路径）
    let w_norm = WindowsPath.normalize("C:\\Users\\Admin\\..\\Public\\.\\Documents", &buf);
    // w_norm => "C:\\Users\\Public\\Documents"

    let w_base = WindowsPath.basename("C:\\Windows\\System32\\CMD.EXE", ".exe");
    // w_base => "CMD" (Windows 扩展名匹配自动忽略大小写)

    let w_rel = WindowsPath.relative("C:\\dev\\cp\\lib", "C:\\dev\\bun\\src", &buf);
    // w_rel => "..\\..\\bun\\src"
}
```

### 2. 结构化路径解构与格式化 (`parse` & `format`)

```typescript
import { Path } from "cap:path";

fn demoParseFormat(): void {
    let parsed = Path.parse("/home/developer/project/index.cp");
    // parsed.root => "/"
    // parsed.dir  => "/home/developer/project"
    // parsed.base => "index.cp"
    // parsed.name => "index"
    // parsed.ext  => ".cp"

    let buf: [512]u8 = undefined;
    let formatted = Path.format(parsed, &buf);
    // formatted => "/home/developer/project/index.cp"
}
```

### 3. `EnvPath` 环境变量处理与 `PathBufferPool` 零分配池

```typescript
const std = @import("std");
import { EnvPath, PathBufferPool, PosixPath } from "cap:path";

fn demoPoolAndEnv(): !void {
    // 从缓冲池借用 4KB 路径缓冲区
    let pooled_buf = PathBufferPool.get();
    defer PathBufferPool.put(pooled_buf);

    let normalized = PosixPath.normalize("/var/log//nginx/../app/./error.log", pooled_buf);
    // normalized => "/var/log/app/error.log"

    // 构建与切分 PATH 环境变量
    let env = EnvPath.init(std.heap.page_allocator);
    defer env.deinit();

    try env.append("/usr/local/bin", ':');
    try env.append("/usr/bin", ':');

    let iter = EnvPath.split(env.slice(), ':');
    while (entry := iter.next()) {
        // 依次产出 "/usr/local/bin", "/usr/bin"
    }
}
```

---

## 五、 边界规则说明

1. **空路径归约**：`normalize("", buf)` 与 `join(&[], buf)` 均按照 POSIX 标准归约为当前目录 `"."`。
2. **根目录越界防护**：绝对路径中的上级跳转不会越过根目录，例如 `PosixPath.normalize("/../../a", buf)` 归约为 `"/a"`；相对路径则保留前导 `..`，如 `PosixPath.normalize("../../a", buf)` 保持 `"../../a"`。
3. **隐藏文件与特殊目录扩展名**：
   - `extname(".bashrc")` 返回 `""`（以点开头的隐藏配置文件无扩展名）。
   - `extname(".")` 与 `extname("..")` 均返回 `""`。
   - `extname("archive.tar.gz")` 返回最后一个点之后的 `".gz"`。
4. **Windows 跨盘符 `relative`**：当 `from` 与 `to` 位于不同驱动器卷（如 `C:\foo` 与 `D:\bar`）时，无法通过 `..` 相对跳转，`WindowsPath.relative` 直接返回规范化后的目标绝对路径 `"D:\\bar"`。
