# `cap:path` — 工业级跨平台路径标准库

`cap:path` 是 CP 语言核心的基础设施标准库之一，全量对标并吸收自 Bun 原生路径运行时资产（`bun/src/paths/`，涵盖 `Path.cp`, `resolve_path.cp`, `EnvPath.cp`, `path_buffer_pool.cp`, `paths.cp`）。该模块提供了从底层的定长零堆分配缓冲池（`PathBufferPool`）、POSIX 与 Windows 双引擎路径状态机，到高层的跨平台统一路径门面（`Path`）、PATH 环境变量构建器（`EnvPath`）与结构化解析器（`ParsedPath`）的完整工业级支持。

---

## 一、 模块结构与谱系映射

所有文件第 1 行均严格标注对应的 Bun 原生源码谱系标记：

| 文件 | 谱系来源 | 核心职能 |
| :--- | :--- | :--- |
| `lib/path/buffer_pool.cp` | `bun/src/paths/path_buffer_pool.cp` | `PathBufferPool` 零堆分配定长路径缓冲池（4096 字节）、`PathBuffer` |
| `lib/path/env_path.cp` | `bun/src/paths/EnvPath.cp` | `EnvPath` 环境变量（`PATH`）字符串构建器、`EnvPathIterator` 分隔符迭代器 |
| `lib/path/resolve.cp` | `bun/src/paths/resolve_path.cp` | 工业级 `normalize`、`join`、`resolve`、`relative` 与 `longestCommonPath` 核心状态机 |
| `lib/path/posix.cp` | `bun/src/paths/Path.cp` | POSIX 规范路径静态工具类（`PosixPath`） |
| `lib/path/windows.cp` | `bun/src/paths/Path.cp` | Windows 规范路径静态工具类（`WindowsPath`，支持盘符、UNC 与大小写不敏感匹配） |
| `lib/path/path.cp` | `bun/src/paths/Path.cp` | 跨平台统一门面 `Path`、结构化路径 `ParsedPath`、`parse` 与 `format` |
| `lib/path/index.cp` | `bun/src/paths/paths.cp` | 模块对外统一聚合导出入口 |

---

## 二、 核心组件与 API 规范

### 1. 跨平台统一门面 (`Path`)
根据当前编译目标平台（`@import("builtin").os.tag == .windows`）自动路由至 `WindowsPath` 或 `PosixPath`，同时显式保留 `.posix` 与 `.win32` 命名空间：

- **常量**：
  - `Path.sep`: 路径分隔符（POSIX 为 `'/'`，Windows 为 `'\\'`）
  - `Path.sep_str`: 路径分隔符字符串（`"/"` 或 `"\\"`）
  - `Path.delimiter`: 环境变量路径分隔符（POSIX 为 `':'`，Windows 为 `';'`）
  - `Path.posix`: `PosixPath` 实例与命名空间
  - `Path.win32`: `WindowsPath` 实例与命名空间
- **路径操作函数**：
  - `isAbsolute(path: []const u8): bool`: 判断是否为绝对路径
  - `basename(path: []const u8, ext: ([]const u8)?): []const u8`: 获取文件名（支持可选剥离指定拓展名）
  - `dirname(path: []const u8): []const u8`: 获取父级目录路径
  - `extname(path: []const u8): []const u8`: 获取拓展名（以 `.` 开头，点文件如 `.gitignore` 视为无拓展名返回空串）
  - `normalize(path: []const u8, buf: []u8): []const u8`: 路径规范化消解（处理 `.`、`..` 与冗余斜杠）
  - `join(paths: []const []const u8, buf: []u8): []const u8`: 拼接多段路径并执行规范化
  - `resolve(paths: []const []const u8, buf: []u8): []const u8`: 从右至左解析出绝对路径（未显式提供绝对路径时默认以根目录绝对化）
  - `relative(from_path: []const u8, to_path: []const u8, buf: []u8): []const u8`: 计算两个路径之间的相对路径（跨驱动器时安全回退）
  - `longestCommonPath(paths: []const []const u8): []const u8`: 计算一组路径的最长公共目录前缀
  - `parse(path: []const u8): ParsedPath`: 结构化拆解路径为 `root`, `dir`, `base`, `ext`, `name`
  - `format(p: ParsedPath, buf: []u8): []const u8`: 重新序列化 `ParsedPath`

### 2. POSIX 与 Windows 双规范引擎 (`PosixPath` / `WindowsPath`)
提供严格遵循各平台规范的独立实现，便于跨平台网络服务或转译器在任意宿主操作系统上精准模拟目标操作系统路径行为：

