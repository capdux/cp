// Source: Absorbed from /data/dev/bun/src/bundler/options.cp | Synced: 2026-11-22T12:08:48Z
// Pure-CP zero-allocation bundler output path template engine (`[dir]`, `[name]`, `[ext]`,
// `[hash]`, `[target]`) with official entry/chunk/asset presets, POSIX/Windows slash
// normalization, strict placeholder & traversal validation, and deterministic truncated hex hash formatting.

/// Bundler 产物路径模板校验与格式化错误集
pub err PathTemplateError {
    EmptyTemplate,
    UnclosedPlaceholder,
    UnknownPlaceholder,
    MissingHashValue,
    UnsafePathTraversal,
    BufferTooSmall,
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

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (bytesEql(haystack[i..i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn writeByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn writeSliceWithStyle(out: []u8, start: usize, src: []const u8, style: PathSeparatorStyle): usize! {
    if (start + src.len > out.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        const c = src[i];
        if (style == .windows && c == '/') {
            out[start + i] = '\\';
        } else {
            out[start + i] = c;
        }
        i += 1;
    }
    return start + src.len;
}

fn hasTraversalSegment(path: []const u8): bool {
    let seg_start: usize = 0;
    let i: usize = 0;
    while (i <= path.len) {
        if (i == path.len || path[i] == '/' || path[i] == '\\') {
            const seg = path[seg_start..i];
            if (bytesEql(seg, "..")) {
                return true;
            }
            seg_start = i + 1;
        }
        i += 1;
    }
    return false;
}

/// 路径模板支持的 5 种官方占位符字段
pub enum PathTemplateField: u8 {
    dir = 0,
    name = 1,
    ext = 2,
    hash = 3,
    target = 4,

    /// 从占位符名（不含方括号）解析字段
    static fn fromName(s: []const u8): PathTemplateField? {
        if (bytesEql(s, "dir")) return .dir;
        if (bytesEql(s, "name")) return .name;
        if (bytesEql(s, "ext")) return .ext;
        if (bytesEql(s, "hash")) return .hash;
        if (bytesEql(s, "target")) return .target;
        return null;
    }

    /// 返回带方括号的规范令牌
    fn bracketToken(): []const u8 {
        if (_ == .dir) return "[dir]";
        if (_ == .name) return "[name]";
        if (_ == .ext) return "[ext]";
        if (_ == .hash) return "[hash]";
        return "[target]";
    }
}

/// 输出路径分隔符风格（对应上游 `writeReplacingSlashesOnWindows`）
pub enum PathSeparatorStyle: u8 {
    posix = 0,
    windows = 1,
}

/// 将 64 位内容哈希格式化为 8 位小写十六进制截断指纹（对应 Bun 产物 `[hash]`）
pub fn formatTruncatedHashHex(hash: u64, out: []u8): usize! {
    if (out.len < 8) {
        return err.BufferTooSmall;
    }
    const hex_chars = "0123456789abcdef";
    const h32: u32 = ((hash ^ (hash >> 32)) & 0xFFFF_FFFF) as u32;
    let i: usize = 0;
    while (i < 8) {
        const shift: u5 = ((7 - i) * 4) as u5;
        const nibble: usize = ((h32 >> shift) & 0xF) as usize;
        out[i] = hex_chars[nibble];
        i += 1;
    }
    return 8;
}

/// 路径模板占位符替换上下文（对应上游 `_PathTemplatePlaceholder`）
pub struct PathTemplatePlaceholder {
    dir: []const u8,
    name: []const u8,
    ext: []const u8,
    hash: u64?,
    target: []const u8,

    /// 空占位符上下文
    static fn empty(): PathTemplatePlaceholder {
        return PathTemplatePlaceholder{
            dir: "",
            name: "",
            ext: "",
            hash: null,
            target: "",
        };
    }

    /// 语义等价比较
    fn eql(other: PathTemplatePlaceholder): bool {
        if (!bytesEql(_.dir, other.dir) || !bytesEql(_.name, other.name) || !bytesEql(_.ext, other.ext) || !bytesEql(_.target, other.target)) {
            return false;
        }
        if (a := _.hash) {
            if (b := other.hash) {
                return a == b;
            }
            return false;
        }
        return other.hash == null;
    }
}

/// Bundler 产物命名路径模板（对应上游 `options.PathTemplate`）
pub struct PathTemplate {
    data: []const u8,
    placeholder: PathTemplatePlaceholder,

    /// 默认入口文件模板：`"[dir]/[name].[ext]"`
    static fn file(): PathTemplate {
        return PathTemplate{
            data: "[dir]/[name].[ext]",
            placeholder: PathTemplatePlaceholder.empty(),
        };
    }

    /// 带目标平台的入口文件模板：`"[dir]/[target]/[name].[ext]"`
    static fn fileWithTarget(): PathTemplate {
        return PathTemplate{
            data: "[dir]/[target]/[name].[ext]",
            placeholder: PathTemplatePlaceholder.empty(),
        };
    }

    /// 默认共享 Chunk 模板：`"./chunk-[hash].[ext]"`
    static fn chunk(): PathTemplate {
        return PathTemplate{
            data: "./chunk-[hash].[ext]",
            placeholder: PathTemplatePlaceholder{
                dir: "",
                name: "chunk",
                ext: "js",
                hash: null,
                target: "",
            },
        };
    }

    /// 带目标平台的共享 Chunk 模板：`"[dir]/[target]/chunk-[hash].[ext]"`
    static fn chunkWithTarget(): PathTemplate {
        return PathTemplate{
            data: "[dir]/[target]/chunk-[hash].[ext]",
            placeholder: PathTemplatePlaceholder{
                dir: "",
                name: "chunk",
                ext: "js",
                hash: null,
                target: "",
            },
        };
    }

    /// 默认静态资源模板：`"./[name]-[hash].[ext]"`
    static fn asset(): PathTemplate {
        return PathTemplate{
            data: "./[name]-[hash].[ext]",
            placeholder: PathTemplatePlaceholder.empty(),
        };
    }

    /// 带目标平台的静态资源模板：`"[dir]/[target]/[name]-[hash].[ext]"`
    static fn assetWithTarget(): PathTemplate {
        return PathTemplate{
            data: "[dir]/[target]/[name]-[hash].[ext]",
            placeholder: PathTemplatePlaceholder.empty(),
        };
    }

    /// 从自定义模板字符串构造
    static fn fromString(tpl: []const u8): PathTemplate {
        return PathTemplate{
            data: tpl,
            placeholder: PathTemplatePlaceholder.empty(),
        };
    }

    /// 返回绑定了新替换值的模板副本
    fn withValues(values: PathTemplatePlaceholder): PathTemplate {
        return PathTemplate{
            data: _.data,
            placeholder: values,
        };
    }

    /// 静态探测模板中是否引用了指定占位符（对应上游 `PathTemplate.needs`）
    fn needs(field: PathTemplateField): bool {
        return containsSubstr(_.data, field.bracketToken());
    }

    /// 严格校验模板语法与安全性（非空、括号配对、仅含合法占位符、无 `..` 路径穿越），返回占位符总数
    fn validate(): usize! {
        if (_.data.len == 0) {
            return err.EmptyTemplate;
        }
        if (hasTraversalSegment(_.data)) {
            return err.UnsafePathTraversal;
        }

        let count: usize = 0;
        let i: usize = 0;
        while (i < _.data.len) {
            if (_.data[i] == '[') {
                let depth: usize = 1;
                const start = i + 1;
                let j = start;
                while (j < _.data.len && depth > 0) {
                    if (_.data[j] == '[') {
                        depth += 1;
                    } else if (_.data[j] == ']') {
                        depth -= 1;
                        if (depth == 0) {
                            break;
                        }
                    }
                    j += 1;
                }
                if (depth != 0) {
                    return err.UnclosedPlaceholder;
                }
                const token = _.data[start..j];
                if (PathTemplateField.fromName(token) == null) {
                    return err.UnknownPlaceholder;
                }
                count += 1;
                i = j + 1;
                continue;
            }
            if (_.data[i] == ']') {
                return err.UnknownPlaceholder;
            }
            i += 1;
        }
        return count;
    }

    /// 以 POSIX `/` 风格按宽松兼容模式展开模板（对应上游 `PathTemplate.format`）
    fn format(out: []u8): usize! {
        return _.formatWithStyle(.posix, false, out);
    }

    /// 按指定分隔符风格与严格模式展开模板
    fn formatWithStyle(style: PathSeparatorStyle, strict: bool, out: []u8): usize! {
        if (strict) {
            * = _.validate()?;
        } else if (_.data.len == 0) {
            return err.EmptyTemplate;
        }

        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.data.len) {
            if (_.data[i] != '[') {
                const start = i;
                while (i < _.data.len && _.data[i] != '[') {
                    i += 1;
                }
                pos = writeSliceWithStyle(out, pos, _.data[start..i], style)?;
                continue;
            }

            // 找到匹配的 ']'
            let depth: usize = 1;
            const tok_start = i + 1;
            let j = tok_start;
            while (j < _.data.len && depth > 0) {
                if (_.data[j] == '[') {
                    depth += 1;
                } else if (_.data[j] == ']') {
                    depth -= 1;
                    if (depth == 0) {
                        break;
                    }
                }
                j += 1;
            }

            if (depth != 0) {
                if (strict) {
                    return err.UnclosedPlaceholder;
                }
                // 兼容上游宽松降级：输出字面 '[' 并继续
                pos = writeByte(out, pos, '[')?;
                i = tok_start;
                continue;
            }

            const token = _.data[tok_start..j];
            if (field := PathTemplateField.fromName(token)) {
                if (field == .dir) {
                    const dir_val = if (_.placeholder.dir.len > 0) _.placeholder.dir else ".";
                    pos = writeSliceWithStyle(out, pos, dir_val, style)?;
                } else if (field == .name) {
                    pos = writeSliceWithStyle(out, pos, _.placeholder.name, style)?;
                } else if (field == .ext) {
                    pos = writeSliceWithStyle(out, pos, _.placeholder.ext, style)?;
                } else if (field == .target) {
                    pos = writeSliceWithStyle(out, pos, _.placeholder.target, style)?;
                } else if (field == .hash) {
                    if (h := _.placeholder.hash) {
                        const hn = formatTruncatedHashHex(h, out[pos..])?;
                        pos += hn;
                    } else if (strict) {
                        return err.MissingHashValue;
                    }
                }
            } else {
                if (strict) {
                    return err.UnknownPlaceholder;
                }
                // 兼容上游：未知占位符剥离外层方括号输出字面名
                pos = writeSliceWithStyle(out, pos, token, style)?;
            }
            i = j + 1;
        }

        if (strict && hasTraversalSegment(out[0..pos])) {
            return err.UnsafePathTraversal;
        }
        return pos;
    }
}

/// 一步快捷格式化产物命名模板（严格模式）
pub fn formatBundlerNaming(tpl: []const u8, values: PathTemplatePlaceholder, style: PathSeparatorStyle, out: []u8): usize! {
    const pt = PathTemplate.fromString(tpl).withValues(values);
    return pt.formatWithStyle(style, true, out);
}
