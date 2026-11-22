// Source: Absorbed from /data/dev/bun/src/cli/pm_view_command.cp | Synced: 2026-11-22T06:31:59Z
// Pure-CP zero-allocation `bun info` / `bun pm view` package metadata query & card renderer:
// scoped spec parser, registry URL encoder, dot/bracket property path parser,
// human-readable metadata card formatter, recent-versions hint, and JSON error formatter.

/// 属性查询路径允许的最大分段数（如 `repository.url`、`maintainers[0].email`）
pub const PM_VIEW_MAX_PATH_SEGMENTS: usize = 16;

/// 版本未命中时提示的最近版本最大展示条数（对齐 Bun `max_versions_to_display = 5`）
pub const PM_VIEW_RECENT_VERSIONS_LIMIT: usize = 5;

/// `bun info` / `bun pm view` 纯逻辑解析与格式化错误集
pub err PmViewError {
    InvalidSpec,
    InvalidPropertyPath,
    TooManyPathSegments,
    BufferTooSmall,
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendChar(out: []u8, pos: usize, ch: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = ch;
    return pos + 1;
}

fn appendU64(out: []u8, start_pos: usize, val: u64): usize! {
    if (val == 0) {
        return appendChar(out, start_pos, '0');
    }
    let digits: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        digits[n] = (('0' as u64) + (v % 10)) as u8;
        v = v / 10;
        n += 1;
    }
    let pos = start_pos;
    let i = n;
    while (i > 0) {
        i -= 1;
        pos = appendChar(out, pos, digits[i])?;
    }
    return pos;
}

fn appendHumanSize(out: []u8, start_pos: usize, bytes: u64): usize! {
    if (bytes < 1024) {
        let pos = appendU64(out, start_pos, bytes)?;
        return appendBytes(out, pos, " B");
    }
    if (bytes < 1024 * 1024) {
        const tenths = (bytes * 10) / 1024;
        let pos = appendU64(out, start_pos, tenths / 10)?;
        pos = appendChar(out, pos, '.')?;
        pos = appendU64(out, pos, tenths % 10)?;
        return appendBytes(out, pos, " KB");
    }
    const tenths = (bytes * 10) / (1024 * 1024);
    let pos = appendU64(out, start_pos, tenths / 10)?;
    pos = appendChar(out, pos, '.')?;
    pos = appendU64(out, pos, tenths % 10)?;
    return appendBytes(out, pos, " MB");
}

fn appendJsonEscaped(out: []u8, start_pos: usize, s: []const u8): usize! {
    let pos = appendChar(out, start_pos, '"')?;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            pos = appendBytes(out, pos, "\\\"")?;
        } else if (c == '\\') {
            pos = appendBytes(out, pos, "\\\\")?;
        } else if (c == '\n') {
            pos = appendBytes(out, pos, "\\n")?;
        } else if (c == '\r') {
            pos = appendBytes(out, pos, "\\r")?;
        } else if (c == '\t') {
            pos = appendBytes(out, pos, "\\t")?;
        } else if (c < 0x20) {
            pos = appendBytes(out, pos, "\\u00")?;
            const hex = "0123456789abcdef";
            pos = appendChar(out, pos, hex[(c >> 4) as usize])?;
            pos = appendChar(out, pos, hex[(c & 0x0f) as usize])?;
        } else {
            pos = appendChar(out, pos, c)?;
        }
        i += 1;
    }
    return appendChar(out, pos, '"');
}

fn parseUsizeDigits(s: []const u8): usize? {
    if (s.len == 0) {
        return null;
    }
    let val: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        val = val * 10 + ((c - '0') as usize);
        i += 1;
    }
    return val;
}

/// 解析后的 `bun info <pkg>[@<ver>]` 查询目标
pub struct PmViewSpec {
    name: []const u8 = "",
    version: []const u8 = "latest",
    is_current_dir: bool = false,

