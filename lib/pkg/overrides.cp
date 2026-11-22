// Source: Absorbed from /data/dev/bun/src/install/lockfile/OverrideMap.cp | Synced: 2026-11-22T05:48:05Z
// Pure-CP zero-allocation npm `overrides` & Yarn `resolutions` dependency
// version override resolver with `$pkg` root-dependency reference expansion,
// single-level key normalization, deterministic lexicographical sorting, and
// lightweight JSON object parsing/serialization.

import { PackageNameHash, hashPackageName } from "./types.cp";

pub const MAX_OVERRIDE_RULES: usize = 64;

/// 依赖覆写解析、校验与序列化错误集
pub err OverrideError {
    EmptyKey,
    EmptyValue,
    InvalidPackageName,
    NestedOverrideUnsupported,
    PatchOverrideUnsupported,
    UnresolvedDollarRef,
    InvalidJsonObject,
    CapacityExceeded,
    BufferTooSmall,
}

/// 覆写规则来源（npm `overrides` 或 Yarn `resolutions`）
pub enum OverrideSourceKind: u8 {
    npm_override = 0,
    yarn_resolution = 1,
}

/// 根包直接依赖条目（供 `$pkg` 覆写引用解析）
pub struct RootDepSpec {
    name: string,
    spec: string,
}

/// 单条已归一化、已解引用的依赖覆写规则
pub struct OverrideRule {
    name: string,
    name_hash: PackageNameHash,
    spec: string,
    source_kind: OverrideSourceKind,
    from_dollar_ref: bool = false,

    fn eql(other: OverrideRule): bool {
        return _.name_hash == other.name_hash &&
            std.mem.eql(u8, _.name, other.name) &&
            std.mem.eql(u8, _.spec, other.spec) &&
            _.source_kind == other.source_kind &&
            _.from_dollar_ref == other.from_dollar_ref;
    }
}

fn isJsonSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn skipJsonWs(s: string, start: usize): usize {
    let i = start;
    while (i < s.len && isJsonSpace(s[i])) {
        i += 1;
    }
    return i;
}

fn indexOfChar(s: string, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn startsWith(s: string, prefix: string): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return std.mem.eql(u8, s[0..prefix.len], prefix);
}

fn lexLessThan(a: string, b: string): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) {
            return a[i] < b[i];
        }
        i += 1;
    }
    return a.len < b.len;
}

/// 校验并归一化单层覆写包名（支持普通包 `lodash` 与作用域包 `@scope/pkg`）
/// - 若 `strip_glob_prefix` 为 true（Yarn `resolutions`），自动剥离前导 `**/`
/// - 拒绝空包名、非法作用域包（`@scope` 无 `/`）、多级嵌套路径（`foo/bar`、`@scope/a/b`）
pub fn normalizeOverrideKey(raw_key: string, strip_glob_prefix: bool): string! {
    let k = raw_key;
    if (strip_glob_prefix && startsWith(k, "**/")) {
        k = k[3..];
    }
    if (k.len == 0) {
        return err.EmptyKey;
    }
    if (k[0] == '@') {
        const first_slash = indexOfChar(k, '/') ?? return err.InvalidPackageName;
        if (first_slash <= 1 || first_slash + 1 >= k.len) {
            return err.InvalidPackageName;
        }
        if (indexOfChar(k[first_slash + 1 ..], '/') != null) {
            return err.NestedOverrideUnsupported;
        }
    } else if (indexOfChar(k, '/') != null) {
        return err.NestedOverrideUnsupported;
    }
    return k;
}

/// 解析 JSON 字符串字面量（从 `pos` 处的 `"` 开始），返回内容切片与结束后的索引
struct ParsedJsonStr {
    slice: string,
    next_pos: usize,
}

fn parseJsonString(s: string, start: usize): ParsedJsonStr! {
    if (start >= s.len || s[start] != '"') {
        return err.InvalidJsonObject;
    }
    let i = start + 1;
    while (i < s.len) {
        const c = s[i];
        if (c == '\\') {
            if (i + 1 >= s.len) {
                return err.InvalidJsonObject;
            }
            i += 2;
            continue;
        }
        if (c == '"') {
            return ParsedJsonStr{
                slice: s[start + 1 .. i],
                next_pos: i + 1,
            };
        }
        if (c < 0x20) {
            return err.InvalidJsonObject;
        }
        i += 1;
    }
    return err.InvalidJsonObject;
}

fn appendBytes(buf: []u8, pos: usize, s: string): usize! {
    if (pos + s.len > buf.len) {
        return err.BufferTooSmall;
    }
    @memcpy(buf[pos .. pos + s.len], s);
    return pos + s.len;
}

/// 零堆分配依赖版本覆写映射（对应 Bun `src/install/lockfile/OverrideMap.cp`）
pub struct OverrideMap {
    entries: [MAX_OVERRIDE_RULES]OverrideRule = undefined,
    len: usize = 0,

    static fn init(): OverrideMap {
        return OverrideMap{
            entries: undefined,
            len: 0,
        };
    }

    fn count(): usize {
        return _.len;
    }

