// Source: Absorbed from /data/dev/bun/src/bundler/entry_points.cp | Synced: 2026-11-22T02:24:00Z
// Pure-CP zero-allocation bundler virtual entry-point path codec and synthetic
// bootstrap code generator (FallbackEntryPoint, ClientEntryPoint,
// ServerEntryPoint with optional HMR, and deterministic MacroEntryPoint ID).

/// 入口点路径编解码与代码生成错误集
pub err EntryPointError {
    EmptyInput,
    InvalidPath,
    BufferTooSmall,
}

/// 虚拟入口点命名空间
pub enum EntryPointNamespace: u8 {
    fallback_entry = 0,
    client_entry = 1,
    server_entry = 2,
    macro_entry = 3,

    fn asStr(): string {
        return switch (_) {
            .fallback_entry => "fallback-entry",
            .client_entry => "client-entry",
            .server_entry => "server-entry",
            .macro_entry => "macro",
        };
    }
}

/// 宏虚拟入口点确定性标识结果（对齐 Bun `MacroEntryPoint.generateID`）
pub struct MacroEntryId {
    id: i32,
    hash: u64,
    specifier_len: usize,
}

fn appendBytes(buf: []u8, pos: usize, s: string): usize! {
    if (pos + s.len > buf.len) {
        return err.BufferTooSmall;
    }
    @memcpy(buf[pos .. pos + s.len], s);
    return pos + s.len;
}

fn appendByte(buf: []u8, pos: usize, b: u8): usize! {
    if (pos >= buf.len) {
        return err.BufferTooSmall;
    }
    buf[pos] = b;
    return pos + 1;
}

/// 将字符串按 JS 单引号或双引号字符串字面量规则安全转义并写入 `buf`（防代码注入）
fn appendJsEscaped(buf: []u8, pos: usize, s: string, quote_char: u8): usize! {
    let p = pos;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '\\') {
            p = appendBytes(buf, p, "\\\\")?;
        } else if (c == quote_char) {
            p = appendByte(buf, p, '\\')?;
            p = appendByte(buf, p, quote_char)?;
        } else if (c == '\n') {
            p = appendBytes(buf, p, "\\n")?;
        } else if (c == '\r') {
            p = appendBytes(buf, p, "\\r")?;
        } else if (c == '\t') {
            p = appendBytes(buf, p, "\\t")?;
        } else {
            p = appendByte(buf, p, c)?;
        }
        i += 1;
    }
    return p;
}

fn fnv1a64Update(hash_in: u64, data: string): u64 {
    let h = hash_in;
    let i: usize = 0;
    while (i < data.len) {
        h ^= data[i] as u64;
        h = h *% 0x100000001b3;
        i += 1;
    }
    return h;
}

fn appendHexU64Lower(buf: []u8, pos: usize, value: u64): usize! {
    if (pos + 16 > buf.len) {
        return err.BufferTooSmall;
    }
    const hex_chars = "0123456789abcdef";
    let i: usize = 0;
    while (i < 16) {
        const shift: u6 = ((15 - i) * 4) as u6;
        const nibble = ((value >> shift) & 0xf) as usize;
        buf[pos + i] = hex_chars[nibble];
        i += 1;
    }
    return pos + 16;
}