    /// 解析包名与可选版本后缀（支持 `.` / 空串、非作用域包、作用域包 `@scope/pkg@1.2.3`）
    static fn parse(raw: []const u8): PmViewSpec! {
        if (raw.len == 0 || (raw.len == 1 && raw[0] == '.')) {
            return PmViewSpec{
                name: "",
                version: "latest",
                is_current_dir: true,
            };
        }

        // 从索引 1 起查找版本分隔符 `@`，保留作用域前缀 `@scope/`
        let at_pos: usize? = null;
        let i: usize = 1;
        while (i < raw.len) {
            if (raw[i] == '@') {
                at_pos = i;
                break;
            }
            i += 1;
        }

        let pkg_name = raw;
        let pkg_ver: []const u8 = "latest";
        if (pos := at_pos) {
            pkg_name = raw[0..pos];
            if (pos + 1 >= raw.len) {
                return err.InvalidSpec;
            }
            pkg_ver = raw[pos + 1..];
        }

        if (pkg_name.len == 0) {
            return err.InvalidSpec;
        }
        // 作用域包必须包含 `/` 且 `/` 两侧均非空
        if (pkg_name[0] == '@') {
            let slash_pos: usize? = null;
            let k: usize = 1;
            while (k < pkg_name.len) {
                if (pkg_name[k] == '/') {
                    slash_pos = k;
                    break;
                }
                k += 1;
            }
            const sp = slash_pos ?? return err.InvalidSpec;
            if (sp <= 1 || sp + 1 >= pkg_name.len) {
                return err.InvalidSpec;
            }
        }

        return PmViewSpec{
            name: pkg_name,
            version: pkg_ver,
            is_current_dir: false,
        };
    }

    fn isScoped(): bool {
        return _.name.len > 0 && _.name[0] == '@';
    }

    /// 构造 npm Registry 全量 Manifest 请求 URL（自动去除末尾 `/` 并将作用域 `/` 编码为 `%2F`）
    fn buildRegistryUrl(registry_base: []const u8, out: []u8): usize! {
        if (_.name.len == 0) {
            return err.InvalidSpec;
        }
        let base_end = registry_base.len;
        while (base_end > 0 && registry_base[base_end - 1] == '/') {
            base_end -= 1;
        }
        let pos = appendBytes(out, 0, registry_base[0..base_end])?;
        pos = appendChar(out, pos, '/')?;
        let i: usize = 0;
        while (i < _.name.len) {
            if (_.name[i] == '/') {
                pos = appendBytes(out, pos, "%2F")?;
            } else {
                pos = appendChar(out, pos, _.name[i])?;
            }
            i += 1;
        }
        return pos;
    }
}

/// 属性路径单段（键名或数组数字下标）
pub struct PmPropertyPathSegment {
    key: []const u8 = "",
    index: usize? = null,

    fn isIndex(): bool {
        return _.index != null;
    }
}

/// `bun info <pkg> <property-path>` 点分与下标属性路径解析器
///
/// 支持 `version`、`repository.url`、`maintainers[0].email`、`keywords.0` 等形式。
pub struct PmPropertyPath {
    segments: [PM_VIEW_MAX_PATH_SEGMENTS]PmPropertyPathSegment = [PmPropertyPathSegment{}; PM_VIEW_MAX_PATH_SEGMENTS],
    len: usize = 0,

    static fn parse(raw: []const u8): PmPropertyPath! {
        if (raw.len == 0) {
            return err.InvalidPropertyPath;
        }
        let path = PmPropertyPath{};
        let i: usize = 0;
        while (i < raw.len) {
            if (raw[i] == '.') {
                return err.InvalidPropertyPath;
            }
            if (raw[i] == '[') {
                i += 1;
                const idx_start = i;
                while (i < raw.len && raw[i] != ']') {
                    i += 1;
                }
                if (i >= raw.len || i == idx_start) {
                    return err.InvalidPropertyPath;
                }
                const idx_val = parseUsizeDigits(raw[idx_start..i]) ?? return err.InvalidPropertyPath;
                if (path.len >= PM_VIEW_MAX_PATH_SEGMENTS) {
                    return err.TooManyPathSegments;
                }
                path.segments[path.len] = PmPropertyPathSegment{
                    key: raw[idx_start..i],
                    index: idx_val,
                };
                path.len += 1;
                i += 1; // skip ']'
                if (i < raw.len) {
                    if (raw[i] == '.') {
                        i += 1;
                        if (i >= raw.len) {
                            return err.InvalidPropertyPath;
                        }
                    } else if (raw[i] != '[') {
                        return err.InvalidPropertyPath;
                    }
                }
                continue;
            }

            const seg_start = i;
            while (i < raw.len && raw[i] != '.' && raw[i] != '[') {
                if (raw[i] == ']') {
                    return err.InvalidPropertyPath;
                }
                i += 1;
            }
            const seg = raw[seg_start..i];
            if (seg.len == 0) {
                return err.InvalidPropertyPath;
            }
            if (path.len >= PM_VIEW_MAX_PATH_SEGMENTS) {
                return err.TooManyPathSegments;
            }
            path.segments[path.len] = PmPropertyPathSegment{
                key: seg,
                index: parseUsizeDigits(seg),
            };
            path.len += 1;

            if (i < raw.len && raw[i] == '.') {
                i += 1;
                if (i >= raw.len) {
                    return err.InvalidPropertyPath;
                }
            }
        }
        return path;
    }

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): PmPropertyPathSegment? {
        if (idx >= _.len) {
            return null;
        }
        return _.segments[idx];
    }
}

