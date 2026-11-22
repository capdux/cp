// Source: Absorbed from /data/dev/bun/src/install/bin.cp | Synced: 2026-11-22T13:44:00Z
// Pure-CP zero-allocation `package.json` `bin` / `directories.bin` normalizer,
// `npm-normalize-package-bin` security sanitizer, and `node_modules/.bin` link planner.
// Models Bun's `Bin` & `Bin.Linker` invariants:
// - Normalizes scoped package names (`@scope/cli` -> `cli`) and strips path prefixes (`foo/bar:baz` -> `baz`)
// - Rejects path traversal (`..`), absolute paths, drive prefixes, and control characters in bin names & targets
// - Supports `none` / `file` / `named_file` / `dir` / `map` representations with order-independent `eql()` and `toJson()`
// - Deduplicates conflicting `.bin/<name>` entries (first-registered direct dependency wins over transitive collisions)

/// 单个包 `bin` 映射支持的最大可执行文件条目数
pub const BIN_LINK_MAX_ENTRIES: usize = 32;
/// 单次安装规划支持的最大 `.bin` 链接条目数
pub const BIN_LINK_MAX_PLANNED: usize = 64;

/// `bin` 规范化与链接规划错误集
pub err BinLinkError {
    EmptyName,
    EmptyTarget,
    UnsafeBinName,
    UnsafeTargetPath,
    InvalidIndex,
    CapacityExceeded,
    BufferTooSmall,
}

/// `package.json` `bin` 字段规范化存储形态（对齐 Bun `Bin.Tag`）
pub enum PkgBinKind: u8 {
    None = 0,
    File = 1,
    NamedFile = 2,
    Dir = 3,
    Map = 4,
}

/// 单条规范化后的 `(bin_name, target_rel)` 映射
pub struct PkgBinEntry {
    name: []const u8 = "",
    target: []const u8 = "",
}

/// 按 `npm-normalize-package-bin` 规范提取规范可执行文件名：
/// 截取最后一个 `/`、`\` 或 `:` 之后的名称（如 `@scope/my-cli` -> `my-cli`）。
pub fn normalizedBinName(raw: []const u8): []const u8 {
    let i = raw.len;
    while (i > 0) {
        i -= 1;
        const c = raw[i];
        if (c == '/' || c == '\\' || c == ':') {
            return raw[i + 1..];
        }
    }
    return raw;
}

/// 清洗并校验可执行文件名，拒绝空串、`.`、`..` 及 ASCII 控制字符
pub fn validateSafeBinName(raw: []const u8): ([]const u8)! {
    const cleaned = normalizedBinName(trimAscii(raw));
    if (cleaned.len == 0) return err.EmptyName;
    if (bytesEql(cleaned, ".") || bytesEql(cleaned, "..")) {
        return err.UnsafeBinName;
    }
    for (i of 0..cleaned.len) {
        const c = cleaned[i];
        if (c < 0x20 || c == 0x7f) {
            return err.UnsafeBinName;
        }
    }
    return cleaned;
}

/// 规范化并校验包内相对目标路径（剥离前导 `./`，拒绝绝对路径、Windows 盘符、`..` 穿越段及控制字符）
pub fn normalizeBinTargetRel(raw: []const u8): ([]const u8)! {
    let s = trimAscii(raw);
    if (s.len == 0) return err.EmptyTarget;

    // 剥离重复的前导 `./`
    while (s.len >= 2 && s[0] == '.' && (s[1] == '/' || s[1] == '\\')) {
        s = s[2..];
    }
    if (s.len == 0) return err.EmptyTarget;

    // 拒绝绝对路径或 Windows 盘符
    if (s[0] == '/' || s[0] == '\\') return err.UnsafeTargetPath;
    if (s.len >= 2 && s[1] == ':') return err.UnsafeTargetPath;

    // 逐段检查 `..` 穿越与控制字符
    let seg_start: usize = 0;
    for (i of 0..s.len) {
        const c = s[i];
        if (c < 0x20 || c == 0x7f || c == '\\') {
            return err.UnsafeTargetPath;
        }
        if (c == '/') {
            const seg = s[seg_start..i];
            if (bytesEql(seg, "..")) return err.UnsafeTargetPath;
            seg_start = i + 1;
        }
    }
    const last_seg = s[seg_start..];
    if (last_seg.len == 0 || bytesEql(last_seg, ".") || bytesEql(last_seg, "..")) {
        return err.UnsafeTargetPath;
    }
    return s;
}

