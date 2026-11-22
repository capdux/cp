# `cap:path` — 跨平台工业级路径解析与缓冲区池标准库

> 参考来源：`/data/dev/bun/src/paths/paths.cp`, `/data/dev/bun/docs/guides/util/file-url-to-path.mdx`, `/data/dev/bun/docs/guides/util/path-to-file-url.mdx` | 吸收日期：2026-11-22
> **模块路径**: `lib/path/`  
> **导入协议**: `import { Path, PosixPath, WindowsPath, ParsedPath, EnvPath, PathBufferPool } from "cap:path";`

---

## 一、 架构概述与设计亮点

`cap:path` 吸收自 Bun 高性能路径引擎（`bun/src/paths`），采用**零动态内存分配（Zero-Allocation Caller-Buffer / Preallocated Buffer Pool）**与**POSIX / Windows 双轨确定性状态机**架构：

1. **双轨独立引擎 (`PosixPath` & `WindowsPath`)**：无需在目标宿主系统上运行，即可在任意平台上跨平台执行 POSIX（`/` 分隔符）与 Windows（`\` 与 `/` 混合分隔符、盘符 `C:\`、UNC 网络路径 `\\server\share`、Win32 设备命名空间 `\\?\C:\` / `\\?\UNC\server\share`、大小写不敏感比较）路径计算。
2. **宿主自适应门面 (`Path`)**：根据编译期目标操作系统（`@import("builtin").os.tag`）零开销静态分发至 `PosixPath` 或 `WindowsPath`，同时挂载 `Path.posix` 与 `Path.win32` 子命名空间。
3. **预分配缓冲区池 (`PathBufferPool`)**：提供 `4096` 字节（`MAX_PATH_BYTES`）的固定槽位缓冲池，避免高频路径拼接时的堆分配与碎片开销。
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
| **路径规范化** | `Path.normalize(p, buf)` | `path.normalize(p)` | 消解 `.`、`..` 与连续分隔符，折叠 UNC 与盘符根路径 |
| **路径拼接** | `Path.join(paths, buf)` | `path.join(...paths)` | 拼接多段路径并自动执行 `normalize` |
| **绝对路径解析** | `Path.resolve(paths, buf)` | `path.resolve(...paths)` | 从右向左寻找绝对锚点；Windows 支持盘符切换与根目录 `\foo` 回溯 |
| **相对路径计算** | `Path.relative(from, to, buf)` | `path.relative(from, to)` | 精确共同前缀回溯；正确处理 `.`、根路径与跨盘符回退 |
| **Win32 命名空间转换** | `Path.toNamespacedPath(p, buf)` | `path.toNamespacedPath(p)` | Windows 转为 `\\?\C:\...` 或 `\\?\UNC\server\share\...`；POSIX 原样返回 |
| **最长公共前缀** | `Path.longestCommonPath(paths)` | `bun.path.longestCommonPath` | 提取多条路径的公共父目录切片 |
| **结构化解构** | `Path.parse(p)` / `PosixPath.parse` / `WindowsPath.parse` | `path.parse(p)` | 返回 `ParsedPath { root, dir, base, ext, name }` |
| **结构化重组** | `Path.format(parsed, buf)` / `PosixPath.format` / `WindowsPath.format` | `path.format(pathObject)` | 完整支持 `dir`/`root` 与 `base`/`name`+`ext` 优先级重组 |

---

## 三、 模块文件拓扑

```text
lib/path/
├── index.cp         # 模块统一对外导出出口 (cap:path)
├── path.cp          # 跨平台自适应门面 Path、ParsedPath 与 formatParsedPath
├── posix.cp         # POSIX 规范路径实现 (PosixPath，含 parse/format/toNamespacedPath)
├── windows.cp       # Windows (Win32/UNC) 规范路径实现 (WindowsPath，含 parse/format/toNamespacedPath)
├── resolve.cp       # 核心算法实现：normalize, join, resolve, relative, toNamespacedPath, longestCommonPath
├── env_path.cp      # PATH 环境变量构建器与零拷贝分词迭代器 (EnvPath, EnvPathIterator)
└── buffer_pool.cp   # 4KB 预分配路径缓冲池 (PathBufferPool, MAX_PATH_BYTES)
```

---

## 四、 代码示例

### 1. POSIX 与 Windows 双轨路径解析与 `toNamespacedPath`

```typescript
const std = @import("std");
import { Path, PosixPath, WindowsPath, ParsedPath } from "cap:path";