/// 分发包完整性与体积元数据
pub struct PmViewDistInfo {
    tarball: []const u8 = "",
    shasum: []const u8 = "",
    integrity: []const u8 = "",
    unpacked_size: u64 = 0,

    fn isEmpty(): bool {
        return _.tarball.len == 0 && _.shasum.len == 0 && _.integrity.len == 0 && _.unpacked_size == 0;
    }
}

/// 键值条目（用于 `dependencies`、`dist-tags`）
pub struct PmViewKv {
    key: []const u8 = "",
    value: []const u8 = "",
}

/// 维护者信息条目
pub struct PmViewMaintainer {
    name: []const u8 = "",
    email: []const u8 = "",
}

/// `bun info` / `bun pm view` 完整包元数据卡片输入
pub struct PmViewSummary {
    name: []const u8 = "",
    version: []const u8 = "",
    license: []const u8 = "",
    description: []const u8 = "",
    homepage: []const u8 = "",
    versions_count: usize = 0,
    keywords: []const []const u8 = [],
    dependencies: []const PmViewKv = [],
    dist: PmViewDistInfo = PmViewDistInfo{},
    dist_tags: []const PmViewKv = [],
    maintainers: []const PmViewMaintainer = [],
    published_at: []const u8 = "",

    /// 渲染对齐 Bun `pm_view_command.cp` 的纯文本信息卡片
    fn formatCard(out: []u8): usize! {
        let pos = appendBytes(out, 0, _.name)?;
        pos = appendChar(out, pos, '@')?;
        pos = appendBytes(out, pos, _.version)?;
        pos = appendBytes(out, pos, " | ")?;
        pos = appendBytes(out, pos, _.license)?;
        pos = appendBytes(out, pos, " | deps: ")?;
        pos = appendU64(out, pos, _.dependencies.len as u64)?;
        pos = appendBytes(out, pos, " | versions: ")?;
        pos = appendU64(out, pos, _.versions_count as u64)?;
        pos = appendChar(out, pos, '\n')?;

        if (_.description.len > 0) {
            pos = appendBytes(out, pos, _.description)?;
            pos = appendChar(out, pos, '\n')?;
        }
        if (_.homepage.len > 0) {
            pos = appendBytes(out, pos, _.homepage)?;
            pos = appendChar(out, pos, '\n')?;
        }
        if (_.keywords.len > 0) {
            pos = appendBytes(out, pos, "keywords: ")?;
            let i: usize = 0;
            while (i < _.keywords.len) {
                if (i > 0) {
                    pos = appendBytes(out, pos, ", ")?;
                }
                pos = appendBytes(out, pos, _.keywords[i])?;
                i += 1;
            }
            pos = appendChar(out, pos, '\n')?;
        }

        if (_.dependencies.len > 0) {
            pos = appendBytes(out, pos, "\ndependencies (")?;
            pos = appendU64(out, pos, _.dependencies.len as u64)?;
            pos = appendBytes(out, pos, "):\n")?;
            let i: usize = 0;
            while (i < _.dependencies.len) {
                pos = appendBytes(out, pos, "- ")?;
                pos = appendBytes(out, pos, _.dependencies[i].key)?;
                pos = appendBytes(out, pos, ": ")?;
                pos = appendBytes(out, pos, _.dependencies[i].value)?;
                pos = appendChar(out, pos, '\n')?;
                i += 1;
            }
        }

        if (!_.dist.isEmpty()) {
            pos = appendBytes(out, pos, "\ndist\n")?;
            if (_.dist.tarball.len > 0) {
                pos = appendBytes(out, pos, " .tarball: ")?;
                pos = appendBytes(out, pos, _.dist.tarball)?;
                pos = appendChar(out, pos, '\n')?;
            }
            if (_.dist.shasum.len > 0) {
                pos = appendBytes(out, pos, " .shasum: ")?;
                pos = appendBytes(out, pos, _.dist.shasum)?;
                pos = appendChar(out, pos, '\n')?;
            }
            if (_.dist.integrity.len > 0) {
                pos = appendBytes(out, pos, " .integrity: ")?;
                pos = appendBytes(out, pos, _.dist.integrity)?;
                pos = appendChar(out, pos, '\n')?;
            }
            if (_.dist.unpacked_size > 0) {
                pos = appendBytes(out, pos, " .unpackedSize: ")?;
                pos = appendHumanSize(out, pos, _.dist.unpacked_size)?;
                pos = appendChar(out, pos, '\n')?;
            }
        }

        if (_.dist_tags.len > 0) {
            pos = appendBytes(out, pos, "\ndist-tags:\n")?;
            let i: usize = 0;
            while (i < _.dist_tags.len) {
                pos = appendBytes(out, pos, _.dist_tags[i].key)?;
                pos = appendBytes(out, pos, ": ")?;
                pos = appendBytes(out, pos, _.dist_tags[i].value)?;
                pos = appendChar(out, pos, '\n')?;
                i += 1;
            }
        }

        if (_.maintainers.len > 0) {
            pos = appendBytes(out, pos, "\nmaintainers:\n")?;
            let i: usize = 0;
            while (i < _.maintainers.len) {
                const m = _.maintainers[i];
                if (m.name.len == 0 && m.email.len == 0) {
                    i += 1;
                    continue;
                }
                pos = appendBytes(out, pos, "- ")?;
                pos = appendBytes(out, pos, m.name)?;
                if (m.email.len > 0) {
                    pos = appendBytes(out, pos, " <")?;
                    pos = appendBytes(out, pos, m.email)?;
                    pos = appendChar(out, pos, '>')?;
                }
                pos = appendChar(out, pos, '\n')?;
                i += 1;
            }
        }

        if (_.published_at.len > 0) {
            pos = appendBytes(out, pos, "\nPublished: ")?;
            pos = appendBytes(out, pos, _.published_at)?;
            pos = appendChar(out, pos, '\n')?;
        }
        return pos;
    }
}

