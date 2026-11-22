// Source: Absorbed from /data/dev/bun/src/resolver/tsconfig_json.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/resolver/tsconfig.cp — JSONC tsconfig.json Parser & Path Alias Rewriter
// ============================================================================

const std = @import("std");
import { JSON, JsonValue, JsonTag } from "cap:json";

pub struct TsPathMapping {
    pattern: []const u8 = "",
    targets: []const []const u8 = [],
}

fn isWhitespaceByte(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/// Parses `tsconfig.json` (supporting JSONC `//` line comments, `/* ... */` block comments,
/// and trailing commas) and resolves `compilerOptions.paths` aliases such as `@/* -> src/*`.
pub struct TsConfigJson {
    extends: []const u8 = "",
    base_url: []const u8 = "",
    root_dir: []const u8 = "",
    out_dir: []const u8 = "",
    target: []const u8 = "",
    jsx: []const u8 = "",
    jsx_factory: []const u8 = "",
    jsx_fragment_factory: []const u8 = "",
    jsx_import_source: []const u8 = "",
    paths: []TsPathMapping = [],

    /// Strips JSONC single-line comments (`//`), block comments (`/* ... */`),
    /// and trailing commas before `}` or `]` into a valid RFC 8259 JSON buffer.
    pub static fn stripJsonc(allocator: std.mem.Allocator, src: []const u8): ([]u8)! {
        let out = allocator.alloc(u8, src.len)?;
        errdefer allocator.free(out);

        let i: usize = 0;
        let out_len: usize = 0;
        let in_string = false;

        while (i < src.len) {
            const c = src[i];
            if (in_string) {
                if (c == '\\' && i + 1 < src.len) {
                    out[out_len] = c;
                    out[out_len + 1] = src[i + 1];
                    out_len += 2;
                    i += 2;
                    continue;
                }
                if (c == '"') {
                    in_string = false;
                }
                out[out_len] = c;
                out_len += 1;
                i += 1;
                continue;
            }

            if (c == '"') {
                in_string = true;
                out[out_len] = c;
                out_len += 1;
                i += 1;
                continue;
            }

            // Single-line comment //
            if (c == '/' && i + 1 < src.len && src[i + 1] == '/') {
                i += 2;
                while (i < src.len && src[i] != '\n') {
                    i += 1;
                }
                continue;
            }

            // Block comment /* ... */
            if (c == '/' && i + 1 < src.len && src[i + 1] == '*') {
                i += 2;
                let closed = false;
                while (i + 1 < src.len) {
                    if (src[i] == '*' && src[i + 1] == '/') {
                        i += 2;
                        closed = true;
                        break;
                    }
                    i += 1;
                }
                if (!closed) {
                    return error.UnterminatedComment;
                }
                continue;
            }

            // Trailing comma before '}' or ']' (looking past whitespace and comments)
            if (c == ',') {
                let look = i + 1;
                while (look < src.len) {
                    const lc = src[look];
                    if (isWhitespaceByte(lc)) {
                        look += 1;
                        continue;
                    }
                    if (lc == '/' && look + 1 < src.len && src[look + 1] == '/') {
                        look += 2;
                        while (look < src.len && src[look] != '\n') look += 1;
                        continue;
                    }
                    if (lc == '/' && look + 1 < src.len && src[look + 1] == '*') {
                        look += 2;
                        while (look + 1 < src.len && !(src[look] == '*' && src[look + 1] == '/')) look += 1;
                        if (look + 1 < src.len) look += 2;
                        continue;
                    }
                    break;
                }
                if (look < src.len && (src[look] == '}' || src[look] == ']')) {
                    i += 1;
                    continue;
                }
            }

            out[out_len] = c;
            out_len += 1;
            i += 1;
        }

        const shrunk = allocator.realloc(out, out_len) catch {
            return out[0..out_len];
        };
        return shrunk;
    }

    /// Parses a JSONC `tsconfig.json` string into `TsConfigJson`.
    pub static fn parse(allocator: std.mem.Allocator, src: []const u8): TsConfigJson! {
        const clean_json = TsConfigJson.stripJsonc(allocator, src)?;
        const root = JSON.parse(allocator, clean_json)?;
        if (root.tag != .Object) {
            return error.InvalidTsConfig;
        }

        let cfg = TsConfigJson{};
        if (v := root.get("extends")) {
            if (s := v.asString()) cfg.extends = s;
        }

        const co = root.get("compilerOptions") ?? root;
        if (co.tag == .Object) {
            if (v := co.get("baseUrl")) {
                if (s := v.asString()) cfg.base_url = s;
            }
            if (v := co.get("rootDir")) {
                if (s := v.asString()) cfg.root_dir = s;
            }
            if (v := co.get("outDir")) {
                if (s := v.asString()) cfg.out_dir = s;
            }
            if (v := co.get("target")) {
                if (s := v.asString()) cfg.target = s;
            }
            if (v := co.get("jsx")) {
                if (s := v.asString()) cfg.jsx = s;
            }
            if (v := co.get("jsxFactory")) {
                if (s := v.asString()) cfg.jsx_factory = s;
            }
            if (v := co.get("jsxFragmentFactory")) {
                if (s := v.asString()) cfg.jsx_fragment_factory = s;
            }
            if (v := co.get("jsxImportSource")) {
                if (s := v.asString()) cfg.jsx_import_source = s;
            }
            if (pv := co.get("paths")) {
                if (pv.tag == .Object) {
                    let mappings: std.ArrayList(TsPathMapping) = .empty;
                    errdefer mappings.deinit(allocator);

                    for (field of pv.obj_val) {
                        if (field.value.tag != .Array) continue;
                        let targets: std.ArrayList(string) = .empty;
                        errdefer targets.deinit(allocator);

                        for (elem of field.value.arr_val) {
                            if (t := elem.asString()) {
                                targets.append(allocator, t)?;
                            }
                        }
                        if (targets.items.len > 0) {
                            mappings.append(allocator, TsPathMapping{
                                pattern: field.key,
                                targets: targets.toOwnedSlice(allocator)?,
                            })?;
                        } else {
                            targets.deinit(allocator);
                        }
                    }
                    cfg.paths = mappings.toOwnedSlice(allocator)?;
                }
            }
        }

        return cfg;
    }

    pub fn hasBaseUrl(): bool {
        return _.base_url.len > 0;
    }

    fn applyBaseUrl(allocator: std.mem.Allocator, target: []const u8): ([]const u8)! {
        let base = _.base_url;
        if (base.len == 0 || std.mem.eql(u8, base, ".") || std.mem.eql(u8, base, "./")) {
            return target;
        }
        if (std.mem.startsWith(u8, target, "/")) {
            return target;
        }
        if (std.mem.startsWith(u8, base, "./")) {
            base = base[2..];
        }
        while (base.len > 0 && base[base.len - 1] == '/') {
            base = base[0 .. base.len - 1];
        }
        if (base.len == 0) return target;

        let clean_target = target;
        if (std.mem.startsWith(u8, clean_target, "./")) {
            clean_target = clean_target[2..];
        }
        return std.fmt.allocPrint(allocator, "{s}/{s}", .{ base, clean_target });
    }

    /// Resolves `specifier` against `compilerOptions.paths` (e.g. `@/* -> src/*`).
    /// Longest prefix match wins per TypeScript module resolution specification.
    pub fn resolveAlias(allocator: std.mem.Allocator, specifier: []const u8): ([]const u8)? {
        if (_.paths.len == 0) return null;

        let matched_idx: usize = 0;
        let has_match = false;
        let best_score: usize = 0;
        let matched_wildcard: []const u8 = "";

        let i: usize = 0;
        while (i < _.paths.len) : (i += 1) {
            const mapping = _.paths[i];
            if (mapping.targets.len == 0) continue;

            if (star_idx := std.mem.indexOfScalar(u8, mapping.pattern, '*')) {
                const prefix = mapping.pattern[0..star_idx];
                const suffix = mapping.pattern[star_idx + 1 ..];
                if (std.mem.startsWith(u8, specifier, prefix) &&
                    std.mem.endsWith(u8, specifier, suffix) &&
                    specifier.len >= prefix.len + suffix.len)
                {
                    const score = prefix.len * 1024 + suffix.len + 1;
                    if (!has_match || score > best_score) {
                        has_match = true;
                        best_score = score;
                        matched_idx = i;
                        matched_wildcard = specifier[prefix.len .. specifier.len - suffix.len];
                    }
                }
            } else if (std.mem.eql(u8, mapping.pattern, specifier)) {
                const score = specifier.len * 1024 + 512;
                if (!has_match || score > best_score) {
                    has_match = true;
                    best_score = score;
                    matched_idx = i;
                    matched_wildcard = "";
                }
            }
        }

        if (!has_match) return null;

        const target_tpl = _.paths[matched_idx].targets[0];
        let rewritten: []const u8 = target_tpl;
        if (star_pos := std.mem.indexOfScalar(u8, target_tpl, '*')) {
            const prefix = target_tpl[0..star_pos];
            const suffix = target_tpl[star_pos + 1 ..];
            rewritten = std.fmt.allocPrint(allocator, "{s}{s}{s}", .{ prefix, matched_wildcard, suffix }) catch {
                return null;
            };
        }

        const resolved = _.applyBaseUrl(allocator, rewritten) catch {
            return null;
        };
        return resolved;
    }
}
