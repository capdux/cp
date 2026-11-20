# CP 标准库：二进制大对象与文件抽象手册 (`cap:blob`)

## 一、 架构总览与设计准则

`cap:blob` 是 CP 语言官方内置的 Web 标准二进制大对象（`Blob`）与具名文件（`File`）抽象库，吸收自 Bun 的 WebCore Blob/Store 子系统（`/data/dev/bun/src/runtime/webcore/Blob.cp` 与 `blob/Store.cp`），以 **共享引用计数底层存储（`BlobStore`）+ 零拷贝逻辑切片视图** 为核心，无缝衔接内存缓冲区与磁盘文件 I/O。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                       CP 用户层应用 (Application)                       │
├───────────────────────────────────┬────────────────────────────────────┤
│         Blob 二进制视图            │        File 具名文件对象            │
│  fromBytes / fromString / fromFile│  File.init / File.fromPath         │
│  bytes() / text() / arrayBuffer() │  name / lastModified / size / type │
│  slice(start, end, content_type)  │  slice() / save() / writeTo()      │
├───────────────────────────────────┴────────────────────────────────────┤
│         BlobStore 共享底层存储 (BlobKind.Bytes | BlobKind.File)         │
│         offset + size 零拷贝视图借用 · ref() / deref() 引用计数          │
├────────────────────────────────────────────────────────────────────────┤
│                    底层依赖: cap:fs (readFileSync / writeFileSync)      │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/blob/index.cp` | `// Source: /data/dev/bun/src/runtime/webcore/Blob.cp` | `cap:blob` 统一公开导出入口 |
| `lib/blob/store.cp` | `// Source: /data/dev/bun/src/runtime/webcore/blob/Store.cp` | `BlobKind` 枚举与带引用计数的 `BlobStore` 共享存储 |
| `lib/blob/blob.cp` | `// Source: /data/dev/bun/src/runtime/webcore/Blob.cp` | `Blob` 零拷贝切片、文本/字节视图、独立 `arrayBuffer` 拷贝与 MIME 自动推导 |
| `lib/blob/file.cp` | `// Source: /data/dev/bun/src/runtime/webcore/Blob.cp` | `File` 具名文件元数据封装、切片继承与磁盘持久化写回 |

### 2. 核心设计特性

1. **零拷贝子切片与引用计数共享（Zero-Copy Slicing）**：
   - 调用 `blob.slice(start, end, content_type)` 不会复制底层字节，仅递增 `BlobStore.ref_count` 并计算新的 `offset` 与 `size`，支持任意深度嵌套切片。
2. **显式所有权与 RAII 释放**：
   - 每个 `Blob` / `File` 均提供 `release()` 与 `deinit()` 以递减底层 `BlobStore` 引用计数；当需要脱离共享存储获取独立可变副本时，调用 `arrayBuffer()` 分配独立堆切片 `[]u8`。
3. **自动 MIME 媒体类型推导**：
   - `Blob.fromFile` 与 `File.fromPath` 在未显式传入 `mime_type` 时，自动根据文件扩展名识别常见图像（PNG/JPEG/WebP/BMP/GIF）、JSON、HTML、CSS、JS 与纯文本类型，默认回退为 `application/octet-stream`。

---

## 二、 核心 API 详解

### 1. `BlobStore` 与 `BlobKind`

- `BlobKind` 枚举：`Bytes = 0`、`File = 1`。

| API 签名 | 说明 |
| :--- | :--- |
| `BlobStore.fromBytes(data)` / `createFromBytes(data)` | 基于内存字节切片创建引用计数为 `1` 的 `BlobStore` |
| `BlobStore.fromFile(path, cached_data)` / `createFromFile(...)` | 基于文件路径与缓存字节创建文件型 `BlobStore` |
| `store.ref()` / `store.deref()` | 手动递增或递减引用计数 |

### 2. `Blob` 核心对象

字段：`size: usize`、`type: string`、`store: *BlobStore`、`offset: usize`。

| API 签名 | 说明 |
| :--- | :--- |
| `Blob.fromBytes(data, mime_type)` / `Blob.init(data, mime_type)` | 从字节切片构建 `Blob`（空 MIME 默认 `"application/octet-stream"`） |
| `Blob.fromString(content, mime_type)` | 从 UTF-8 字符串构建 `Blob`（空 MIME 默认 `"text/plain;charset=utf-8"`） |
| `Blob.fromFile(path, mime_type): Blob?` | 从磁盘读取文件构建 `Blob`，支持按后缀自动推导 MIME |
| `blob.bytes(): []const u8` | 零拷贝返回当前 `[offset .. offset + size]` 只读字节视图 |
| `blob.text(): string` | 零拷贝返回 UTF-8 文本视图 |
| `blob.slice(start, end, content_type): Blob` | 零拷贝切分子视图并递增共享 `BlobStore` 引用计数（向后兼容 `usize`） |
| `blob.sliceSigned(start: isize, end: isize, content_type): Blob` | 支持负索引（如 `-4, -1`）与反转范围（返回 0 字节空视图）的 W3C 标准切片 |
| `Blob.concat(parts: []const Blob, mime_type): Blob` | 将多段 `Blob` 高效合并为单一连续 `Blob` |
| `blob.equals(other: Blob): bool` | 按存储指针快速路径或逐字节比较两个 `Blob` 内容是否一致 |
| `blob.stream(chunk_size: usize): BlobChunkReader` | 返回分块流式迭代器，支持 `next()`、`remaining()` 与 `reset()` |
| `blob.arrayBuffer(): []u8` | 分配并返回当前视图数据的独立可变堆拷贝 |
| `blob.release()` / `blob.deinit()` | 递减底层 `BlobStore` 引用计数 |