/// 格式化最近可用版本提示（最多展示末尾 5 个版本，超出部分显示 `... and N more`）
pub fn formatRecentVersionsHint(versions: []const []const u8, out: []u8): usize! {
    if (versions.len == 0) {
        return 0;
    }
    const start_idx = if (versions.len > PM_VIEW_RECENT_VERSIONS_LIMIT)
        versions.len - PM_VIEW_RECENT_VERSIONS_LIMIT
    else
        0;

    let pos = appendBytes(out, 0, "Recent versions:\n")?;
    let i = start_idx;
    while (i < versions.len) {
        pos = appendBytes(out, pos, "- ")?;
        pos = appendBytes(out, pos, versions[i])?;
        pos = appendChar(out, pos, '\n')?;
        i += 1;
    }
    if (start_idx > 0) {
        pos = appendBytes(out, pos, "  ... and ")?;
        pos = appendU64(out, pos, start_idx as u64)?;
        pos = appendBytes(out, pos, " more\n")?;
    }
    return pos;
}

/// 格式化 `--json` 模式下的标准错误响应 JSON（自动执行 RFC 8259 字符串转义）
pub fn formatPmViewJsonError(
    message: []const u8,
    spec: []const u8,
    property: ([]const u8)?,
    out: []u8,
): usize! {
    let pos = appendBytes(out, 0, "{ \"error\": ")?;
    pos = appendJsonEscaped(out, pos, message)?;
    pos = appendBytes(out, pos, ", \"version\": ")?;
    pos = appendJsonEscaped(out, pos, spec)?;
    if (prop := property) {
        pos = appendBytes(out, pos, ", \"property\": ")?;
        pos = appendJsonEscaped(out, pos, prop)?;
    }
    return appendBytes(out, pos, " }\n");
}