- **Windows 特有支持**：
  - `isDriveLetter(c: u8): bool`: 检查字符是否为盘符字母（`A-Z`, `a-z`）
  - `volumeName(path: []const u8): []const u8`: 提取盘符（如 `C:`）或 UNC 主机共享卷（如 `\\server\share`）
  - `isSeparator(c: u8): bool`: 兼容识别 `'\\'` 与 `'/'` 两种斜杠
  - 大小写不敏感比较与相对路径跨驱动器安全边界处理

### 3. 环境变量路径构建器 (`EnvPath` / `EnvPathIterator`)
针对 `PATH` 等由分隔符拼接的系统环境变量，提供动态构建与零拷贝流式迭代器：

- `EnvPath.init(allocator)`: 初始化构建器
- `EnvPath.initCapacity(allocator, cap)`: 预分配容量初始化
- `EnvPath.append(input, custom_delimiter)`: 追加目录项，自动剥离边缘分隔符并用指定/平台默认分隔符连接
- `EnvPath.slice()`: 获取底层连接完整的字符串切片
- `EnvPath.count()`: 获取当前有效路径条目数量
- `EnvPath.split(path_str, custom_delimiter)`: 创建 `EnvPathIterator`
- `EnvPathIterator.next()`: 惰性迭代下一段非空路径条目

### 4. 零堆分配路径缓冲池 (`PathBufferPool`)
提供最大 `4096` 字节（`MAX_PATH_BYTES`）的高速静态缓冲槽复用机制，消除高频路径操作下的堆内存分配开销：

- `PathBufferPool.get(): *PathBuffer`: 从缓冲池检出一个可用定长缓冲
- `PathBufferPool.put(buf: *const PathBuffer)`: 将缓冲归还缓冲池供后续重用

---

## 三、 使用示例

### 1. 基础路径解析与拆解
```typescript
import { Path, PosixPath, WindowsPath } from "cap:path";

// POSIX
let base = PosixPath.basename("/var/log/syslog", null); // "syslog"
let base_no_ext = PosixPath.basename("/app/index.ts", ".ts"); // "index"
let dir = PosixPath.dirname("/var/log/syslog"); // "/var/log"
let ext = PosixPath.extname("archive.tar.gz"); // ".gz"

// Windows
let is_abs = WindowsPath.isAbsolute("C:\\Users\\Admin"); // true
let win_dir = WindowsPath.dirname("C:\\dir\\file.exe"); // "C:\\dir"
```

### 2. 路径消解与相对路径
```typescript
let buf: [1024]u8 = undefined;

// 规范化消解
let norm = PosixPath.normalize("/foo/bar//baz/asdf/quux/..", &buf);
// norm => "/foo/bar/baz/asdf"

// 路径多段拼接
let parts: [3][]const u8 = [ "/usr", "local", "bin/cp" ];
let joined = PosixPath.join(parts[0..3], &buf);
// joined => "/usr/local/bin/cp"

// 相对路径推导
let rel = PosixPath.relative("/data/dev/cp", "/data/dev/bun/src", &buf);
// rel => "../bun/src"
```

### 3. PATH 环境变量构建与迭代
```typescript
import { EnvPath } from "cap:path";
const std = @import("std");

let env = EnvPath.init(std.heap.page_allocator);
defer env.deinit();

try env.append("/usr/bin", ':');
try env.append("/usr/local/bin", ':');
try env.append("/bin", ':');
// env.slice() => "/usr/bin:/usr/local/bin:/bin"

let iter = EnvPath.split(env.slice(), ':');
while (item := iter.next()) {
    // 依次迭代各路径项
}
```

### 4. 零分配缓冲池复用
```typescript
import { PathBufferPool, PosixPath } from "cap:path";

let buf_ptr = PathBufferPool.get();
defer PathBufferPool.put(buf_ptr);

let parts: [2][]const u8 = [ "/home/user", "project" ];
let resolved = PosixPath.join(parts[0..2], buf_ptr);
```

---

## 四、 自动化测试与质量保障

已在 `cap test` 原生测试引擎下完成全量校验：

1. `cap test cap/test/test_path_url.cp`（回归校验）：**4/4 PASS**
   - POSIX 基础路径操作（basename, dirname, extname, isAbsolute, normalize, join）
   - Windows 基础路径操作（drive letter, UNC, basename, dirname）
   - URLSearchParams 与 WHATWG URL 集成测试

2. `cap test cap/test/test_path_complete.cp`（高覆盖度综合测试）：**5/5 PASS**
   - 1. POSIX path algorithms: normalize, join, resolve, relative, longestCommonPath
   - 2. Windows path algorithms: drive letter, UNC, normalize, join, resolve, relative
   - 3. Path.parse and Path.format
   - 4. EnvPath: PATH environment variable builder & delimiter-aware iterator
   - 5. PathBufferPool: zero-allocation preallocated path buffers
