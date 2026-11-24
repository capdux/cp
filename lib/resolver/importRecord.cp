// Source: Absorbed from /data/dev/bun/src/options_types/import_record.cp | Synced: 2026-11-23T22:02:19Z
// Pure-CP zero-allocation import record metadata, 12-kind ImportKind classifier,
// 15-bit packed ImportRecordFlags, ImportRecordTag, ImportRecordPrintMode,
// namespace-aware printable path formatter, and fixed-capacity ImportRecordList.

pub const IMPORT_RECORD_INVALID_SOURCE: u32 = 4294967295;
pub const IMPORT_RECORD_MAX_COUNT: usize = 256;

pub error ImportRecordError {
    CapacityExceeded,
    BufferTooSmall,
    IndexOutOfBounds,
}

/// Complete 12-variant import site classification mirrored from Bun `ImportKind`.
pub enum ImportKind: u8 {
    EntryPointRun = 0,
    EntryPointBuild = 1,
    Stmt = 2,
    Require = 3,
    Dynamic = 4,
    RequireResolve = 5,
    At = 6,
    AtConditional = 7,
    Url = 8,
    Composes = 9,
    HtmlManifest = 10,
    Internal = 11,

    /// Canonical kebab-case/metafile label (matches Bun `ImportKind.all_labels`).
    fn label(): []const u8 {
        return switch (_) {
            .EntryPointRun => "entry-point-run",
            .EntryPointBuild => "entry-point-build",
            .Stmt => "import-statement",
            .Require => "require-call",
            .Dynamic => "dynamic-import",
            .RequireResolve => "require-resolve",
            .At => "import-rule",
            .AtConditional => "import-rule-conditional",
            .Url => "url-token",
            .Composes => "composes",
            .HtmlManifest => "html_manifest",
            .Internal => "internal",
        };
    }

    /// Human-readable diagnostic label (matches Bun `ImportKind.error_labels`).
    fn errorLabel(): []const u8 {
        return switch (_) {
            .EntryPointRun => "entry point (run)",
            .EntryPointBuild => "entry point (build)",
            .Stmt => "import",
            .Require => "require()",
            .Dynamic => "import()",
            .RequireResolve => "require.resolve()",
            .At => "@import",
            .AtConditional => "@import",
            .Url => "url()",
            .Composes => "composes",
            .HtmlManifest => "HTML import",
            .Internal => "<bun internal>",
        };
    }

    /// Parses a canonical `ImportKind` from its string label.
    static fn fromLabel(s: []const u8): ImportKind? {
        if (s == "entry-point-run") return .EntryPointRun;
        if (s == "entry-point-build") return .EntryPointBuild;
        if (s == "import-statement") return .Stmt;
        if (s == "require-call") return .Require;
        if (s == "dynamic-import") return .Dynamic;
        if (s == "require-resolve") return .RequireResolve;
        if (s == "import-rule") return .At;
        if (s == "import-rule-conditional") return .AtConditional;
        if (s == "url-token") return .Url;
        if (s == "composes") return .Composes;
        if (s == "html_manifest") return .HtmlManifest;
        if (s == "internal") return .Internal;
        return null;
    }

    fn isCommonJS(): bool {
        return _ == .Require || _ == .RequireResolve;
    }

    fn isFromCSS(): bool {
        return _ == .At || _ == .AtConditional || _ == .Url || _ == .Composes;
    }

    fn isEntryPoint(): bool {
        return _ == .EntryPointRun || _ == .EntryPointBuild;
    }
}

/// 15-bit semantic flags for an `ImportRecord` (exact bit layout of Bun `ImportRecordFlags`).
pub struct ImportRecordFlags {
    handles_import_errors: bool = false,
    is_internal: bool = false,
    is_unused: bool = false,
    contains_import_star: bool = false,
    contains_default_alias: bool = false,
    contains_es_module_alias: bool = false,
    calls_runtime_re_export_fn: bool = false,
    is_inside_try_body: bool = false,
    was_originally_bare_import: bool = false,
    was_originally_require: bool = false,
    was_injected_by_macro: bool = false,
    is_external_without_side_effects: bool = false,
    print_namespace_in_path: bool = false,
    wrap_with_to_esm: bool = false,
    wrap_with_to_commonjs: bool = false,