/// 零堆分配 `package.json` `bin` / `directories.bin` 规范化描述符（对齐 Bun `Bin`）
pub struct PkgBinSpec {
    kind: PkgBinKind = .None,
    entries: [BIN_LINK_MAX_ENTRIES]PkgBinEntry = [PkgBinEntry{}; BIN_LINK_MAX_ENTRIES],
    count: usize = 0,
    dir_path: []const u8 = "",

    static fn init(): PkgBinSpec {
        return PkgBinSpec{};
    }

    /// 构造字符串形态 `"bin": "./cli.js"`（可执行名由包名经 `normalizedBinName` 推导）
    static fn fromSingleFile(pkg_name: []const u8, target_rel: []const u8): PkgBinSpec! {
        const bname = validateSafeBinName(pkg_name)?;
        const target = normalizeBinTargetRel(target_rel)?;
        let spec = PkgBinSpec{ kind: .File, count: 1 };
        spec.entries[0] = PkgBinEntry{ name: bname, target: target };
        return spec;
    }

    /// 构造目录形态 `"directories": { "bin": "./bin" }`
    static fn fromDir(dir_rel: []const u8): PkgBinSpec! {
        const dir = normalizeBinTargetRel(dir_rel)?;
        return PkgBinSpec{
            kind: .Dir,
            count: 0,
            dir_path: dir,
        };
    }

    /// 向对象形态 `"bin": { ... }` 添加条目（单条时为 `NamedFile`，多条时自动升级为 `Map`）
    fn addMapEntry(*_, bin_name: []const u8, target_rel: []const u8): usize! {
        const bname = validateSafeBinName(bin_name)?;
        const target = normalizeBinTargetRel(target_rel)?;

        // 同名去重覆盖
        for (i of 0.._.count) {
            if (bytesEql(_.entries[i].name, bname)) {
                _.entries[i].target = target;
                return _.count;
            }
        }
        if (_.count >= BIN_LINK_MAX_ENTRIES) return err.CapacityExceeded;
        _.entries[_.count] = PkgBinEntry{ name: bname, target: target };
        _.count += 1;
        _.kind = if (_.count == 1) .NamedFile else .Map;
        return _.count;
    }

    fn slice(): []const PkgBinEntry {
        return _.entries[0.._.count];
    }

    /// 语义等价比较（`Map` 形态忽略条目顺序，对齐 Bun `Bin.eql`）
    fn eql(other: PkgBinSpec): bool {
        if (_.kind != other.kind) return false;
        return switch (_.kind) {
            .None => true,
            .Dir => bytesEql(_.dir_path, other.dir_path),
            .File, .NamedFile => _.count == other.count &&
                _.count == 1 &&
                bytesEql(_.entries[0].name, other.entries[0].name) &&
                bytesEql(_.entries[0].target, other.entries[0].target),
            .Map => {
                if (_.count != other.count) return false;
                for (i of 0.._.count) {
                    let found = false;
                    for (j of 0..other.count) {
                        if (bytesEql(_.entries[i].name, other.entries[j].name)) {
                            if (!bytesEql(_.entries[i].target, other.entries[j].target)) {
                                return false;
                            }
                            found = true;
                            break;
                        }
                    }
                    if (!found) return false;
                }
                return true;
            },
        };
    }

    /// 序列化为单行 JSON 片段（对齐 Bun `Bin.toJson(.single_line)`）
    fn toJson(out: []u8): usize! {
        return switch (_.kind) {
            .None => copyInto(out, 0, "null"),
            .File => writeJsonString(out, 0, _.entries[0].target),
            .Dir => writeJsonString(out, 0, _.dir_path),
            .NamedFile, .Map => {
                let pos = copyInto(out, 0, "{ ")?;
                for (i of 0.._.count) {
                    if (i > 0) {
                        pos = copyInto(out, pos, ", ")?;
                    }
                    pos = writeJsonString(out, pos, _.entries[i].name)?;
                    pos = copyInto(out, pos, ": ")?;
                    pos = writeJsonString(out, pos, _.entries[i].target)?;
                }
                return copyInto(out, pos, " }");
            },
        };
    }
}