### 3. `File` 具名文件对象

字段：`blob: Blob`、`name: string`、`path: string`、`lastModified: i64`、`size: usize`、`type: string`（磁盘操作基于 Zig 0.17 `std.c.open` / `read` / `write` / `statx` / `access` / `unlink`）。

| API 签名 | 说明 |
| :--- | :--- |
| `File.init(data, name, mime_type, last_modified): File` | 从内存字节构建带文件名与时间戳的 `File` 对象 |
| `File.fromPath(path, mime_type): File?` | 从磁盘读取文件与 `statx` 元数据（路径不存在时安全返回 `null`） |
| `File.at(path, mime_type): File` | 构造绑定磁盘路径的 `File` 句柄（自动探测存在性与 `lastModified`） |
| `file.exists(): bool` | 通过 `std.c.access` 检测磁盘文件是否存在 |
| `file.bytes()` / `file.text()` | 获取只读字节视图或文本视图 |
| `file.slice(start, end, content_type)` / `sliceSigned(...)` | 零拷贝切分 `File`（保留 `name`、`path` 与 `lastModified`） |
| `file.write(data): bool` | 通过 `std.c.open`/`write` 将数据写入 `file.path` 并同步内存元数据 |
| `file.delete(): bool` | 通过 `std.c.unlink` 删除磁盘上的 `file.path` |
| `file.save(dest_path)` / `file.writeTo(dest_path): bool` | 将当前视图内容持久化写入目标磁盘路径 |
| `file.release()` / `file.deinit()` | 释放底层 `Blob` 引用 |

---

## 三、 可运行代码示例

```typescript
import { Blob, File, BlobStore, BlobKind } from "cap:blob";

pub fn demoBlobAndFile() {
    // 1. 构建 Blob 与嵌套零拷贝切片
    let root = Blob.fromString("0123456789ABCDEF", "text/plain");
    let mid = root.slice(4, 10, "application/octet-stream"); // "456789"
    // 此时 root.store.ref_count == 2，未发生字节复制

    // 2. 获取独立可变堆拷贝 (ArrayBuffer 语义)
    let buf = mid.arrayBuffer();
    buf[0] = 'X'; // 修改副本不影响原 root/mid 内容

    // 3. File 创建、持久化与从路径加载
    let doc = File.init("{\"ok\":true}", "payload.json", "application/json", 1732000000 as i64);
    let saved = doc.writeTo("temp/payload.json");
    if (saved) {
        if (loaded := File.fromPath("temp/payload.json", "")) {
            // loaded.name == "payload.json", loaded.type == "application/json"
            const txt = loaded.text();
            _ = txt;
            loaded.release();
        }
    }

    doc.release();
    mid.release();
    root.release();
}
```

---

## 四、 与 Bun `src/runtime/webcore/Blob` 对照表

| 功能分类 | Bun (`src/runtime/webcore/Blob.cp` & `Bun.file`) | CP 标准库 (`cap:blob`) |
| :--- | :--- | :--- |
| **共享存储结构** | `src/runtime/webcore/blob/Store.cp` (`Store`) | `BlobStore` (`fromBytes`, `fromFile`, `ref`, `deref`) |
| **内存构建 Blob** | `new Blob([data], { type })` | `Blob.fromBytes(data, mime)` / `Blob.fromString(text, mime)` |
| **文件句柄构建** | `Bun.file(path, { type })` | `Blob.fromFile(path, mime)` / `File.fromPath(path, mime)` |
| **零拷贝切片** | `blob.slice(start, end, type)` | `blob.slice(start, end, content_type)` |
| **读取文本 / 字节** | `await blob.text()` / `await blob.bytes()` | `blob.text()` / `blob.bytes()`（同步零拷贝） |
| **独立缓冲区拷贝** | `await blob.arrayBuffer()` | `blob.arrayBuffer()` |
| **文件持久化写出** | `await Bun.write(dest, file)` | `file.save(dest_path)` / `file.writeTo(dest_path)` |
| **底层零拷贝拷贝引擎** | `src/runtime/webcore/blob/copy_file.cp` | 已解耦至 `cap:sys` (`lib/sys/copy_file.cp`) |
