// Source: Absorbed from /data/dev/bun/src/bundler/ServerComponentParseTask.cp | Synced: 2026-11-22T14:10:00Z
// Pure-CP zero-allocation React Server Components (RSC) Synthetic Module Generator:
// - Generates server-side `"use client"` Client Reference Proxy modules importing
//   `registerClientReference` and wrapping every default/named export with React Flight
//   server-invocation error guards.
// - Formats Bun's development (`pretty_path`) vs production (`<hex_unique_key>S<08d_source_index>`)
//   module reference keys for deferred chunk-path replacement.
// - Generates browser-side bare-import Client Entry Wrapper modules (`import "<path>";`).

/// 单个 `"use client"` 模块支持的最大导出符号数
pub const RSC_PROXY_MAX_EXPORTS: usize = 64;

/// RSC 合成模块生成器错误集
pub err RscProxyError {
    TooManyExports,
    DuplicateExportName,
    EmptyExportName,
    InvalidExportIdent,
    EmptyModulePath,
    EmptyRuntimeImport,
    EmptyRegisterFnName,
    BufferTooSmall,
}

/// RSC 合成模块任务类型（对齐 Bun `ServerComponentParseTask.Data`）
pub enum RscTaskKind: u8 {
    ClientReferenceProxy = 0,
    ClientEntryWrapper = 1,
}

/// 合成 AST 的目标运行平台
pub enum RscTargetPlatform: u8 {
    Server = 0,
    Browser = 1,
}

/// RSC 框架配置（对齐 Bun `framework.server_components` 与 `transpiler.options`）
pub struct RscFrameworkConfig {
    server_runtime_import: []const u8 = "react-server-dom-webpack/server",
    server_register_client_reference: []const u8 = "registerClientReference",
    /// 开发模式直接使用 `pretty_path`；生产模式使用 `<hex_unique_key>S<08d_source_index>` 占位键
    is_dev_server: bool = false,
    hot_module_reloading: bool = false,
    bundle_unique_key: u64 = 0,
}

