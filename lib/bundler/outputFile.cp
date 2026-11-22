// Source: Absorbed from /data/dev/bun/src/bundler/OutputFile.cp | Synced: 2026-11-22T12:25:00Z
// Pure-CP zero-allocation Bundler `BuildOutputFile` (`OutputFile` / `BuildArtifact`) model:
// supports Buffer / Move / Copy / Saved / Noop storage modes, `OutputBakeExtra` metadata,
// extension-based loader & MIME inference, 64-bit FNV-1a content hashing, path-traversal
// safe `resolveFullPath(outdir)`, and fixed-capacity `BuildOutputCollection` aggregation.

/// 单个 `BuildOutputCollection` 支持的最大构建产物文件数
pub const OUTPUT_FILE_MAX_COLLECTION: usize = 64;

/// 构建产物模型与路径解析错误集
pub err OutputFileError {
    EmptyPath,
    UnsafePathTraversal,
    CollectionFull,
    BufferTooSmall,
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn endsWithIgnoreCase(s: []const u8, suffix: []const u8): bool {
    if (suffix.len > s.len) {
        return false;
    }
    const off = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (toLowerAscii(s[off + i]) != toLowerAscii(suffix[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < data.len) {
        h ^= @as(u64, data[i]);
        h = h *% 1099511628211;
        i += 1;
    }
    return h;
}

fn isSafeRelOutputPath(path: []const u8): bool {
    if (path.len == 0) {
        return false;
    }
    // 禁止 NUL 字节与路径穿越段 `..`
    let i: usize = 0;
    let seg_start: usize = 0;
    while (i <= path.len) {
        if (i < path.len && path[i] == 0) {
            return false;
        }
        if (i == path.len || path[i] == '/' || path[i] == '\\') {
            const seg = path[seg_start..i];
            if (bytesEql(seg, "..")) {
                return false;
            }
            seg_start = i + 1;
        }
        i += 1;
    }
    return true;
}

fn writeByte(out: []u8, pos: *usize, b: u8): usize! {
    if (pos.* >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
    return 1;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): usize! {
    if (s.len > out.len - pos.*) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return s.len;
}

fn writeUsizeDecimal(out: []u8, pos: *usize, val: usize): usize! {
    if (val == 0) {
        return writeByte(out, pos, '0');
    }
    let tmp: [24]u8 = [0; 24];
    let len: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[len] = @as(u8, @intCast(v % 10)) + '0';
        v = v / 10;
        len += 1;
    }
    if (len > out.len - pos.*) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < len) {
        out[pos.* + i] = tmp[len - 1 - i];
        i += 1;
    }
    pos.* += len;
    return len;
}

/// 构建产物语义分类（对齐 `Bun.build` `BuildArtifact.kind`）
pub enum OutputFileKind: u8 {
    EntryPoint = 0,
    Chunk = 1,
    Asset = 2,
    Sourcemap = 3,
    Bytecode = 4,

    fn asStr(): []const u8 {
        return switch (_) {
            .EntryPoint => "entry-point",
            .Chunk => "chunk",
            .Asset => "asset",
            .Sourcemap => "sourcemap",
            .Bytecode => "bytecode",
        };
    }
}

/// 产物内容载体类型（对齐 Bun `OutputFile.Kind`）
pub enum OutputValueKind: u8 {
    Move = 0,
    Copy = 1,
    Noop = 2,
    Buffer = 3,
    Pending = 4,
    Saved = 5,

    fn asStr(): []const u8 {
        return switch (_) {
            .Move => "move",
            .Copy => "copy",
            .Noop => "noop",
            .Buffer => "buffer",
            .Pending => "pending",
            .Saved => "saved",
        };
    }

    fn isInMemory(): bool {
        return _ == .Buffer;
    }

    fn isPersisted(): bool {
        return _ == .Saved;
    }
}

/// 产物 Loader 与 MIME 类型分类
pub enum OutputLoaderKind: u8 {
    Js = 0,
    Jsx = 1,
    Ts = 2,
    Tsx = 3,
    Css = 4,
    Html = 5,
    Json = 6,
    Wasm = 7,
    File = 8,
    Text = 9,
    Bytecode = 10,

    fn asStr(): []const u8 {
        return switch (_) {
            .Js => "js",
            .Jsx => "jsx",
            .Ts => "ts",
            .Tsx => "tsx",
            .Css => "css",
            .Html => "html",
            .Json => "json",
            .Wasm => "wasm",
            .File => "file",
            .Text => "text",
            .Bytecode => "bytecode",
        };
    }

    fn mimeType(): []const u8 {
        return switch (_) {
            .Js, .Jsx, .Ts, .Tsx => "text/javascript;charset=utf-8",
            .Css => "text/css;charset=utf-8",
            .Html => "text/html;charset=utf-8",
            .Json => "application/json;charset=utf-8",
            .Wasm => "application/wasm",
            .Text => "text/plain;charset=utf-8",
            .Bytecode, .File => "application/octet-stream",
        };
    }

    static fn inferFromPath(path: []const u8): OutputLoaderKind {
        if (endsWithIgnoreCase(path, ".js") || endsWithIgnoreCase(path, ".mjs") || endsWithIgnoreCase(path, ".cjs")) {
            return .Js;
        }
        if (endsWithIgnoreCase(path, ".jsx")) return .Jsx;
        if (endsWithIgnoreCase(path, ".ts") || endsWithIgnoreCase(path, ".mts") || endsWithIgnoreCase(path, ".cts")) {
            return .Ts;
        }
        if (endsWithIgnoreCase(path, ".tsx")) return .Tsx;
        if (endsWithIgnoreCase(path, ".css")) return .Css;
        if (endsWithIgnoreCase(path, ".html") || endsWithIgnoreCase(path, ".htm")) return .Html;
        if (endsWithIgnoreCase(path, ".json") || endsWithIgnoreCase(path, ".map")) return .Json;
        if (endsWithIgnoreCase(path, ".wasm")) return .Wasm;
        if (endsWithIgnoreCase(path, ".jsc")) return .Bytecode;
        if (endsWithIgnoreCase(path, ".txt")) return .Text;
        return .File;
    }
}

/// 全栈框架（Bake）路由与静态化附加标志（对齐 Bun `OutputFile.BakeExtra`）
pub struct OutputBakeExtra {
    is_route: bool = false,
    fully_static: bool = false,
    bake_is_runtime: bool = false,
}

/// 零堆分配 Bundler 单个构建产物文件描述模型（对齐 Bun `OutputFile`）
pub struct BuildOutputFile {
    dest_path: []const u8 = "",
    kind: OutputFileKind = .Chunk,
    value_kind: OutputValueKind = .Buffer,
    loader: OutputLoaderKind = .Js,
    bytes: []const u8 = "",
    src_path: []const u8 = "",
    content_hash: u64 = 0,
    size_bytes: usize = 0,
    sourcemap_index: u32? = null,
    bake: OutputBakeExtra = OutputBakeExtra{},

    /// 从内存缓冲区构造构建产物（自动校验路径安全、推断 Loader 并计算 64 位 FNV-1a 哈希）
    static fn fromBuffer(dest_path: []const u8, kind: OutputFileKind, bytes: []const u8): BuildOutputFile! {
        if (dest_path.len == 0) {
            return err.EmptyPath;
        }
        if (!isSafeRelOutputPath(dest_path)) {
            return err.UnsafePathTraversal;
        }
        return BuildOutputFile{
            dest_path: dest_path,
            kind: kind,
            value_kind: .Buffer,
            loader: OutputLoaderKind.inferFromPath(dest_path),
            bytes: bytes,
            src_path: "",
            content_hash: fnv1a64(bytes),
            size_bytes: bytes.len,
            sourcemap_index: null,
            bake: OutputBakeExtra{},
        };
    }

    static fn fromMove(src_path: []const u8, dest_path: []const u8, kind: OutputFileKind, size_bytes: usize): BuildOutputFile! {
        if (src_path.len == 0 || dest_path.len == 0) {
            return err.EmptyPath;
        }
        if (!isSafeRelOutputPath(dest_path)) {
            return err.UnsafePathTraversal;
        }
        return BuildOutputFile{
            dest_path: dest_path,
            kind: kind,
            value_kind: .Move,
            loader: OutputLoaderKind.inferFromPath(dest_path),
            bytes: "",
            src_path: src_path,
            content_hash: 0,
            size_bytes: size_bytes,
            sourcemap_index: null,
            bake: OutputBakeExtra{},
        };
    }

    static fn fromCopy(src_path: []const u8, dest_path: []const u8, kind: OutputFileKind, size_bytes: usize): BuildOutputFile! {
        if (src_path.len == 0 || dest_path.len == 0) {
            return err.EmptyPath;
        }
        if (!isSafeRelOutputPath(dest_path)) {
            return err.UnsafePathTraversal;
        }
        return BuildOutputFile{
            dest_path: dest_path,
            kind: kind,
            value_kind: .Copy,
            loader: OutputLoaderKind.inferFromPath(dest_path),
            bytes: "",
            src_path: src_path,
            content_hash: 0,
            size_bytes: size_bytes,
            sourcemap_index: null,
            bake: OutputBakeExtra{},
        };
    }

    static fn noop(dest_path: []const u8): BuildOutputFile! {
        if (dest_path.len == 0) {
            return err.EmptyPath;
        }
        if (!isSafeRelOutputPath(dest_path)) {
            return err.UnsafePathTraversal;
        }
        return BuildOutputFile{
            dest_path: dest_path,
            kind: .Asset,
            value_kind: .Noop,
            loader: OutputLoaderKind.inferFromPath(dest_path),
            bytes: "",
            src_path: "",
            content_hash: 0,
            size_bytes: 0,
            sourcemap_index: null,
            bake: OutputBakeExtra{},
        };
    }

    /// 对齐 Bun `OutputFile.Value.asSlice()`：仅 `Buffer` 返回内存字节切片，其余返回空串
    fn asSlice(): []const u8 {
        if (_.value_kind == .Buffer) {
            return _.bytes;
        }
        return "";
    }

    fn mimeType(): []const u8 {
        return _.loader.mimeType();
    }

    fn markSaved(*_) {
        _.value_kind = .Saved;
    }

    /// 将 `outdir` 与 `dest_path` 安全拼接为最终落盘路径（拒绝 `dest_path` 穿越或绝对路径覆盖）
    fn resolveFullPath(outdir: []const u8, out: []u8): usize! {
        if (!isSafeRelOutputPath(_.dest_path)) {
            return err.UnsafePathTraversal;
        }
        if (_.dest_path[0] == '/' || _.dest_path[0] == '\\') {
            return err.UnsafePathTraversal;
        }
        let pos: usize = 0;
        let trimmed_dir = outdir;
        while (trimmed_dir.len > 1 && trimmed_dir[trimmed_dir.len - 1] == '/') {
            trimmed_dir = trimmed_dir[0 .. trimmed_dir.len - 1];
        }
        if (trimmed_dir.len > 0) {
            * = writeSlice(out, *pos, trimmed_dir)?;
            if (trimmed_dir[trimmed_dir.len - 1] != '/') {
                * = writeByte(out, *pos, '/')?;
            }
        }
        * = writeSlice(out, *pos, _.dest_path)?;
        return pos;
    }

    /// 格式化构建汇总行：`<dest_path> [<kind>, <size_bytes> B]`
    fn formatSummaryLine(out: []u8): usize! {
        let pos: usize = 0;
        * = writeSlice(out, *pos, _.dest_path)?;
        * = writeSlice(out, *pos, " [")?;
        * = writeSlice(out, *pos, _.kind.asStr())?;
        * = writeSlice(out, *pos, ", ")?;
        * = writeUsizeDecimal(out, *pos, _.size_bytes)?;
        * = writeSlice(out, *pos, " B]")?;
        return pos;
    }
}

/// 零堆分配构建产物集合（支持按路径检索、按类型计数与总字节统计）
pub struct BuildOutputCollection {
    items: [OUTPUT_FILE_MAX_COLLECTION]BuildOutputFile = [BuildOutputFile{}; OUTPUT_FILE_MAX_COLLECTION],
    len: usize = 0,

    static fn init(): BuildOutputCollection {
        return BuildOutputCollection{};
    }

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): BuildOutputFile? {
        if (idx >= _.len) {
            return null;
        }
        return _.items[idx];
    }

    fn push(*_, item: BuildOutputFile): usize! {
        if (_.len >= OUTPUT_FILE_MAX_COLLECTION) {
            return err.CollectionFull;
        }
        _.items[_.len] = item;
        _.len += 1;
        return _.len;
    }

    fn findByPath(dest_path: []const u8): BuildOutputFile? {
        let i: usize = 0;
        while (i < _.len) {
            if (bytesEql(_.items[i].dest_path, dest_path)) {
                return _.items[i];
            }
            i += 1;
        }
        return null;
    }

    fn totalBytes(): usize {
        let sum: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            sum += _.items[i].size_bytes;
            i += 1;
        }
        return sum;
    }

    fn countByKind(kind: OutputFileKind): usize {
        let c: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (_.items[i].kind == kind) {
                c += 1;
            }
            i += 1;
        }
        return c;
    }
}