    fn items(): []const OverrideRule {
        return _.entries[0.._.len];
    }

    /// 按包名查找覆写后的版本说明符
    fn get(name: string): string? {
        if (_.len == 0) {
            return null;
        }
        const target_hash = hashPackageName(name);
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].name_hash == target_hash && std.mem.eql(u8, _.entries[i].name, name)) {
                return _.entries[i].spec;
            }
            i += 1;
        }
        return null;
    }

    /// 按包名查找完整 `OverrideRule`
    fn getRule(name: string): OverrideRule? {
        if (_.len == 0) {
            return null;
        }
        const target_hash = hashPackageName(name);
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].name_hash == target_hash && std.mem.eql(u8, _.entries[i].name, name)) {
                return _.entries[i];
            }
            i += 1;
        }
        return null;
    }

    /// 按 `PackageNameHash` 查找覆写规则（对齐 Bun `OverrideMap.get(name_hash)`）
    fn getByHash(name_hash: PackageNameHash): OverrideRule? {
        if (_.len == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].name_hash == name_hash) {
                return _.entries[i];
            }
            i += 1;
        }
        return null;
    }

    /// 按包名字典序原地稳定排序（对齐 Bun `OverrideMap.sort()`）
    fn sort(*_) {
        let i: usize = 1;
        while (i < _.len) {
            const key_item = _.entries[i];
            let j: usize = i;
            while (j > 0 && lexLessThan(key_item.name, _.entries[j - 1].name)) {
                _.entries[j] = _.entries[j - 1];
                j -= 1;
            }
            _.entries[j] = key_item;
            i += 1;
        }
    }

    /// 内部插入或覆盖同名规则，并执行 `$pkg` 根依赖解引用与 `patch:` 防护
    fn putResolved(
        *_,
        normalized_key: string,
        raw_value: string,
        source_kind: OverrideSourceKind,
        root_deps: []const RootDepSpec,
    ): ! {
        if (raw_value.len == 0) {
            return err.EmptyValue;
        }
        if (startsWith(raw_value, "patch:")) {
            return err.PatchOverrideUnsupported;
        }

        let resolved_spec = raw_value;
        let from_dollar = false;

        if (raw_value[0] == '$') {
            const ref_name = raw_value[1..];
            if (ref_name.len == 0) {
                return err.UnresolvedDollarRef;
            }
            let found: string? = null;
            let idx: usize = 0;
            while (idx < root_deps.len) {
                if (std.mem.eql(u8, root_deps[idx].name, ref_name)) {
                    found = root_deps[idx].spec;
                    break;
                }
                idx += 1;
            }
            resolved_spec = found ?? return err.UnresolvedDollarRef;
            from_dollar = true;
        }

        const name_hash = hashPackageName(normalized_key);
        const rule = OverrideRule{
            name: normalized_key,
            name_hash: name_hash,
            spec: resolved_spec,
            source_kind: source_kind,
            from_dollar_ref: from_dollar,
        };

        // 同名覆写原地更新（保持确定性）
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].name_hash == name_hash && std.mem.eql(u8, _.entries[i].name, normalized_key)) {
                _.entries[i] = rule;
                return;
            }
            i += 1;
        }

        if (_.len >= MAX_OVERRIDE_RULES) {
            return err.CapacityExceeded;
        }
        _.entries[_.len] = rule;
        _.len += 1;
    }

    /// 添加一条 npm `overrides` 规则
    fn addOverride(*_, key: string, raw_value: string, root_deps: []const RootDepSpec)! {
        const norm_key = normalizeOverrideKey(key, false)?;
        _.putResolved(norm_key, raw_value, .npm_override, root_deps)?;
    }

    /// 添加一条 Yarn `resolutions` 规则（自动剥离前导 `**/`）
    fn addResolution(*_, raw_key: string, raw_value: string, root_deps: []const RootDepSpec)! {
        const norm_key = normalizeOverrideKey(raw_key, true)?;
        _.putResolved(norm_key, raw_value, .yarn_resolution, root_deps)?;
    }

    /// 解析 `package.json` 中的 `"overrides"` JSON 对象文本（支持 `"pkg": "ver"` 与 `"pkg": { ".": "ver" }`）
    fn parseOverridesObject(*_, json_obj: string, root_deps: []const RootDepSpec)! {
        let pos = skipJsonWs(json_obj, 0);
        if (pos >= json_obj.len || json_obj[pos] != '{') {
            return err.InvalidJsonObject;
        }
        pos = skipJsonWs(json_obj, pos + 1);
        if (pos < json_obj.len && json_obj[pos] == '}') {
            pos = skipJsonWs(json_obj, pos + 1);
            if (pos != json_obj.len) {
                return err.InvalidJsonObject;
            }
            return;
        }

        while (true) {
            pos = skipJsonWs(json_obj, pos);
            const key_res = parseJsonString(json_obj, pos)?;
            pos = skipJsonWs(json_obj, key_res.next_pos);
            if (pos >= json_obj.len || json_obj[pos] != ':') {
                return err.InvalidJsonObject;
            }
            pos = skipJsonWs(json_obj, pos + 1);
            if (pos >= json_obj.len) {
                return err.InvalidJsonObject;
            }

            let val_slice: string = "";
            if (json_obj[pos] == '"') {
                const val_res = parseJsonString(json_obj, pos)?;
                val_slice = val_res.slice;
                pos = val_res.next_pos;
            } else if (json_obj[pos] == '{') {
                // 单层 `{ ".": "version" }` 语法；若存在其他子键则报 NestedOverrideUnsupported
                pos = skipJsonWs(json_obj, pos + 1);
                if (pos >= json_obj.len || json_obj[pos] == '}') {
                    return err.NestedOverrideUnsupported;
                }
                let dot_val: string? = null;
                let prop_count: usize = 0;
                while (true) {
                    pos = skipJsonWs(json_obj, pos);
                    const sub_key = parseJsonString(json_obj, pos)?;
                    pos = skipJsonWs(json_obj, sub_key.next_pos);
                    if (pos >= json_obj.len || json_obj[pos] != ':') {
                        return err.InvalidJsonObject;
                    }
                    pos = skipJsonWs(json_obj, pos + 1);
                    if (pos >= json_obj.len || json_obj[pos] != '"') {
                        return err.NestedOverrideUnsupported;
                    }
                    const sub_val = parseJsonString(json_obj, pos)?;
                    pos = skipJsonWs(json_obj, sub_val.next_pos);
                    prop_count += 1;
                    if (std.mem.eql(u8, sub_key.slice, ".")) {
                        dot_val = sub_val.slice;
                    } else {
                        return err.NestedOverrideUnsupported;
                    }
                    if (pos < json_obj.len && json_obj[pos] == ',') {
                        pos += 1;
                        continue;
                    }
                    if (pos < json_obj.len && json_obj[pos] == '}') {
                        pos += 1;
                        break;
                    }
                    return err.InvalidJsonObject;
                }
                if (prop_count != 1) {
                    return err.NestedOverrideUnsupported;
                }
                val_slice = dot_val ?? return err.NestedOverrideUnsupported;
            } else {
                return err.InvalidJsonObject;
            }

            _.addOverride(key_res.slice, val_slice, root_deps)?;

            pos = skipJsonWs(json_obj, pos);
            if (pos < json_obj.len && json_obj[pos] == ',') {
                pos += 1;
                continue;
            }
            if (pos < json_obj.len && json_obj[pos] == '}') {
                pos = skipJsonWs(json_obj, pos + 1);
                if (pos != json_obj.len) {
                    return err.InvalidJsonObject;
                }
                break;
            }
            return err.InvalidJsonObject;
        }
    }

    /// 解析 `package.json` 中的 Yarn `"resolutions"` JSON 对象文本
    fn parseResolutionsObject(*_, json_obj: string, root_deps: []const RootDepSpec)! {
        let pos = skipJsonWs(json_obj, 0);
        if (pos >= json_obj.len || json_obj[pos] != '{') {
            return err.InvalidJsonObject;
        }
        pos = skipJsonWs(json_obj, pos + 1);
        if (pos < json_obj.len && json_obj[pos] == '}') {
            pos = skipJsonWs(json_obj, pos + 1);
            if (pos != json_obj.len) {
                return err.InvalidJsonObject;
            }
            return;
        }

        while (true) {
            pos = skipJsonWs(json_obj, pos);
            const key_res = parseJsonString(json_obj, pos)?;
            pos = skipJsonWs(json_obj, key_res.next_pos);
            if (pos >= json_obj.len || json_obj[pos] != ':') {
                return err.InvalidJsonObject;
            }
            pos = skipJsonWs(json_obj, pos + 1);
            const val_res = parseJsonString(json_obj, pos)?;
            pos = val_res.next_pos;

            _.addResolution(key_res.slice, val_res.slice, root_deps)?;

            pos = skipJsonWs(json_obj, pos);
            if (pos < json_obj.len && json_obj[pos] == ',') {
                pos += 1;
                continue;
            }
            if (pos < json_obj.len && json_obj[pos] == '}') {
                pos = skipJsonWs(json_obj, pos + 1);
                if (pos != json_obj.len) {
                    return err.InvalidJsonObject;
                }
                break;
            }
            return err.InvalidJsonObject;
        }
    }

    /// 序列化为紧凑 JSON 对象（如 `{"lodash":"4.17.21","react":"19.0.0"}`）
    fn toJson(buf: []u8): string! {
        let pos: usize = 0;
        pos = appendBytes(buf, pos, "{")?;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                pos = appendBytes(buf, pos, ",")?;
            }
            pos = appendBytes(buf, pos, "\"")?;
            pos = appendBytes(buf, pos, _.entries[i].name)?;
            pos = appendBytes(buf, pos, "\":\"")?;
            pos = appendBytes(buf, pos, _.entries[i].spec)?;
            pos = appendBytes(buf, pos, "\"")?;
            i += 1;
        }
        pos = appendBytes(buf, pos, "}")?;
        return buf[0..pos];
    }
}