fn findSubstring(haystack: string, needle: string): usize? {
    if (needle.len == 0 || needle.len > haystack.len) {
        return null;
    }
    let i: usize = 0;
    const limit = haystack.len - needle.len;
    while (i <= limit) {
        if (std.mem.eql(u8, haystack[i .. i + needle.len], needle)) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// 检测扩展名或完整路径是否为虚拟客户端入口点（匹配 `entry.` 或 `.entry.` 标记）
pub fn isEntryPointPath(path_or_ext: string): bool {
    if (path_or_ext.len >= 6 && std.mem.eql(u8, path_or_ext[0..6], "entry.")) {
        return true;
    }
    return findSubstring(path_or_ext, ".entry.") != null;
}

/// 将原始路径 `(dir, base, ext)` 编码为虚拟入口路径 `<dir>/<base>.entry<ext>`
pub fn encodeEntryPointPath(dir: string, base: string, ext: string, buf: []u8): string! {
    if (base.len == 0) {
        return err.InvalidPath;
    }
    let pos: usize = 0;
    if (dir.len > 0) {
        pos = appendBytes(buf, pos, dir)?;
        if (dir[dir.len - 1] != '/') {
            pos = appendByte(buf, pos, '/')?;
        }
    }
    pos = appendBytes(buf, pos, base)?;
    pos = appendBytes(buf, pos, ".entry")?;
    if (ext.len > 0) {
        if (ext[0] != '.') {
            pos = appendByte(buf, pos, '.')?;
        }
        pos = appendBytes(buf, pos, ext)?;
    }
    return buf[0..pos];
}

/// 将虚拟入口路径 `<dir>/<base>.entry<ext>` 解码还原为 `<dir>/<base><ext>`
pub fn decodeEntryPointPath(entry_path: string, buf: []u8): string! {
    if (entry_path.len == 0) {
        return err.EmptyInput;
    }
    if (marker := findSubstring(entry_path, ".entry.")) {
        let pos: usize = 0;
        pos = appendBytes(buf, pos, entry_path[0..marker])?;
        pos = appendBytes(buf, pos, entry_path[marker + 6 ..])?;
        return buf[0..pos];
    }
    const pos = appendBytes(buf, 0, entry_path)?;
    return buf[0..pos];
}

/// 生成 Fallback 入口引导代码（对齐 Bun `FallbackEntryPoint.generate`）
pub fn generateFallbackEntryCode(
    boot_module: string,
    disable_css_imports: bool,
    buf: []u8,
): string! {
    if (boot_module.len == 0) {
        return err.EmptyInput;
    }
    let pos: usize = 0;
    if (disable_css_imports) {
        pos = appendBytes(buf, pos, "globalThis.Bun_disableCSSImports = true;\n")?;
    }
    pos = appendBytes(buf, pos, "import boot from '")?;
    pos = appendJsEscaped(buf, pos, boot_module, '\'')?;
    pos = appendBytes(buf, pos, "';\nboot(globalThis.__BUN_DATA__);\n")?;
    return buf[0..pos];
}

/// 生成 Client 框架入口引导代码（对齐 Bun `ClientEntryPoint.generate`）
pub fn generateClientEntryCode(
    client_boot: string,
    entry_import_path: string,
    disable_css_imports: bool,
    buf: []u8,
): string! {
    if (client_boot.len == 0 || entry_import_path.len == 0) {
        return err.EmptyInput;
    }
    let pos: usize = 0;
    if (disable_css_imports) {
        pos = appendBytes(buf, pos, "globalThis.Bun_disableCSSImports = true;\n")?;
        pos = appendBytes(buf, pos, "import boot from '")?;
        pos = appendJsEscaped(buf, pos, client_boot, '\'')?;
        pos = appendBytes(buf, pos, "';\nimport * as EntryPoint from '")?;
        pos = appendJsEscaped(buf, pos, entry_import_path, '\'')?;
        pos = appendBytes(buf, pos, "';\nboot(EntryPoint);\n")?;
    } else {
        pos = appendBytes(buf, pos, "import boot from '")?;
        pos = appendJsEscaped(buf, pos, client_boot, '\'')?;
        pos = appendBytes(buf, pos, "';\nif ('setLoaded' in boot) boot.setLoaded(loaded);\nimport * as EntryPoint from '")?;
        pos = appendJsEscaped(buf, pos, entry_import_path, '\'')?;
        pos = appendBytes(buf, pos, "';\nboot(EntryPoint);\n")?;
    }
    return buf[0..pos];
}

/// 生成 Server 入口引导代码（对齐 Bun `ServerEntryPoint.generate`，支持普通与 HMR 热重载模式）
pub fn generateServerEntryCode(
    entry_path: string,
    hot_reload: bool,
    buf: []u8,
): string! {
    if (entry_path.len == 0) {
        return err.EmptyInput;
    }
    let pos: usize = 0;
    pos = appendBytes(buf, pos, "// @bun\n")?;
    if (hot_reload) {
        pos = appendBytes(buf, pos, "import * as start from '")?;
        pos = appendJsEscaped(buf, pos, entry_path, '\'')?;
        pos = appendBytes(buf, pos, "';\nvar hmrSymbol = Symbol(\"BunServerHMR\");\nvar entryNamespace = start;\nfunction isServerConfig(def) {\n  return def && def !== globalThis && (typeof def.fetch === 'function' || def.app != undefined) && typeof def.stop !== 'function';\n}\nif (isServerConfig(entryNamespace?.default)) {\n  var server = globalThis[hmrSymbol];\n  if (server) { server.reload(entryNamespace.default); }\n  else { globalThis[hmrSymbol] = Bun.serve(entryNamespace.default); }\n}\n")?;
    } else {
        pos = appendBytes(buf, pos, "import * as start from \"")?;
        pos = appendJsEscaped(buf, pos, entry_path, '"')?;
        pos = appendBytes(buf, pos, "\";\nvar entryNamespace = start;\nfunction isServerConfig(def) {\n  return def && def !== globalThis && (typeof def.fetch === 'function' || def.app != undefined) && typeof def.stop !== 'function';\n}\nif (isServerConfig(entryNamespace?.default)) {\n  Bun.serve(entryNamespace.default);\n}\n")?;
    }
    return buf[0..pos];
}

/// 生成确定性宏虚拟入口点说明符 `macro://<16-hex>.js` 与 `i32` ID（对齐 Bun `MacroEntryPoint.generateID`）
pub fn generateMacroEntryId(
    entry_path: string,
    function_name: string,
    specifier_buf: []u8,
): MacroEntryId! {
    if (entry_path.len == 0 || function_name.len == 0) {
        return err.EmptyInput;
    }
    let h: u64 = 0xcbf29ce484222325;
    h = fnv1a64Update(h, "macro:");
    h = fnv1a64Update(h, entry_path);
    h = fnv1a64Update(h, "\x00");
    h = fnv1a64Update(h, function_name);

    let pos: usize = 0;
    pos = appendBytes(specifier_buf, pos, "macro://")?;
    pos = appendHexU64Lower(specifier_buf, pos, h)?;
    pos = appendBytes(specifier_buf, pos, ".js")?;

    const spec_hash = fnv1a64Update(0xcbf29ce484222325, specifier_buf[0..pos]);
    const id_u32 = (spec_hash & 0xffffffff) as u32;
    return MacroEntryId{
        id: id_u32 as i32,
        hash: h,
        specifier_len: pos,
    };
}