/// 被代理的客户端模块元信息
pub struct RscProxyModuleMeta {
    pretty_path: []const u8 = "",
    source_index: u32 = 0,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn isValidJsExportIdent(s: []const u8): bool {
    if (s.len == 0) return false;
    const c0 = s[0];
    const ok0 = (c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z') || c0 == '_' || c0 == '$';
    if (!ok0) return false;
    for (i of 1..s.len) {
        const c = s[i];
        const ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '$';
        if (!ok) return false;
    }
    return true;
}

fn copyInto(dst: []u8, offset: usize, src: []const u8): usize! {
    if (offset + src.len > dst.len) return err.BufferTooSmall;
    for (i of 0..src.len) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

fn copyByte(dst: []u8, offset: usize, b: u8): usize! {
    if (offset + 1 > dst.len) return err.BufferTooSmall;
    dst[offset] = b;
    return offset + 1;
}

fn copyJsStringLiteral(dst: []u8, offset: usize, s: []const u8): usize! {
    let pos = copyByte(dst, offset, '"')?;
    for (i of 0..s.len) {
        const ch = s[i];
        if (ch == '"' || ch == '\\') {
            pos = copyByte(dst, pos, '\\')?;
            pos = copyByte(dst, pos, ch)?;
        } else if (ch == '\n') {
            pos = copyInto(dst, pos, "\\n")?;
        } else if (ch == '\r') {
            pos = copyInto(dst, pos, "\\r")?;
        } else {
            pos = copyByte(dst, pos, ch)?;
        }
    }
    return copyByte(dst, pos, '"');
}

/// 返回给定 RSC 任务类型对应的合成目标平台（代理在 Server，入口包装在 Browser）
pub fn rscTargetForTask(kind: RscTaskKind): RscTargetPlatform {
    return switch (kind) {
        .ClientReferenceProxy => .Server,
        .ClientEntryWrapper => .Browser,
    };
}

/// 格式化生产模式 RSC 客户端引用模块占位键：`"<hex_unique_key>S<08d_source_index>"`
/// （对齐 Bun `ServerComponentParseTask.generateClientReferenceProxy`）
pub fn formatRscProdModuleKey(unique_key: u64, source_index: u32, out: []u8): usize! {
    const hex_digits = "0123456789abcdef";
    let hex_tmp: [16]u8 = [0; 16];
    let hex_len: usize = 0;
    let v = unique_key;
    if (v == 0) {
        hex_tmp[0] = '0';
        hex_len = 1;
    } else {
        while (v > 0) {
            hex_tmp[hex_len] = hex_digits[(v & 0xF) as usize];
            v = v >> 4;
            hex_len += 1;
        }
    }

    let pos: usize = 0;
    while (hex_len > 0) {
        hex_len -= 1;
        pos = copyByte(out, pos, hex_tmp[hex_len])?;
    }
    pos = copyByte(out, pos, 'S')?;

    // 至少 8 位零填充十进制 source_index（超过 8 位时完整输出）
    let dec_tmp: [16]u8 = [0; 16];
    let dec_len: usize = 0;
    let d: u64 = source_index as u64;
    if (d == 0) {
        dec_tmp[0] = '0';
        dec_len = 1;
    } else {
        while (d > 0) {
            dec_tmp[dec_len] = ((d % 10) + 48) as u8;
            d = d / 10;
            dec_len += 1;
        }
    }
    if (dec_len < 8) {
        const pad = 8 - dec_len;
        for (p of 0..pad) {
            * = p;
            pos = copyByte(out, pos, '0')?;
        }
    }
    while (dec_len > 0) {
        dec_len -= 1;
        pos = copyByte(out, pos, dec_tmp[dec_len])?;
    }
    return pos;
}

/// 格式化 React Flight / Bun 客户端函数在服务端误调用时的标准错误消息
pub fn formatRscServerCallErrorMsg(pretty_path: []const u8, export_name: []const u8, out: []u8): usize! {
    if (export_name.len == 0) return err.EmptyExportName;
    let pos: usize = 0;
    if (bytesEql(export_name, "default")) {
        if (pretty_path.len == 0) return err.EmptyModulePath;
        pos = copyInto(out, pos, "Attempted to call the default export of ")?;
        pos = copyInto(out, pos, pretty_path)?;
        pos = copyInto(
            out,
            pos,
            " from the server, but it's on the client. It's not possible to invoke a client function from the server, it can only be rendered as a Component or passed to props of a Client Component.",
        )?;
        return pos;
    }
    pos = copyInto(out, pos, "Attempted to call ")?;
    pos = copyInto(out, pos, export_name)?;
    pos = copyInto(out, pos, "() from the server but ")?;
    pos = copyInto(out, pos, export_name)?;
    pos = copyInto(
        out,
        pos,
        " is on the client. It's not possible to invoke a client function from the server, it can only be rendered as a Component or passed to props of a Client Component.",
    )?;
    return pos;
}

/// 生成浏览器侧裸导入 Client Entry Wrapper 代码：`import "<entry_path>";\n`
pub fn formatClientEntryWrapper(entry_path: []const u8, out: []u8): usize! {
    if (entry_path.len == 0) return err.EmptyModulePath;
    let pos: usize = 0;
    pos = copyInto(out, pos, "import ")?;
    pos = copyJsStringLiteral(out, pos, entry_path)?;
    pos = copyInto(out, pos, ";\n")?;
    return pos;
}

/// 零堆分配 RSC Server-Side Client Reference Proxy 代码生成器
pub struct RscProxyGenerator {
    cfg: RscFrameworkConfig = RscFrameworkConfig{},
    meta: RscProxyModuleMeta = RscProxyModuleMeta{},
    exports: [RSC_PROXY_MAX_EXPORTS][]const u8 = [""; RSC_PROXY_MAX_EXPORTS],
    export_count: usize = 0,

    static fn init(cfg: RscFrameworkConfig, meta: RscProxyModuleMeta): RscProxyGenerator {
        return RscProxyGenerator{
            cfg: cfg,
            meta: meta,
        };
    }

    /// 注册一个客户端导出符号（支持 `"default"` 或合法 JS 标识符，自动防重复）
    fn addExport(*_, name: []const u8): usize! {
        if (name.len == 0) return err.EmptyExportName;
        if (!bytesEql(name, "default") && !isValidJsExportIdent(name)) {
            return err.InvalidExportIdent;
        }
        for (i of 0.._.export_count) {
            if (bytesEql(_.exports[i], name)) {
                return err.DuplicateExportName;
            }
        }
        if (_.export_count >= RSC_PROXY_MAX_EXPORTS) {
            return err.TooManyExports;
        }
        const idx = _.export_count;
        _.exports[idx] = name;
        _.export_count += 1;
        return idx;
    }

    /// 返回已注册的导出名称切片
    fn exportSlice(): []const []const u8 {
        return _.exports[0.._.export_count];
    }

    /// 生成完整的 Server 侧 `"use client"` 引用代理 JS 源码
    fn generateProxyCode(out: []u8): usize! {
        if (_.cfg.server_runtime_import.len == 0) return err.EmptyRuntimeImport;
        if (_.cfg.server_register_client_reference.len == 0) return err.EmptyRegisterFnName;
        if (_.meta.pretty_path.len == 0) return err.EmptyModulePath;

        let pos: usize = 0;
        // 1. import { registerClientReference } from "<server_runtime_import>";
        pos = copyInto(out, pos, "import { ")?;
        pos = copyInto(out, pos, _.cfg.server_register_client_reference)?;
        pos = copyInto(out, pos, " } from ")?;
        pos = copyJsStringLiteral(out, pos, _.cfg.server_runtime_import)?;
        pos = copyInto(out, pos, ";\n")?;

        // 2. 计算模块引用路径（dev 使用 pretty_path，prod 使用 unique_key 占位键）
        let prod_key_buf: [48]u8 = [0; 48];
        let mod_ref_path: []const u8 = _.meta.pretty_path;
        if (!_.cfg.is_dev_server) {
            const pk_len = formatRscProdModuleKey(_.cfg.bundle_unique_key, _.meta.source_index, prod_key_buf[0..])?;
            mod_ref_path = prod_key_buf[0..pk_len];
        }

        // 3. 为每个导出生成 registerClientReference(...) 代理
        let msg_buf: [512]u8 = [0; 512];
        for (i of 0.._.export_count) {
            const key = _.exports[i];
            const is_default = bytesEql(key, "default");
            const msg_len = formatRscServerCallErrorMsg(_.meta.pretty_path, key, msg_buf[0..])?;

            if (is_default) {
                pos = copyInto(out, pos, "export default ")?;
            } else {
                pos = copyInto(out, pos, "export const ")?;
                pos = copyInto(out, pos, key)?;
                pos = copyInto(out, pos, " = ")?;
            }

            pos = copyInto(out, pos, _.cfg.server_register_client_reference)?;
            pos = copyInto(out, pos, "(() => { throw new Error(")?;
            pos = copyJsStringLiteral(out, pos, msg_buf[0..msg_len])?;
            pos = copyInto(out, pos, "); }, ")?;
            pos = copyJsStringLiteral(out, pos, mod_ref_path)?;
            pos = copyInto(out, pos, ", ")?;
            pos = copyJsStringLiteral(out, pos, key)?;
            pos = copyInto(out, pos, ");\n")?;
        }

        return pos;
    }
}
