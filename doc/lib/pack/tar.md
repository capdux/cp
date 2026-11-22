# CP 标准库：POSIX ustar 归档打包与安全解包 (`cap:tar`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:pack`，导入请使用 `import { ... } from "cap:pack"`。

> 参考来源：`/data/dev/bun/docs/runtime/archive.mdx`, `/data/dev/bun/src/libarchive/` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:tar` 是 CP 语言官方内置的零 OS 依赖、纯 CP 内存流式 **POSIX.1-1988 ustar** 归档编解码标准库，面向容器镜像层、包管理器分发归档及内存文件系统快照设计。

**零 `std.posix`、零 `@import("bun")`、无磁盘 syscall 依赖**，既支持 `import { TarBuilder, parseArchive, ... } from "cap:pack"`，也支持 `import cap from "cap"; cap.tar.*` 动态挂载调用。

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
import { TarBuilder, parseArchive, freeEntries } from "cap:pack";
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

---

## 上游 Bun Archive 行为契约与安全边界注意事项

1. **严格路径穿越防护（`UnsafePathTraversal`）**：
   - 参考 `/data/dev/bun/docs/runtime/archive.mdx` 安全解包原则：`TarReader` 与 `parseArchive` 在解析阶段即自动拒绝以 `/` 开头的绝对路径以及包含 `..` 段的路径，无需调用方二次手写路径清洗即可免疫 Tar-Slip 越界写入。
2. **USTAR 长度边界与扩展选择**：
   - POSIX.1-1988 ustar 通过 `prefix`（155B）+ `/` + `name`（100B）支持最长 256 字节路径，符号链接目标上限为 100 字节；超出限制时 `TarBuilder` 明确返回 `TarError.PathTooLong`，若业务需要支持超长路径或大文件（> 8 GiB）请使用 `cap:pack` 中的 `PaxTarWriter` / `PaxTarReader`。
3. **512 字节块对齐与严格八进制校验和**：
   - 所有条目头与载荷均按 `TAR_BLOCK_SIZE = 512` 字节对齐填充零字节；解析器严格校验每条头的 512 字节无符号校验和，损坏或截断的归档流立即返回 `InvalidChecksum` 或 `TruncatedArchive`。
4. **确定性内存释放**：
   - `parseArchive` 返回的条目列表必须配对 `defer freeEntries(alloc, entries);`，`TarBuilder` 必须配对 `defer builder.deinit();`。
