# CP 标准库：双模文件系统、目录操作与原子写入手册 (`cap:fs`)

## 一、 架构总览与设计准则

`cap:fs` 是 CP 语言官方内置的高阶文件系统标准库，采用 **双模编译期分发（`@is_nest`）** 架构：在 BEAM/Nest 分布式调度模式下，异步文件操作通过 `SystemAction.FS_*` 挂起当前协程并委托给异步 I/O 网关，绝不阻塞调度线程；在 Native 原生模式下，零开销直通编译器内建 `@fs_*` 原语。同时内置 `FileSystem` 原子写与线程安全临时路径生成工具，防止断电或并发写入造成文件半截损坏。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP 用户层文件 API (`cap:fs`)                       │
├──────────────────────┬─────────────────────────┬────────────────────────┤
│ 双模异步 I/O          │  同步文件与目录操作       │  原子写与临时路径引擎   │
│ readFile / writeFile │  readFileSync /         │  tmpDir()              │
│ removeAsync /        │  writeFileSync          │  tmpPath(pre, suf, buf)│
│ rmdirAsync /         │  mkdir / rmdir / remove │  writeAtomic(dst, data)│
│ renameAsync /        │  exists / rename /      │    (write tmp -> rename│
│ copyFileAsync        │  copyFile / statSize /  │     防半写损坏)         │
│                      │  isDirectory            │                        │
├──────────────────────┴─────────────────────────┴────────────────────────┤
│    @is_nest == true: @yield_to_beam(SystemAction.FS_*)                  │
│    @is_nest == false: Native @fs_* 内建原语 + std.atomic + clock_gettime │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/fs.cp` | `// Source: Custom (CP native stdlib; dual-mode @is_nest & atomic fs)` | 双模异步/同步文件读写、目录管理、`statSize`/`isDirectory` 元信息探测、`FileSystem.tmpDir`/`tmpPath`/`writeAtomic` |

---

## 二、 核心 API 速查

### 1. 异步与同步文件读写

| 函数签名 | 行为说明 |
| :--- | :--- |
| `readFile(path: string): string` (`async`) | 双模异步读取全文（BEAM 模式走 `FS_READ`，原生模式走 `@fs_read_file`） |
| `writeFile(path: string, content: string)` (`async`) | 双模异步写入全文 |
| `readFileSync(path: string): string` | 同步读取文件全部内容 |
| `writeFileSync(path: string, content: string)` | 同步写入（创建或截断覆盖） |

### 2. 目录、元信息与变更操作

| 函数签名 | 行为说明 |
| :--- | :--- |
| `mkdir(path)` / `rmdir(path)` / `rmdirAsync(path)` | 创建或删除目录 |
| `exists(path): bool` | 检查路径（文件或目录）是否存在 |
| `remove(path)` / `removeAsync(path)` | 删除文件 |
| `rename(oldPath, newPath)` / `renameAsync(...)` | 原子重命名或移动路径 |
| `copyFile(src, dest)` / `copyFileAsync(src, dest)` | 复制文件 |
| `statSize(path): i32` | 查询文件字节大小 |
| `isDirectory(path): bool` | 判断目标路径是否为目录 |

### 3. 原子写与无锁临时路径（`FileSystem` / 顶层快捷导出）

- **`tmpDir(): []const u8`**：依次探测环境变量 `TMPDIR` -> `TEMP` -> `TMP`，回退至 `"/tmp"`（Windows 为 `"C:\\Temp"`）。
- **`tmpPath(prefix, suffix, dest): []const u8`**：使用原子递增计数器 `std.atomic.Value(u64)` 异或单调纳秒时钟（Linux `clock_gettime(.MONOTONIC)`），零堆分配生成高熵唯一临时路径。
- **`writeAtomic(dest, content)`**：先写入同目录隐藏临时文件 `.{basename}.tmp_{id}`，再通过 POSIX 原子 `rename` 替换目标文件，保证并发读者永不看到半写入状态。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    tmpDir,
    tmpPath,
    writeFileSync,
    readFileSync,
    writeAtomic,
    copyFile,
    rename,
    exists,
    statSize,
    isDirectory,
    mkdir,
    remove,
    rmdir,
} from "cap:fs";

pub fn demoFs() {
    // 1. 生成高熵临时路径并测试原子写入
    let p1_buf: [256]u8 = undefined;
    const p1 = tmpPath("cap_fs_demo", ".txt", &p1_buf);
    writeAtomic(p1, "Atomic CP content v1");

    // 2. 元信息检查与同步读取
    const ok = exists(p1);          // true
    const sz = statSize(p1);        // 20
    const body = readFileSync(p1);  // "Atomic CP content v1"
    * = ok;
    * = sz;
    * = body;

    // 3. 复制、重命名与清理
    let p2_buf: [256]u8 = undefined;
    const p2 = tmpPath("cap_fs_copy", ".txt", &p2_buf);
    copyFile(p1, p2);
    remove(p1);
    remove(p2);

    // 4. 目录创建与判定
    let dir_buf: [256]u8 = undefined;
    const d = tmpPath("cap_fs_dir", "", &dir_buf);
    mkdir(d);
    const is_dir = isDirectory(d); // true
    * = is_dir;
    rmdir(d);
}
```

---

## 四、 与 Bun / Node.js API 对照表

| 功能特性 | Bun (`Bun.file` / `node:fs`) | Node.js (`node:fs` / `node:fs/promises`) | CP 标准库 (`cap:fs`) |
| :--- | :--- | :--- | :--- |
| **异步读写** | `await Bun.file(p).text()` / `Bun.write()` | `fs.promises.readFile` / `writeFile` | `await readFile(p)` / `await writeFile(p, s)` |
| **同步读写** | `fs.readFileSync` / `writeFileSync` | `fs.readFileSync` / `writeFileSync` | `readFileSync(p)` / `writeFileSync(p, s)` |
| **原子替换写入** | 手动临时文件 + `rename` | 第三方库（`write-file-atomic`） | `writeAtomic(dest, content)` 内置支持 |
| **临时路径生成** | `os.tmpdir()` + 拼接 | `os.tmpdir()` + `crypto.randomUUID()` | `tmpDir()` / `tmpPath(prefix, suffix, buf)` |
| **目录与元信息** | `fs.existsSync` / `statSync().isDirectory()` | `fs.existsSync` / `statSync()` | `exists` / `statSize` / `isDirectory` / `mkdir` / `rmdir` |