fn demoDualTrack() {
    let buf: [1024]u8 = undefined;

    // 1. POSIX 路径操作
    let p_norm = PosixPath.normalize("/data/dev//cp/../bun/./src", &buf);
    // p_norm => "/data/dev/bun/src"

    let parts: [3][]const u8 = [ "/usr/local", "lib", "../bin/cap" ];
    let p_join = PosixPath.join(parts[0..3], &buf);
    // p_join => "/usr/local/bin/cap"

    let p_rel = PosixPath.relative("/data/dev/cp/lib/path", "/data/dev/cp/docs/path", &buf);
    // p_rel => "../../docs/path"

    // 2. Windows 路径操作（盘符、UNC 与 Win32 长路径命名空间）
    let w_norm = WindowsPath.normalize("//server/share//dir/../sub/file.txt", &buf);
    // w_norm => "\\\\server\\share\\sub\\file.txt"

    let w_ns_drive = WindowsPath.toNamespacedPath("C:\\Program Files\\CP\\bin", &buf);
    // w_ns_drive => "\\\\?\\C:\\Program Files\\CP\\bin"

    let w_ns_unc = WindowsPath.toNamespacedPath("\\\\fileserver\\public\\data.json", &buf);
    // w_ns_unc => "\\\\?\\UNC\\fileserver\\public\\data.json"

    let res_win: [2][]const u8 = [ "D:\\projects\\cp", "\\assets\\logo.png" ];
    let w_res = WindowsPath.resolve(res_win[0..2], &buf);
    // w_res => "D:\\assets\\logo.png"
}
```

### 2. 结构化路径解构与格式化 (`parse` & `format`)

```typescript
import { Path, PosixPath, WindowsPath, ParsedPath } from "cap:path";

fn demoParseFormat() {
    let parsed = PosixPath.parse("/home/developer/project/index.cp");
    // parsed.root => "/"
    // parsed.dir  => "/home/developer/project"
    // parsed.base => "index.cp"
    // parsed.name => "index"
    // parsed.ext  => ".cp"

    let buf: [512]u8 = undefined;
    let formatted = PosixPath.format(parsed, &buf);
    // formatted => "/home/developer/project/index.cp"

    // 仅指定 dir + name + ext（省略 base）
    let custom = ParsedPath{ dir: "C:\\src", name: "app", ext: ".cp" };
    let win_fmt = WindowsPath.format(custom, &buf);
    // win_fmt => "C:\\src\\app.cp"
}
```

---

## 五、 边界规则说明

1. **空路径归约**：`normalize("", buf)` 与 `join(&[], buf)` 均按照 POSIX 标准归约为当前目录 `"."`；`resolvePosix(&[], buf)` 归约为 `"/"`，`resolveWindows(&[], buf)` 归约为 `"C:\\"`。
2. **根目录越界防护**：绝对路径中的上级跳转不会越过根目录，例如 `PosixPath.normalize("/../../a", buf)` 归约为 `"/a"`，`WindowsPath.normalize("\\\\server\\share\\..\\..\\a", buf)` 归约为 `"\\\\server\\share\\a"`。
3. **相对路径锚点**：`relativePosix(".", "a/b", buf)` 返回 `"a/b"`，`relativePosix("a/b", ".", buf)` 返回 `"../.."`；当一方为绝对路径而另一方为相对路径时，相对路径自动锚定至根路径后再计算共同前缀。
4. **Windows 跨盘符 `relative`**：当 `from` 与 `to` 位于不同驱动器卷或不同 UNC 共享（如 `C:\foo` 与 `D:\bar`）时，直接返回规范化后的目标绝对路径 `"D:\\bar"`。
5. **上游 Bun 缓冲区大小与跨平台复用约定**：
   - 继承自 `/data/dev/bun/src/paths/paths.cp`：`MAX_PATH_BYTES` 为 `4096` 字节（`PathBuffer`），高频路径拼接/规范化场景推荐使用 `PathBufferPool` 复用预分配槽位，避免在热循环中反复分配大栈帧或堆内存。
   - `PosixPath` 与 `WindowsPath` 在任意宿主 OS 上行为完全确定，适用于构建工具、打包器（如解析 `tsconfig` 路径映射与跨平台产物路径）与 `file://` URL 双向转换辅助。
6. **`formatParsedPath` 字段优先级**：
   - 遵循 Node.js / Bun 规范：若同时提供 `dir` 与 `root`，优先使用 `dir`；若同时提供 `base` 与 `name`/`ext`，优先使用 `base` 忽略 `name`/`ext`；当 `ext` 不带前导 `.` 时按原样拼接。
