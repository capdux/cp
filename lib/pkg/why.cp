// Source: Absorbed from /data/dev/bun/src/cli/why_command.cp | Synced: 2026-11-22T09:59:11Z
// Pure-CP zero-allocation package dependency reverse-explanation (`bun why`) engine:
// glob + version pattern matcher, specifier specificity & dependency-type sorter,
// cycle-safe dependency chain renderer (`--top` / `--depth`).

/// `WhyGraph` 支持的最大包节点数量
pub const WHY_MAX_PACKAGES: usize = 64;

/// `WhyGraph` 支持的最大依赖边数量
pub const WHY_MAX_EDGES: usize = 256;

/// `WhyGraph` 树形渲染支持的最大递归深度
pub const WHY_MAX_DEPTH: usize = 32;

const PREFIX_LAST: []const u8 = "  └─ ";
const PREFIX_MIDDLE: []const u8 = "  ├─ ";
const PREFIX_CONTINUE: []const u8 = "  │  ";
const PREFIX_SPACE: []const u8 = "     ";

/// `cap:pkg` `why` 依赖解释引擎错误集
pub err WhyError {
    EmptyPattern,
    InvalidPattern,
    PackageTableFull,
    EdgeTableFull,
    InvalidPackageId,
    NoMatchingPackage,
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

fn startsWith(haystack: []const u8, prefix: []const u8): bool {
    if (prefix.len > haystack.len) {
        return false;
    }
    return bytesEql(haystack[0..prefix.len], prefix);
}

fn endsWith(haystack: []const u8, suffix: []const u8): bool {
    if (suffix.len > haystack.len) {
        return false;
    }
    return bytesEql(haystack[haystack.len - suffix.len..], suffix);
}

fn indexOfCharFrom(s: []const u8, start: usize, ch: u8): usize? {
    let i = start;
    while (i < s.len) {
        if (s[i] == ch) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn indexOfSubstr(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) {
        return 0;
    }
    if (needle.len > haystack.len) {
        return null;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (bytesEql(haystack[i..i + needle.len], needle)) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn bytesLessThan(a: []const u8, b: []const u8): bool {
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

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
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

/// 依赖类型枚举（对齐 Bun `why_command.cp` 的 `DependencyType`）
pub enum WhyDepType: u8 {
    Dev = 0,
    Optional = 1,
    OptionalPeer = 2,
    Peer = 3,
    Prod = 4,

    /// 返回依赖类型排序优先级（`Prod(4) > Peer(3) > OptionalPeer(2) > Optional(1) > Dev(0)`）
    fn priority(): u8 {
        return switch (_) {
            .Prod => 4,
            .Peer => 3,
            .OptionalPeer => 2,
            .Optional => 1,
            .Dev => 0,
        };
    }

    /// 返回渲染树节点时的前缀标签
    fn labelPrefix(): []const u8 {
        return switch (_) {
            .Prod => "",
            .Dev => "dev ",
            .Peer => "peer ",
            .Optional => "optional ",
            .OptionalPeer => "optional peer ",
        };
    }
}

/// 计算依赖版本说明符的精确度分值（严格对齐 Bun `why_command.cp` `getSpecifierSpecificity`）
///
/// 排序权重：`精确版本(10) > workspace/空(9) > ^(8) > ~(7) > <>=(6) > .x(5) > 其他(3) > *(1)`
pub fn specifierSpecificity(spec: []const u8): u8 {
    if (spec.len == 0) {
        return 9;
    }
    if (spec[0] == '*') {
        return 1;
    }
    if (indexOfSubstr(spec, ".x") != null) {
        return 5;
    }
    let i: usize = 0;
    while (i < spec.len) {
        if (spec[i] == '<' || spec[i] == '>' || spec[i] == '=') {
            return 6;
        }
        i += 1;
    }
    if (spec[0] == '~') {
        return 7;
    }
    if (spec[0] == '^') {
        return 8;
    }
    if (indexOfSubstr(spec, "workspace:") != null) {
        return 9;
    }
    if (isAsciiDigit(spec[0])) {
        return 10;
    }
    return 3;
}

/// 简单三段式 Semver 解析（供 `WhyGlobPattern.matchesVersion` 做 `^`/`~`/`>=` 区间匹配）
struct SimpleSemver {
    major: u32 = 0,
    minor: u32 = 0,
    patch: u32 = 0,
    has_minor: bool = false,
    has_patch: bool = false,
}

fn parseSimpleSemver(raw: []const u8): SimpleSemver? {
    if (raw.len == 0) {
        return null;
    }
    let i: usize = 0;
    if (raw[0] == 'v' || raw[0] == 'V') {
        i = 1;
    }
    if (i >= raw.len || !isAsciiDigit(raw[i])) {
        return null;
    }
    let res = SimpleSemver{};
    let major: u32 = 0;
    while (i < raw.len && isAsciiDigit(raw[i])) {
        major = major * 10 + ((raw[i] - '0') as u32);
        i += 1;
    }
    res.major = major;

    if (i < raw.len && raw[i] == '.') {
        i += 1;
        if (i >= raw.len || !isAsciiDigit(raw[i])) {
            return null;
        }
        let minor: u32 = 0;
        while (i < raw.len && isAsciiDigit(raw[i])) {
            minor = minor * 10 + ((raw[i] - '0') as u32);
            i += 1;
        }
        res.minor = minor;
        res.has_minor = true;
    }

    if (i < raw.len && raw[i] == '.') {
        i += 1;
        if (i >= raw.len || !isAsciiDigit(raw[i])) {
            return null;
        }
        let patch: u32 = 0;
        while (i < raw.len && isAsciiDigit(raw[i])) {
            patch = patch * 10 + ((raw[i] - '0') as u32);
            i += 1;
        }
        res.patch = patch;
        res.has_patch = true;
    }
    return res;
}

fn cmpSemver(a: SimpleSemver, b: SimpleSemver): i8 {
    if (a.major != b.major) {
        return if (a.major < b.major) -1 else 1;
    }
    if (a.minor != b.minor) {
        return if (a.minor < b.minor) -1 else 1;
    }
    if (a.patch != b.patch) {
        return if (a.patch < b.patch) -1 else 1;
    }
    return 0;
}

/// `bun why` 通配符模式分类
pub enum WhyGlobKind: u8 {
    Exact = 0,
    Prefix = 1,
    Suffix = 2,
    Middle = 3,
    Contains = 4,
    Invalid = 5,
}

/// `bun why` 包名与版本查询模式（支持 `react`、`@types/*`、`*-lodash`、`*react*`、`@org/pkg@^18.0.0`）
pub struct WhyGlobPattern {
    kind: WhyGlobKind = .Exact,
    name_pattern: []const u8 = "",
    prefix: []const u8 = "",
    suffix: []const u8 = "",
    substring: []const u8 = "",
    version_pattern: []const u8 = "",

    /// 解析查询模式；支持带 `@version` 后缀（正确保留作用域包开头的 `@scope/`）
    static fn parse(pattern: []const u8): WhyGlobPattern! {
        if (pattern.len == 0) {
            return err.EmptyPattern;
        }

        let pkg_part = pattern;
        let ver_part: []const u8 = "";

        // 从索引 1 起查找 `@`，防止把 `@types/*` 的作用域前缀误判为版本分隔符
        if (at_pos := indexOfCharFrom(pattern, 1, '@')) {
            if (at_pos < pattern.len - 1) {
                pkg_part = pattern[0..at_pos];
                ver_part = pattern[at_pos + 1..];
            }
        }

        let parsed = WhyGlobPattern.parseNameOnly(pkg_part);
        parsed.name_pattern = pkg_part;
        parsed.version_pattern = ver_part;
        if (parsed.kind == .Invalid) {
            return err.InvalidPattern;
        }
        return parsed;
    }

    static fn parseNameOnly(pattern: []const u8): WhyGlobPattern {
        const first_star = indexOfCharFrom(pattern, 0, '*') ?? return WhyGlobPattern{
            kind: .Exact,
            name_pattern: pattern,
        };

        // `*sub*` -> Contains（中间不含额外 `*`）
        if (pattern.len >= 3 && pattern[0] == '*' && pattern[pattern.len - 1] == '*') {
            const sub = pattern[1..pattern.len - 1];
            if (sub.len > 0 && indexOfCharFrom(sub, 0, '*') == null) {
                return WhyGlobPattern{
                    kind: .Contains,
                    name_pattern: pattern,
                    substring: sub,
                };
            }
            return WhyGlobPattern{ kind: .Invalid, name_pattern: pattern };
        }

        // 检查是否存在多个 `*`
        if (indexOfCharFrom(pattern, first_star + 1, '*') != null) {
            return WhyGlobPattern{ kind: .Invalid, name_pattern: pattern };
        }

        if (first_star == pattern.len - 1) {
            return WhyGlobPattern{
                kind: .Prefix,
                name_pattern: pattern,
                prefix: pattern[0..first_star],
            };
        }
        if (first_star == 0) {
            return WhyGlobPattern{
                kind: .Suffix,
                name_pattern: pattern,
                suffix: pattern[1..],
            };
        }
        return WhyGlobPattern{
            kind: .Middle,
            name_pattern: pattern,
            prefix: pattern[0..first_star],
            suffix: pattern[first_star + 1..],
        };
    }

    fn matchesName(name: []const u8): bool {
        return switch (_.kind) {
            .Exact => bytesEql(name, _.name_pattern),
            .Prefix => startsWith(name, _.prefix),
            .Suffix => endsWith(name, _.suffix),
            .Middle => name.len >= _.prefix.len + _.suffix.len &&
                startsWith(name, _.prefix) &&
                endsWith(name, _.suffix),
            .Contains => indexOfSubstr(name, _.substring) != null,
            .Invalid => false,
        };
    }

    fn matchesVersion(version: []const u8): bool {
        if (_.version_pattern.len == 0 || bytesEql(_.version_pattern, "latest") || bytesEql(_.version_pattern, "*")) {
            return true;
        }
        if (bytesEql(version, _.version_pattern)) {
            return true;
        }

        // 支持常见 Semver 区间前缀：`^`、`~`、`>=`
        if (_.version_pattern[0] == '^' && _.version_pattern.len > 1) {
            if (want := parseSimpleSemver(_.version_pattern[1..])) {
                if (got := parseSimpleSemver(version)) {
                    if (cmpSemver(got, want) < 0) {
                        return false;
                    }
                    if (want.major > 0) {
                        return got.major == want.major;
                    }
                    if (want.minor > 0) {
                        return got.major == 0 && got.minor == want.minor;
                    }
                    return got.major == 0 && got.minor == 0 && got.patch == want.patch;
                }
            }
        } else if (_.version_pattern[0] == '~' && _.version_pattern.len > 1) {
            if (want := parseSimpleSemver(_.version_pattern[1..])) {
                if (got := parseSimpleSemver(version)) {
                    if (cmpSemver(got, want) < 0) {
                        return false;
                    }
                    return got.major == want.major && got.minor == want.minor;
                }
            }
        } else if (startsWith(_.version_pattern, ">=") && _.version_pattern.len > 2) {
            if (want := parseSimpleSemver(_.version_pattern[2..])) {
                if (got := parseSimpleSemver(version)) {
                    return cmpSemver(got, want) >= 0;
                }
            }
        }

        return startsWith(version, _.version_pattern);
    }

    fn matches(name: []const u8, version: []const u8): bool {
        return _.matchesName(name) && _.matchesVersion(version);
    }
}

/// 图中的单个包节点
pub struct WhyPackage {
    name: []const u8 = "",
    version: []const u8 = "",
    is_workspace: bool = false,
}

/// 图中的单条有向依赖边（`from_id` 依赖 `to_id`）
struct WhyEdge {
    from_id: usize = 0,
    to_id: usize = 0,
    spec: []const u8 = "",
    dep_type: WhyDepType = .Prod,
}

/// 反向依赖方信息（对齐 Bun `DependentInfo`）
pub struct WhyDependent {
    name: []const u8 = "",
    version: []const u8 = "",
    spec: []const u8 = "",
    dep_type: WhyDepType = .Prod,
    pkg_id: usize = 0,
    is_workspace: bool = false,
}

/// 排序比较器：精确度降序 -> 依赖类型优先级降序 -> 包名字典序升序
pub fn compareWhyDependents(a: WhyDependent, b: WhyDependent): bool {
    const a_spec = specifierSpecificity(a.spec);
    const b_spec = specifierSpecificity(b.spec);
    if (a_spec != b_spec) {
        return a_spec > b_spec;
    }
    const a_prio = a.dep_type.priority();
    const b_prio = b.dep_type.priority();
    if (a_prio != b_prio) {
        return a_prio > b_prio;
    }
    return bytesLessThan(a.name, b.name);
}

/// `WhyGraph.renderExplain` 渲染选项（对齐 `bun why --top --depth <N>`）
pub struct WhyOptions {
    top_only: bool = false,
    max_depth: usize = WHY_MAX_DEPTH,

    fn effectiveDepth(): usize {
        if (_.top_only) {
            return 1;
        }
        if (_.max_depth > WHY_MAX_DEPTH) {
            return WHY_MAX_DEPTH;
        }
        return _.max_depth;
    }
}

/// 零堆分配包依赖反向追踪与 `bun why` 树形渲染引擎
pub struct WhyGraph {
    packages: [WHY_MAX_PACKAGES]WhyPackage = [WhyPackage{}; WHY_MAX_PACKAGES],
    pkg_len: usize = 0,
    edges: [WHY_MAX_EDGES]WhyEdge = [WhyEdge{}; WHY_MAX_EDGES],
    edge_len: usize = 0,

    static fn init(): WhyGraph {
        return WhyGraph{};
    }

    fn packageCount(): usize {
        return _.pkg_len;
    }

    fn edgeCount(): usize {
        return _.edge_len;
    }

    /// 添加包节点并返回其稳定索引 `pkg_id`
    fn addPackage(*_, name: []const u8, version: []const u8, is_workspace: bool): usize! {
        if (_.pkg_len >= WHY_MAX_PACKAGES) {
            return err.PackageTableFull;
        }
        const id = _.pkg_len;
        _.packages[id] = WhyPackage{
            name: name,
            version: version,
            is_workspace: is_workspace,
        };
        _.pkg_len += 1;
        return id;
    }

    /// 添加有向依赖边：`from_id` 依赖 `to_id`
    fn addDependency(*_, from_id: usize, to_id: usize, spec: []const u8, dep_type: WhyDepType)! {
        if (from_id >= _.pkg_len || to_id >= _.pkg_len) {
            return err.InvalidPackageId;
        }
        if (_.edge_len >= WHY_MAX_EDGES) {
            return err.EdgeTableFull;
        }
        _.edges[_.edge_len] = WhyEdge{
            from_id: from_id,
            to_id: to_id,
            spec: spec,
            dep_type: dep_type,
        };
        _.edge_len += 1;
    }

    /// 查找所有匹配 `WhyGlobPattern` 的包索引，返回命中数
    fn findMatchingPackages(pattern: WhyGlobPattern, out_ids: []usize): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.pkg_len) {
            const pkg = _.packages[i];
            if (pkg.name.len > 0 && pattern.matches(pkg.name, pkg.version)) {
                if (count < out_ids.len) {
                    out_ids[count] = i;
                    count += 1;
                }
            }
            i += 1;
        }
        return count;
    }

    /// 收集并按 `compareWhyDependents` 确定性排序 `target_id` 的所有直接依赖方
    fn collectDependents(target_id: usize, out: []WhyDependent): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.edge_len) {
            const e = _.edges[i];
            if (e.to_id == target_id) {
                if (count < out.len) {
                    const p = _.packages[e.from_id];
                    out[count] = WhyDependent{
                        name: p.name,
                        version: p.version,
                        spec: e.spec,
                        dep_type: e.dep_type,
                        pkg_id: e.from_id,
                        is_workspace: p.is_workspace,
                    };
                    count += 1;
                }
            }
            i += 1;
        }

        // 插入排序保证小规模确定性稳定排序
        let k: usize = 1;
        while (k < count) {
            const key = out[k];
            let j = k;
            while (j > 0 && compareWhyDependents(key, out[j - 1])) {
                out[j] = out[j - 1];
                j -= 1;
            }
            out[j] = key;
            k += 1;
        }
        return count;
    }

    /// 递归渲染某节点的上游依赖链
    fn renderSubtree(
        target_id: usize,
        depth: usize,
        max_depth: usize,
        indent_flags: *[WHY_MAX_DEPTH]bool,
        path_visited: *[WHY_MAX_PACKAGES]bool,
        out: []u8,
        start_pos: usize,
    ): usize! {
        let pos = start_pos;
        let deps: [WHY_MAX_PACKAGES]WhyDependent = [WhyDependent{}; WHY_MAX_PACKAGES];
        const dep_count = _.collectDependents(target_id, deps[0..]);
        if (dep_count == 0) {
            return pos;
        }

        if (depth >= max_depth) {
            pos = writeIndent(indent_flags, depth, out, pos)?;
            pos = appendBytes(out, pos, PREFIX_LAST)?;
            pos = appendBytes(out, pos, "(deeper dependencies hidden)\n")?;
            return pos;
        }

        let i: usize = 0;
        while (i < dep_count) {
            const dep = deps[i];
            const is_last = (i + 1 == dep_count);
            pos = writeIndent(indent_flags, depth, out, pos)?;
            pos = appendBytes(out, pos, if (is_last) PREFIX_LAST else PREFIX_MIDDLE)?;
            pos = writeDependentLine(dep, out, pos)?;

            if (path_visited[dep.pkg_id]) {
                pos = appendBytes(out, pos, " (circular)\n")?;
            } else {
                pos = appendBytes(out, pos, "\n")?;
                if (depth + 1 < WHY_MAX_DEPTH) {
                    indent_flags[depth] = !is_last;
                    path_visited[dep.pkg_id] = true;
                    pos = _.renderSubtree(
                        dep.pkg_id,
                        depth + 1,
                        max_depth,
                        indent_flags,
                        path_visited,
                        out,
                        pos,
                    )?;
                    path_visited[dep.pkg_id] = false;
                }
            }
            i += 1;
        }
        return pos;
    }

    /// 渲染 `bun why <pattern>` 完整输出文本至 `out`
    fn renderExplain(pattern_str: []const u8, opts: WhyOptions, out: []u8): usize! {
        const pat = WhyGlobPattern.parse(pattern_str)?;
        let matches: [WHY_MAX_PACKAGES]usize = [0; WHY_MAX_PACKAGES];
        const match_count = _.findMatchingPackages(pat, matches[0..]);
        if (match_count == 0) {
            return err.NoMatchingPackage;
        }

        const max_depth = opts.effectiveDepth();
        let pos: usize = 0;
        let m: usize = 0;
        while (m < match_count) {
            if (m > 0) {
                pos = appendBytes(out, pos, "\n")?;
            }
            const target_id = matches[m];
            const pkg = _.packages[target_id];
            pos = appendBytes(out, pos, pkg.name)?;
            if (pkg.version.len > 0) {
                pos = appendBytes(out, pos, "@")?;
                pos = appendBytes(out, pos, pkg.version)?;
            }
            pos = appendBytes(out, pos, "\n")?;

            let deps: [WHY_MAX_PACKAGES]WhyDependent = [WhyDependent{}; WHY_MAX_PACKAGES];
            const dep_count = _.collectDependents(target_id, deps[0..]);
            if (dep_count == 0) {
                pos = appendBytes(out, pos, PREFIX_LAST)?;
                pos = appendBytes(out, pos, "No dependents found\n")?;
            } else if (max_depth == 0) {
                pos = appendBytes(out, pos, PREFIX_LAST)?;
                pos = appendBytes(out, pos, "(deeper dependencies hidden)\n")?;
            } else {
                let indent_flags: [WHY_MAX_DEPTH]bool = [false; WHY_MAX_DEPTH];
                let path_visited: [WHY_MAX_PACKAGES]bool = [false; WHY_MAX_PACKAGES];
                path_visited[target_id] = true;
                pos = _.renderSubtree(
                    target_id,
                    0,
                    max_depth,
                    &indent_flags,
                    &path_visited,
                    out,
                    pos,
                )?;
            }
            m += 1;
        }
        return pos;
    }
}

fn writeIndent(indent_flags: *[WHY_MAX_DEPTH]bool, depth: usize, out: []u8, start_pos: usize): usize! {
    let pos = start_pos;
    let d: usize = 0;
    while (d < depth) {
        pos = appendBytes(out, pos, if (indent_flags[d]) PREFIX_CONTINUE else PREFIX_SPACE)?;
        d += 1;
    }
    return pos;
}

fn writeDependentLine(dep: WhyDependent, out: []u8, start_pos: usize): usize! {
    let pos = start_pos;
    const prefix = dep.dep_type.labelPrefix();
    if (prefix.len > 0) {
        pos = appendBytes(out, pos, prefix)?;
    }
    pos = appendBytes(out, pos, dep.name)?;
    if (!dep.is_workspace && dep.version.len > 0) {
        pos = appendBytes(out, pos, "@")?;
        pos = appendBytes(out, pos, dep.version)?;
    }
    if (dep.spec.len > 0) {
        pos = appendBytes(out, pos, " (requires ")?;
        pos = appendBytes(out, pos, dep.spec)?;
        pos = appendBytes(out, pos, ")")?;
    }
    return pos;
}