/// 单条待物化的 `.bin` 链接计划项
pub struct BinLinkPlanItem {
    bin_name: []const u8 = "",
    pkg_name: []const u8 = "",
    target_rel: []const u8 = "",
    global: bool = false,
}

/// 零堆分配 `node_modules/.bin` 符号链接规划器（对齐 Bun `Bin.Linker`）
pub struct BinLinkPlanner {
    items: [BIN_LINK_MAX_PLANNED]BinLinkPlanItem = [BinLinkPlanItem{}; BIN_LINK_MAX_PLANNED],
    count: usize = 0,
    skipped_conflicts: usize = 0,

    static fn init(): BinLinkPlanner {
        return BinLinkPlanner{};
    }

    /// 检查 `bin_name` 是否已在当前 `.bin` 目录规划过（防止传递依赖覆盖直接依赖的同名命令）
    fn hasBinName(bin_name: []const u8): bool {
        for (i of 0.._.count) {
            if (bytesEql(_.items[i].bin_name, bin_name)) return true;
        }
        return false;
    }

    /// 将指定包的 `PkgBinSpec` 加入链接计划，返回实际新增链接数（同名冲突自动跳过并计入 `skipped_conflicts`）
    fn planSpec(*_, pkg_name: []const u8, spec: PkgBinSpec, global: bool): usize! {
        if (trimAscii(pkg_name).len == 0) return err.EmptyName;
        let added: usize = 0;
        for (i of 0..spec.count) {
            const entry = spec.entries[i];
            if (_.hasBinName(entry.name)) {
                _.skipped_conflicts += 1;
                continue;
            }
            if (_.count >= BIN_LINK_MAX_PLANNED) return err.CapacityExceeded;
            _.items[_.count] = BinLinkPlanItem{
                bin_name: entry.name,
                pkg_name: pkg_name,
                target_rel: entry.target,
                global: global,
            };
            _.count += 1;
            added += 1;
        }
        return added;
    }

    fn slice(): []const BinLinkPlanItem {
        return _.items[0.._.count];
    }

    /// 格式化链接目标落地路径：
    /// - 本地安装：`<base_dir>/.bin/<bin_name>`
    /// - 全局安装：`<base_dir>/<bin_name>`
    fn formatDestPath(idx: usize, base_dir: []const u8, out: []u8): usize! {
        if (idx >= _.count) return err.InvalidIndex;
        const item = _.items[idx];
        let pos: usize = 0;
        if (base_dir.len > 0) {
            pos = copyInto(out, pos, base_dir)?;
            if (base_dir[base_dir.len - 1] != '/') {
                pos = copyInto(out, pos, "/")?;
            }
        }
        if (!item.global) {
            pos = copyInto(out, pos, ".bin/")?;
        }
        return copyInto(out, pos, item.bin_name);
    }

    /// 格式化 POSIX 相对符号链接源目标（如 `../@acme/cli/bin/cli.js`）
    fn formatRelativeSymlinkTarget(idx: usize, out: []u8): usize! {
        if (idx >= _.count) return err.InvalidIndex;
        const item = _.items[idx];
        let pos: usize = 0;
        if (!item.global) {
            pos = copyInto(out, pos, "../")?;
        }
        pos = copyInto(out, pos, item.pkg_name)?;
        pos = copyInto(out, pos, "/")?;
        return copyInto(out, pos, item.target_rel);
    }
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' || s[start] == '\r')) {
        start += 1;
    }
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\n' || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
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

fn writeJsonString(dst: []u8, offset: usize, s: []const u8): usize! {
    let pos = copyInto(dst, offset, "\"")?;
    pos = copyInto(dst, pos, s)?;
    return copyInto(dst, pos, "\"");
}
