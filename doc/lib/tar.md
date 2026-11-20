# CP 标准库：POSIX ustar 归档打包与安全解包 (`cap:tar`)

## 一、 架构总览与设计准则

`cap:tar` 是 CP 语言官方内置的零 OS 依赖、纯 CP 内存流式 **POSIX.1-1988 ustar** 归档编解码标准库，面向容器镜像层、包管理器分发归档及内存文件系统快照设计。

**零 `std.posix`、零 `@import("bun")`、无磁盘 syscall 依赖**，既支持 `import { TarBuilder, parseArchive, ... } from "cap:tar"`，也支持 `import cap from "cap"; cap.tar.*` 动态挂载调用。

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/tar/tar.cp` | `// Source: cp/lib/tar/tar.cp (...)` | `TarError`、`TarTypeFlag`、`TarEntry`、`TarBuilder`、`TarReader`、`parseArchive`、`freeEntries` |
| `lib/tar/index.cp` | `// Source: cp/lib/tar/index.cp (...)` | `cap:tar` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合 (`TarError`)

- `InvalidHeader`：魔数非 `"ustar"` 或头部字段非法（如空文件名）
- `InvalidChecksum`：512 字节头部八进制校验和不匹配或校验和字段非合法八进制
- `TruncatedArchive`：归档字节流在头部或载荷 512 字节块边界前意外截断
- `PathTooLong`：路径超出 ustar 最大限制（`prefix` 155 + `/` + `name` 100 = 256 字节）或符号链接目标超过 100 字节
- `UnsafePathTraversal`：路径安全校验拦截到绝对前导 `/` 或包含 `..` 路径段
- `InvalidOctal`：头部数值字段包含非八进制字符
- `OutOfMemory`：内存分配失败

### 2. 类型标志与条目 (`TarTypeFlag` / `TarEntry`)

- `TarTypeFlag`：`Regular`（`'0'` / `'\0'`）、`Symlink`（`'2'`）、`Directory`（`'5'`）、`Other`
- `TarEntry`：包含 `name`, `mode`, `uid`, `gid`, `size`, `mtime`, `type_flag`, `linkname`, `uname`, `gname`, `data: []const u8`

### 3. 归档构建器 (`TarBuilder`)

- `TarBuilder.init(allocator: std.mem.Allocator): TarBuilder`
- `deinit(*_)`: 释放内部缓冲
- `addFile(*_, name: []const u8, data: []const u8, mode: u32, mtime: u64): (void)!TarError`
- `addDir(*_, name: []const u8, mode: u32, mtime: u64): (void)!TarError`
- `addSymlink(*_, name: []const u8, target: []const u8, mode: u32, mtime: u64): (void)!TarError`
- `finish(*_, allocator: std.mem.Allocator): ([]u8)!TarError`：自动按需拆分 ustar `prefix`（最长 155 + `/` + 100 字节）、计算 512 字节头八进制校验和、对数据块执行 512 字节零填充，并在归档末尾附加双 512 字节全零 EOF 块。

### 4. 安全归档解析器 (`TarReader` / `parseArchive` / `freeEntries`)

- `parseArchive(allocator: std.mem.Allocator, bytes: []const u8): ([]TarEntry)!TarError`：默认启用路径安全校验（拦截 `..` 路径段与前导 `/`）
- `TarReader.parse(allocator, bytes)` / `TarReader.parseWithOptions(allocator, bytes, check_path_safety)`
- `freeEntries(allocator: std.mem.Allocator, entries: []TarEntry)` / `TarReader.free(allocator, entries)`：完整释放条目切片及其内部分配的字符串与数据缓冲区。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { TarBuilder, parseArchive, freeEntries } from "cap:tar";
import cap from "cap";

pub fn demoTar()! {
    const alloc = std.heap.page_allocator;
    let builder = TarBuilder.init(alloc);
    defer builder.deinit();

    builder.addDir("app/", 0o755, 1700000000)?;
    builder.addFile("app/hello.txt", "Hello CP Tar!", 0o644, 1700000001)?;
    builder.addSymlink("app/latest", "hello.txt", 0o777, 1700000002)?;

    const archive = builder.finish(alloc)?;
    defer alloc.free(archive);

    const entries = parseArchive(alloc, archive)?;
    defer freeEntries(alloc, entries);

    // 动态挂载调用
    const entries2 = cap.tar.parseArchive(alloc, archive)?;
    cap.tar.freeEntries(alloc, entries2);
}
```