    /// Packs all 15 boolean flags into a 16-bit integer (bit 15 reserved as 0).
    fn toBits(): u16 {
        let bits: u16 = 0;
        if (_.handles_import_errors) bits |= 1 << 0;
        if (_.is_internal) bits |= 1 << 1;
        if (_.is_unused) bits |= 1 << 2;
        if (_.contains_import_star) bits |= 1 << 3;
        if (_.contains_default_alias) bits |= 1 << 4;
        if (_.contains_es_module_alias) bits |= 1 << 5;
        if (_.calls_runtime_re_export_fn) bits |= 1 << 6;
        if (_.is_inside_try_body) bits |= 1 << 7;
        if (_.was_originally_bare_import) bits |= 1 << 8;
        if (_.was_originally_require) bits |= 1 << 9;
        if (_.was_injected_by_macro) bits |= 1 << 10;
        if (_.is_external_without_side_effects) bits |= 1 << 11;
        if (_.print_namespace_in_path) bits |= 1 << 12;
        if (_.wrap_with_to_esm) bits |= 1 << 13;
        if (_.wrap_with_to_commonjs) bits |= 1 << 14;
        return bits;
    }

    /// Unpacks a 16-bit integer back into `ImportRecordFlags`.
    static fn fromBits(bits: u16): ImportRecordFlags {
        return ImportRecordFlags{
            handles_import_errors: (bits & (1 << 0)) != 0,
            is_internal: (bits & (1 << 1)) != 0,
            is_unused: (bits & (1 << 2)) != 0,
            contains_import_star: (bits & (1 << 3)) != 0,
            contains_default_alias: (bits & (1 << 4)) != 0,
            contains_es_module_alias: (bits & (1 << 5)) != 0,
            calls_runtime_re_export_fn: (bits & (1 << 6)) != 0,
            is_inside_try_body: (bits & (1 << 7)) != 0,
            was_originally_bare_import: (bits & (1 << 8)) != 0,
            was_originally_require: (bits & (1 << 9)) != 0,
            was_injected_by_macro: (bits & (1 << 10)) != 0,
            is_external_without_side_effects: (bits & (1 << 11)) != 0,
            print_namespace_in_path: (bits & (1 << 12)) != 0,
            wrap_with_to_esm: (bits & (1 << 13)) != 0,
            wrap_with_to_commonjs: (bits & (1 << 14)) != 0,
        };
    }

    /// True if resolution failure should be deferred/suppressed (e.g. `try { require('x') } catch {}`).
    fn canSuppressResolveError(): bool {
        return _.handles_import_errors || _.is_inside_try_body;
    }

    /// True if this record can be safely elided by tree-shaking when unused.
    fn canPruneWhenUnused(): bool {
        return _.is_unused && _.is_external_without_side_effects;
    }

    /// True if module interop helper wrapping (`__toESM` / `__toCommonJS`) is required.
    fn needsModuleInteropWrap(): bool {
        return _.wrap_with_to_esm || _.wrap_with_to_commonjs;
    }
}

/// Semantic origin tag for an import record (mirrors Bun `ImportRecordTag`).
pub enum ImportRecordTag: u8 {
    None = 0,
    Bun = 1,
    Builtin = 2,
    Runtime = 3,
    Macro = 4,
    BakeResolveToSsrGraph = 5,
    Tailwind = 6,

    fn isRuntime(): bool {
        return _ == .Runtime;
    }

    /// Mirrors Bun: `tag >= .runtime` indicates an internal/special compiler-managed record.
    fn isInternal(): bool {
        return @intFromEnum(_) >= @intFromEnum(ImportRecordTag.Runtime);
    }

    /// Classifies an import specifier prefix into its default `ImportRecordTag`.
    static fn fromSpecifier(spec: []const u8): ImportRecordTag {
        if (spec == "bun") return .Bun;
        if (hasPrefix(spec, "bun:") || hasPrefix(spec, "node:") || hasPrefix(spec, "cap:")) {
            return .Builtin;
        }
        if (hasPrefix(spec, "macro:")) return .Macro;
        if (spec == "tailwindcss") return .Tailwind;
        return .None;
    }
}

/// Code printer mode for emitting an import path (mirrors Bun `ImportRecordPrintMode`).
pub enum ImportRecordPrintMode: u8 {
    Normal = 0,
    ImportPath = 1,
    Css = 2,
    NapiModule = 3,
}

/// Byte offset and length of the import specifier in source text.
pub struct ImportSourceRange {
    loc: i32 = 0,
    len: i32 = 0,
}

/// Zero-allocation import record preserving both original specifier (for metafile)
/// and resolved path + namespace (for plugin/printer pipeline).
pub struct ImportRecord {
    range: ImportSourceRange = ImportSourceRange{},
    path: []const u8 = "",
    original_path: []const u8 = "",
    namespace: []const u8 = "file",
    kind: ImportKind = .Stmt,
    tag: ImportRecordTag = .None,
    source_index: u32 = IMPORT_RECORD_INVALID_SOURCE,
    flags: ImportRecordFlags = ImportRecordFlags{},

    static fn init(specifier: []const u8, kind: ImportKind, loc: i32, len: i32): ImportRecord {
        const tag = ImportRecordTag.fromSpecifier(specifier);
        let flags = ImportRecordFlags{};
        if (kind == .Require) flags.was_originally_require = true;
        if (tag.isInternal()) flags.is_internal = true;
        return ImportRecord{
            range: ImportSourceRange{ loc: loc, len: len },
            path: specifier,
            original_path: specifier,
            namespace: "file",
            kind: kind,
            tag: tag,
            source_index: IMPORT_RECORD_INVALID_SOURCE,
            flags: flags,
        };
    }

    /// Updates the resolved target while preserving `original_path` for metafile output.
    /// Automatically sets `print_namespace_in_path` when a non-default plugin namespace is assigned.
    fn resolveTo(*_, resolved_path: []const u8, ns: []const u8, source_idx: u32) {
        _.path = resolved_path;
        _.namespace = if (ns.len == 0) "file" else ns;
        _.source_index = source_idx;
        if (_.namespace != "file") {
            _.flags.print_namespace_in_path = true;
        }
    }

    fn isResolved(): bool {
        return _.source_index != IMPORT_RECORD_INVALID_SOURCE;
    }

    fn shouldPrintNamespace(): bool {
        return _.flags.print_namespace_in_path && _.namespace.len > 0 && _.namespace != "file";
    }

    /// Formats the emitted path into caller-provided `out` buffer (`"ns:path"` when namespace printing is enabled,
    /// otherwise `"path"`). Returns the written slice.
    fn formatPrintablePath(out: []u8): []const u8 ! ImportRecordError {
        if (_.shouldPrintNamespace()) {
            const need = _.namespace.len + 1 + _.path.len;
            if (need > out.len) return err.BufferTooSmall;
            let pos: usize = 0;
            let i: usize = 0;
            while (i < _.namespace.len) {
                out[pos] = _.namespace[i];
                pos += 1;
                i += 1;
            }
            out[pos] = ':';
            pos += 1;
            i = 0;
            while (i < _.path.len) {
                out[pos] = _.path[i];
                pos += 1;
                i += 1;
            }
            return out[0..pos];
        }
        if (_.path.len > out.len) return err.BufferTooSmall;
        let i: usize = 0;
        while (i < _.path.len) {
            out[i] = _.path[i];
            i += 1;
        }
        return out[0.._.path.len];
    }
}

/// Fixed-capacity zero-allocation list of `ImportRecord` entries for a parsed module.
pub struct ImportRecordList {
    items: [IMPORT_RECORD_MAX_COUNT]ImportRecord = [ImportRecord{}; IMPORT_RECORD_MAX_COUNT],
    count: usize = 0,

    static fn init(): ImportRecordList {
        return ImportRecordList{};
    }

    fn len(): usize {
        return _.count;
    }

    fn isEmpty(): bool {
        return _.count == 0;
    }

    fn clear(*_) {
        _.count = 0;
    }

    fn push(*_, rec: ImportRecord): usize ! ImportRecordError {
        if (_.count >= IMPORT_RECORD_MAX_COUNT) {
            return err.CapacityExceeded;
        }
        const idx = _.count;
        _.items[idx] = rec;
        _.count += 1;
        return idx;
    }

    fn get(idx: usize): ImportRecord ! ImportRecordError {
        if (idx >= _.count) return err.IndexOutOfBounds;
        return _.items[idx];
    }

    fn markUnused(*_, idx: usize): bool ! ImportRecordError {
        if (idx >= _.count) return err.IndexOutOfBounds;
        _.items[idx].flags.is_unused = true;
        return _.items[idx].flags.canPruneWhenUnused();
    }

    fn countByKind(kind: ImportKind): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (_.items[i].kind == kind) total += 1;
            i += 1;
        }
        return total;
    }

    fn countActive(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (!_.items[i].flags.is_unused) total += 1;
            i += 1;
        }
        return total;
    }

    fn countPrunable(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (_.items[i].flags.canPruneWhenUnused()) total += 1;
            i += 1;
        }
        return total;
    }
}

fn hasPrefix(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}
