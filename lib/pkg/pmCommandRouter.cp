// Source: Absorbed from /data/dev/bun/src/cli/package_manager_command.cp | Synced: 2026-11-22T12:42:30Z
// ============================================================================
// lib/pkg/pmCommandRouter.cp — Zero-Alloc `cap pm` Router, Tree & Cache Auditor
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven operations engine for `cap pm`:
//   1. Full `cap pm` subcommand parser & help formatter (`parsePmSubcommandArgs`,
//      `formatPmCommandHelp`) covering `bin`, `ls`/`list` (`--all`), `cache`,
//      `cache rm`, `hash`, `hash-string`, `hash-tokens`, `migrate`, `untrusted`,
//      `default-trusted`, `trust` (`--all`), `version`, `pack`, `pkg`, `whoami`,
//      `outdated`, `audit`, and `why`;
//   2. ASCII/Unicode dependency tree renderer (`renderPmLsTree`) with direct
//      vs `--all` nested views, cycle `(circular)` detection, deduplication
//      `(deduped)` tagging, and disk-size summary;
//   3. Global cache directory auditor & LRU/TTL prune planner (`planCachePrune`)
//      protecting lockfile-referenced packages while reclaiming expired or
//      over-quota entries;
//   4. Deterministic lockfile hash (`computePmLockfileHash`) and `cap pm whoami`
//      credential resolution diagnostics (`diagnosePmWhoami`).

/// Maximum nesting depth rendered by `renderPmLsTree`.
pub const PM_ROUTER_MAX_TREE_DEPTH: usize = 32;

/// `cap pm` subcommand parsing, tree rendering, and cache auditing error set.
pub err PmRouterError {
    UnknownSubcommand,
    UnknownFlag,
    MissingFlagValue,
    MissingRequiredArg,
    InvalidGzipLevel,
    SliceCapacityExceeded,
    BufferTooSmall,
}

// ============================================================================
// Internal Zero-Alloc Byte & Formatting Helpers
// ============================================================================

fn pmrBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn pmrStartsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    return pmrBytesEql(s[0..prefix.len], prefix);
}

fn pmrIndexOf(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) return i;
        i += 1;
    }
    return null;
}

fn pmrAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn pmrAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn pmrAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        pmrAppendByte(out, pos, '0')?;
        return;
    }
    while (v > 0) {
        const d: u8 = @as(u8, @intCast(v % 10));
        tmp[n] = @as(u8, '0') + d;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        pmrAppendByte(out, pos, tmp[n])?;
    }
}

fn pmrAppendHex64(out: []u8, pos: *usize, val: u64)! {
    const hex = "0123456789abcdef";
    let shift: u6 = 60;
    while (true) {
        const nib: usize = @as(usize, @intCast((val >> shift) & 0xF));
        pmrAppendByte(out, pos, hex[nib])?;
        if (shift == 0) break;
        shift -= 4;
    }
}

fn pmrAppendHumanBytes(out: []u8, pos: *usize, bytes: u64)! {
    if (bytes < 1024) {
        pmrAppendU64(out, pos, bytes)?;
        pmrAppendBytes(out, pos, " B")?;
        return;
    }
    if (bytes < 1024 * 1024) {
        const kb = bytes / 1024;
        const frac = ((bytes % 1024) * 10) / 1024;
        pmrAppendU64(out, pos, kb)?;
        pmrAppendByte(out, pos, '.')?;
        pmrAppendU64(out, pos, frac)?;
        pmrAppendBytes(out, pos, " KB")?;
        return;
    }
    const mb = bytes / (1024 * 1024);
    const frac = ((bytes % (1024 * 1024)) * 10) / (1024 * 1024);
    pmrAppendU64(out, pos, mb)?;
    pmrAppendByte(out, pos, '.')?;
    pmrAppendU64(out, pos, frac)?;
    pmrAppendBytes(out, pos, " MB")?;
}

// ============================================================================
// Section 1: `cap pm` Subcommand Parser & Help Renderer
// ============================================================================

/// Complete set of `cap pm` operational subcommands.
pub enum PmSubcommandKind: u8 {
    Help = 0,
    Bin = 1,
    Ls = 2,
    Cache = 3,
    CacheRm = 4,
    Hash = 5,
    HashString = 6,
    HashTokens = 7,
    Migrate = 8,
    Untrusted = 9,
    DefaultTrusted = 10,
    Trust = 11,
    Version = 12,
    Pack = 13,
    Pkg = 14,
    Whoami = 15,
    Outdated = 16,
    Audit = 17,
    Why = 18,

    fn asName(): []const u8 {
        return switch (_) {
            .Help => "help",
            .Bin => "bin",
            .Ls => "ls",
            .Cache => "cache",
            .CacheRm => "cache rm",
            .Hash => "hash",
            .HashString => "hash-string",
            .HashTokens => "hash-tokens",
            .Migrate => "migrate",
            .Untrusted => "untrusted",
            .DefaultTrusted => "default-trusted",
            .Trust => "trust",
            .Version => "version",
            .Pack => "pack",
            .Pkg => "pkg",
            .Whoami => "whoami",
            .Outdated => "outdated",
            .Audit => "audit",
            .Why => "why",
        };
    }

    fn summary(): []const u8 {
        return switch (_) {
            .Help => "print package manager utility help",
            .Bin => "print the path to the bin folder (-g for global)",
            .Ls => "list the dependency tree from the current lockfile (--all for full tree)",
            .Cache => "print the path to the global cache folder",
            .CacheRm => "clear or prune the global package cache",
            .Hash => "generate and print the deterministic lockfile hash",
            .HashString => "print the canonical string used to hash the lockfile",
            .HashTokens => "print the stored lockfile hash token",
            .Migrate => "migrate an existing lockfile to cap.lock without installing",
            .Untrusted => "list untrusted dependencies with blocked lifecycle scripts",
            .DefaultTrusted => "print the default trusted dependencies allowlist",
            .Trust => "trust dependencies and run their lifecycle scripts (--all)",
            .Version => "bump package version and optionally create a git tag",
            .Pack => "create a publishable tarball of the current workspace",
            .Pkg => "get, set, delete, or fix fields in cap.json / package.json",
            .Whoami => "print the authenticated registry username and credential status",
            .Outdated => "display outdated packages compared to registry latest",
            .Audit => "scan lockfile packages for known security advisories",
            .Why => "explain why a package is installed in the dependency graph",
        };
    }

    static fn fromName(raw: []const u8): PmSubcommandKind? {
        if (pmrBytesEql(raw, "help")) return PmSubcommandKind.Help;
        if (pmrBytesEql(raw, "bin")) return PmSubcommandKind.Bin;
        if (pmrBytesEql(raw, "ls") || pmrBytesEql(raw, "list")) return PmSubcommandKind.Ls;
        if (pmrBytesEql(raw, "cache")) return PmSubcommandKind.Cache;
        if (pmrBytesEql(raw, "hash")) return PmSubcommandKind.Hash;
        if (pmrBytesEql(raw, "hash-string")) return PmSubcommandKind.HashString;
        if (pmrBytesEql(raw, "hash-tokens") || pmrBytesEql(raw, "hash-print")) return PmSubcommandKind.HashTokens;
        if (pmrBytesEql(raw, "migrate")) return PmSubcommandKind.Migrate;
        if (pmrBytesEql(raw, "untrusted")) return PmSubcommandKind.Untrusted;
        if (pmrBytesEql(raw, "default-trusted")) return PmSubcommandKind.DefaultTrusted;
        if (pmrBytesEql(raw, "trust")) return PmSubcommandKind.Trust;
        if (pmrBytesEql(raw, "version")) return PmSubcommandKind.Version;
        if (pmrBytesEql(raw, "pack")) return PmSubcommandKind.Pack;
        if (pmrBytesEql(raw, "pkg")) return PmSubcommandKind.Pkg;
        if (pmrBytesEql(raw, "whoami")) return PmSubcommandKind.Whoami;
        if (pmrBytesEql(raw, "outdated")) return PmSubcommandKind.Outdated;
        if (pmrBytesEql(raw, "audit") || pmrBytesEql(raw, "scan")) return PmSubcommandKind.Audit;
        if (pmrBytesEql(raw, "why")) return PmSubcommandKind.Why;
        return null;
    }
}

/// Parsed `cap pm` subcommand invocation with typed flags and positional arguments.
pub struct PmParsedCommand {
    kind: PmSubcommandKind = PmSubcommandKind.Help,
    show_help: bool = false,
    global: bool = false,
    all: bool = false,
    dry_run: bool = false,
    quiet: bool = false,
    json_output: bool = false,
    force: bool = false,
    ignore_scripts: bool = false,
    recursive: bool = false,
    top_only: bool = false,
    no_git_tag_version: bool = false,
    gzip_level: u8 = 9,
    destination: []const u8 = "",
    filename: []const u8 = "",
    registry_url: []const u8 = "",
    audit_level: []const u8 = "",
    message: []const u8 = "",
    positionals: []const []const u8 = [],
}

fn pmrConsumeVal(args: []const []const u8, idx: *usize, inline_val: []const u8, has_eq: bool): ([]const u8)! {
    if (has_eq) {
        if (inline_val.len == 0) return err.MissingFlagValue;
        return inline_val;
    }
    if (idx.* + 1 >= args.len) return err.MissingFlagValue;
    idx.* += 1;
    return args[idx.*];
}

/// Parses `cap pm` CLI arguments (with or without leading `"pm"`) into `PmParsedCommand`.
pub fn parsePmSubcommandArgs(
    args: []const []const u8,
    out_positionals: [][]const u8,
): PmParsedCommand! {
    let start: usize = 0;
    if (start < args.len && pmrBytesEql(args[start], "pm")) {
        start += 1;
    }

    if (start >= args.len) {
        return PmParsedCommand{ kind: PmSubcommandKind.Help, show_help: true };
    }

    if (pmrBytesEql(args[start], "-h") || pmrBytesEql(args[start], "--help")) {
        return PmParsedCommand{ kind: PmSubcommandKind.Help, show_help: true };
    }

    let kind = PmSubcommandKind.fromName(args[start]) ?? return err.UnknownSubcommand;
    start += 1;

    // Two-word `cache rm` detection
    if (kind == PmSubcommandKind.Cache && start < args.len && pmrBytesEql(args[start], "rm")) {
        kind = PmSubcommandKind.CacheRm;
        start += 1;
    }

    let cmd = PmParsedCommand{ kind: kind };
    let pos_count: usize = 0;
    let i = start;

    while (i < args.len) {
        const arg = args[i];
        if (arg.len == 0 || arg[0] != '-') {
            if (pos_count >= out_positionals.len) return err.SliceCapacityExceeded;
            out_positionals[pos_count] = arg;
            pos_count += 1;
            i += 1;
            continue;
        }

        let flag = arg;
        let inline_val: []const u8 = "";
        let has_eq = false;
        if (eq := pmrIndexOf(arg, '=')) {
            flag = arg[0..eq];
            inline_val = arg[eq + 1..arg.len];
            has_eq = true;
        }

        if (pmrBytesEql(flag, "-h") || pmrBytesEql(flag, "--help")) {
            cmd.show_help = true;
        } else if (pmrBytesEql(flag, "-g") || pmrBytesEql(flag, "--global")) {
            cmd.global = true;
        } else if (pmrBytesEql(flag, "--all") || pmrBytesEql(flag, "-a")) {
            cmd.all = true;
        } else if (pmrBytesEql(flag, "--dry-run")) {
            cmd.dry_run = true;
        } else if (pmrBytesEql(flag, "--quiet") || pmrBytesEql(flag, "-q")) {
            cmd.quiet = true;
        } else if (pmrBytesEql(flag, "--json")) {
            cmd.json_output = true;
        } else if (pmrBytesEql(flag, "--force") || pmrBytesEql(flag, "-f")) {
            cmd.force = true;
        } else if (pmrBytesEql(flag, "--ignore-scripts")) {
            cmd.ignore_scripts = true;
        } else if (pmrBytesEql(flag, "--recursive") || pmrBytesEql(flag, "-r")) {
            cmd.recursive = true;
        } else if (pmrBytesEql(flag, "--top")) {
            cmd.top_only = true;
        } else if (pmrBytesEql(flag, "--no-git-tag-version")) {
            cmd.no_git_tag_version = true;
        } else if (pmrBytesEql(flag, "--destination")) {
            cmd.destination = pmrConsumeVal(args, &i, inline_val, has_eq)?;
        } else if (pmrBytesEql(flag, "--filename")) {
            cmd.filename = pmrConsumeVal(args, &i, inline_val, has_eq)?;
        } else if (pmrBytesEql(flag, "--registry")) {
            cmd.registry_url = pmrConsumeVal(args, &i, inline_val, has_eq)?;
        } else if (pmrBytesEql(flag, "--audit-level")) {
            cmd.audit_level = pmrConsumeVal(args, &i, inline_val, has_eq)?;
        } else if (pmrBytesEql(flag, "--message") || pmrBytesEql(flag, "-m")) {
            cmd.message = pmrConsumeVal(args, &i, inline_val, has_eq)?;
        } else if (pmrBytesEql(flag, "--gzip-level")) {
            const raw_lvl = pmrConsumeVal(args, &i, inline_val, has_eq)?;
            if (raw_lvl.len != 1 || raw_lvl[0] < '0' || raw_lvl[0] > '9') {
                return err.InvalidGzipLevel;
            }
            cmd.gzip_level = raw_lvl[0] - @as(u8, '0');
        } else {
            return err.UnknownFlag;
        }

        i += 1;
    }

    cmd.positionals = out_positionals[0..pos_count];
    return cmd;
}

/// Renders help text for `cap pm` or a specific subcommand into caller-owned `out`.
pub fn formatPmCommandHelp(kind: PmSubcommandKind, out: []u8): usize! {
    let pos: usize = 0;
    if (kind == PmSubcommandKind.Help) {
        pmrAppendBytes(out, &pos, "Usage: cap pm <command> [flags]\n\nCommands:\n")?;
        pmrAppendBytes(out, &pos, "  bin [-g]           print path to local or global .bin folder\n")?;
        pmrAppendBytes(out, &pos, "  ls [--all]         list dependency tree from cap.lock\n")?;
        pmrAppendBytes(out, &pos, "  cache [rm]         print or prune global package cache\n")?;
        pmrAppendBytes(out, &pos, "  hash               print deterministic lockfile digest\n")?;
        pmrAppendBytes(out, &pos, "  untrusted / trust  inspect or trust lifecycle scripts\n")?;
        pmrAppendBytes(out, &pos, "  pack / version     package tarball or bump version\n")?;
        pmrAppendBytes(out, &pos, "  whoami / audit     inspect registry auth or advisories\n")?;
        return pos;
    }

    pmrAppendBytes(out, &pos, "Usage: cap pm ")?;
    pmrAppendBytes(out, &pos, kind.asName())?;
    pmrAppendBytes(out, &pos, " [flags]\n  ")?;
    pmrAppendBytes(out, &pos, kind.summary())?;
    pmrAppendByte(out, &pos, '\n')?;
    return pos;
}

// ============================================================================
// Section 2: ASCII/Unicode Dependency Tree Renderer (`renderPmLsTree`)
// ============================================================================

/// Node in a dependency tree passed to `renderPmLsTree`.
pub struct PmLsNode {
    id: u32 = 0,
    name: []const u8 = "",
    version: []const u8 = "",
    byte_size: u64 = 0,
    is_direct: bool = false,
    is_dev: bool = false,
    is_optional: bool = false,

    static fn empty(): PmLsNode {
        return PmLsNode{};
    }
}

/// Directed parent -> child dependency edge for `renderPmLsTree`.
pub struct PmLsEdge {
    parent_idx: u32 = 0,
    child_idx: u32 = 0,

    static fn empty(): PmLsEdge {
        return PmLsEdge{};
    }
}

/// Rendering options for `renderPmLsTree`.
pub struct PmLsRenderOptions {
    show_all: bool = false,
    use_unicode: bool = true,
    show_sizes: bool = true,
    max_depth: usize = PM_ROUTER_MAX_TREE_DEPTH,

    static fn defaults(): PmLsRenderOptions {
        return PmLsRenderOptions{};
    }
}

/// Render statistics returned by `renderPmLsTree`.
pub struct PmLsRenderResult {
    bytes_written: usize = 0,
    nodes_rendered: u32 = 0,
    circular_refs: u32 = 0,
    deduped_refs: u32 = 0,
    total_disk_bytes: u64 = 0,
}

fn countChildrenOf(edges: []const PmLsEdge, parent_idx: u32): usize {
    let n: usize = 0;
    let i: usize = 0;
    while (i < edges.len) {
        if (edges[i].parent_idx == parent_idx) n += 1;
        i += 1;
    }
    return n;
}

fn isInAncestorStack(stack: []const u32, depth: usize, node_idx: u32): bool {
    let i: usize = 0;
    while (i < depth) {
        if (stack[i] == node_idx) return true;
        i += 1;
    }
    return false;
}

fn renderLsSubtree(
    node_idx: u32,
    depth: usize,
    is_last: bool,
    branch_active: []bool,
    ancestors: []u32,
    nodes: []const PmLsNode,
    edges: []const PmLsEdge,
    opts: PmLsRenderOptions,
    visited: []bool,
    res: *PmLsRenderResult,
    out: []u8,
    pos: *usize,
)! {
    const idx = @as(usize, node_idx);
    if (idx >= nodes.len) return;

    // Emit prefix connectors for depth 1..depth-1
    let d: usize = 0;
    while (d < depth) {
        if (branch_active[d]) {
            pmrAppendBytes(out, pos, if (opts.use_unicode) "│   " else "|   ")?;
        } else {
            pmrAppendBytes(out, pos, "    ")?;
        }
        d += 1;
    }

    if (is_last) {
        pmrAppendBytes(out, pos, if (opts.use_unicode) "└── " else "`-- ")?;
    } else {
        pmrAppendBytes(out, pos, if (opts.use_unicode) "├── " else "|-- ")?;
    }

    const node = nodes[idx];
    pmrAppendBytes(out, pos, node.name)?;
    if (node.version.len > 0) {
        pmrAppendByte(out, pos, '@')?;
        pmrAppendBytes(out, pos, node.version)?;
    }

    const is_cycle = isInAncestorStack(ancestors, depth, node_idx);
    const already_visited = visited[idx];

    if (is_cycle) {
        pmrAppendBytes(out, pos, " (circular)")?;
        res.circular_refs += 1;
    } else if (already_visited) {
        pmrAppendBytes(out, pos, " (deduped)")?;
        res.deduped_refs += 1;
    } else if (opts.show_sizes && node.byte_size > 0) {
        pmrAppendBytes(out, pos, " [")?;
        pmrAppendHumanBytes(out, pos, node.byte_size)?;
        pmrAppendByte(out, pos, ']')?;
    }

    pmrAppendByte(out, pos, '\n')?;
    res.nodes_rendered += 1;

    if (is_cycle || already_visited) return;
    visited[idx] = true;
    res.total_disk_bytes += node.byte_size;

    if (!opts.show_all) return;
    if (depth + 1 >= opts.max_depth || depth + 1 >= PM_ROUTER_MAX_TREE_DEPTH) return;

    const child_total = countChildrenOf(edges, node_idx);
    if (child_total == 0) return;

    ancestors[depth] = node_idx;
    branch_active[depth] = !is_last;

    let seen_children: usize = 0;
    let e: usize = 0;
    while (e < edges.len) {
        if (edges[e].parent_idx == node_idx) {
            seen_children += 1;
            const child_last = (seen_children == child_total);
            renderLsSubtree(
                edges[e].child_idx,
                depth + 1,
                child_last,
                branch_active,
                ancestors,
                nodes,
                edges,
                opts,
                visited,
                res,
                out,
                pos,
            )?;
        }
        e += 1;
    }
}

/// Renders a formatted ASCII/Unicode dependency tree for `cap pm ls [--all]`.
pub fn renderPmLsTree(
    root_label: []const u8,
    nodes: []const PmLsNode,
    edges: []const PmLsEdge,
    opts: PmLsRenderOptions,
    visited_scratch: []bool,
    out: []u8,
): PmLsRenderResult! {
    if (visited_scratch.len < nodes.len) return err.SliceCapacityExceeded;
    let i: usize = 0;
    while (i < nodes.len) {
        visited_scratch[i] = false;
        i += 1;
    }

    let pos: usize = 0;
    let res = PmLsRenderResult{};

    pmrAppendBytes(out, &pos, root_label)?;
    pmrAppendByte(out, &pos, '\n')?;

    let direct_total: usize = 0;
    let j: usize = 0;
    while (j < nodes.len) {
        if (nodes[j].is_direct) direct_total += 1;
        j += 1;
    }

    let branch_active: [PM_ROUTER_MAX_TREE_DEPTH]bool = [false; PM_ROUTER_MAX_TREE_DEPTH];
    let ancestors: [PM_ROUTER_MAX_TREE_DEPTH]u32 = [0; PM_ROUTER_MAX_TREE_DEPTH];

    let seen_direct: usize = 0;
    let k: usize = 0;
    while (k < nodes.len) {
        if (nodes[k].is_direct) {
            seen_direct += 1;
            const is_last = (seen_direct == direct_total);
            renderLsSubtree(
                @as(u32, @intCast(k)),
                0,
                is_last,
                branch_active[0..],
                ancestors[0..],
                nodes,
                edges,
                opts,
                visited_scratch,
                &res,
                out,
                &pos,
            )?;
        }
        k += 1;
    }

    pmrAppendU64(out, &pos, @as(u64, res.nodes_rendered))?;
    pmrAppendBytes(out, &pos, " package(s)")?;
    if (opts.show_sizes) {
        pmrAppendBytes(out, &pos, " (")?;
        pmrAppendHumanBytes(out, &pos, res.total_disk_bytes)?;
        pmrAppendByte(out, &pos, ')')?;
    }
    pmrAppendByte(out, &pos, '\n')?;

    res.bytes_written = pos;
    return res;
}

// ============================================================================
// Section 3: Global Cache Auditor & LRU / TTL Prune Planner
// ============================================================================

/// Metadata for a single global package cache entry.
pub struct CacheEntryMeta {
    key: []const u8 = "",
    byte_size: u64 = 0,
    last_access_secs: u64 = 0,
    referenced_by_lockfile: bool = false,

    static fn empty(): CacheEntryMeta {
        return CacheEntryMeta{};
    }
}

/// Retention and eviction policy for `planCachePrune`.
pub struct CachePrunePolicy {
    now_secs: u64 = 0,
    max_bytes: u64 = 0,
    max_age_secs: u64 = 0,
    protect_referenced: bool = true,
}

/// Per-entry retention or eviction reason.
pub enum CachePruneDecisionKind: u8 {
    Keep = 0,
    KeepProtected = 1,
    EvictExpiredTtl = 2,
    EvictLruQuota = 3,
}

/// Individual cache prune decision.
pub struct CachePruneDecision {
    entry_index: u32 = 0,
    key: []const u8 = "",
    byte_size: u64 = 0,
    decision: CachePruneDecisionKind = CachePruneDecisionKind.Keep,

    static fn empty(): CachePruneDecision {
        return CachePruneDecision{};
    }
}

/// Aggregate cache audit & prune statistics.
pub struct CachePruneSummary {
    total_entries: u32 = 0,
    kept_count: u32 = 0,
    evicted_ttl_count: u32 = 0,
    evicted_lru_count: u32 = 0,
    bytes_kept: u64 = 0,
    bytes_reclaimed: u64 = 0,
}

/// Plans TTL-based and LRU-quota-based cache eviction into caller-owned `out_decisions`,
/// protecting entries referenced by the active lockfile when `policy.protect_referenced` is true.
pub fn planCachePrune(
    entries: []const CacheEntryMeta,
    policy: CachePrunePolicy,
    out_decisions: []CachePruneDecision,
): CachePruneSummary! {
    if (entries.len > out_decisions.len) return err.SliceCapacityExceeded;

    let sum = CachePruneSummary{
        total_entries: @as(u32, @intCast(entries.len)),
    };

    // Pass 1: Classify protected entries and evict TTL-expired entries
    let i: usize = 0;
    while (i < entries.len) {
        const e = entries[i];
        const is_protected = policy.protect_referenced && e.referenced_by_lockfile;
        const is_expired = (policy.max_age_secs > 0) &&
            (policy.now_secs >= e.last_access_secs) &&
            ((policy.now_secs - e.last_access_secs) > policy.max_age_secs);

        if (is_protected) {
            out_decisions[i] = CachePruneDecision{
                entry_index: @as(u32, @intCast(i)),
                key: e.key,
                byte_size: e.byte_size,
                decision: CachePruneDecisionKind.KeepProtected,
            };
            sum.kept_count += 1;
            sum.bytes_kept += e.byte_size;
        } else if (is_expired) {
            out_decisions[i] = CachePruneDecision{
                entry_index: @as(u32, @intCast(i)),
                key: e.key,
                byte_size: e.byte_size,
                decision: CachePruneDecisionKind.EvictExpiredTtl,
            };
            sum.evicted_ttl_count += 1;
            sum.bytes_reclaimed += e.byte_size;
        } else {
            out_decisions[i] = CachePruneDecision{
                entry_index: @as(u32, @intCast(i)),
                key: e.key,
                byte_size: e.byte_size,
                decision: CachePruneDecisionKind.Keep,
            };
            sum.kept_count += 1;
            sum.bytes_kept += e.byte_size;
        }
        i += 1;
    }

    // Pass 2: Enforce `max_bytes` quota by evicting least-recently-used unprotected entries
    if (policy.max_bytes > 0) {
        while (sum.bytes_kept > policy.max_bytes) {
            let victim: usize = entries.len;
            let oldest_ts: u64 = 0;
            let j: usize = 0;
            while (j < entries.len) {
                if (out_decisions[j].decision == CachePruneDecisionKind.Keep) {
                    if (victim == entries.len || entries[j].last_access_secs < oldest_ts) {
                        victim = j;
                        oldest_ts = entries[j].last_access_secs;
                    }
                }
                j += 1;
            }
            if (victim == entries.len) break;

            out_decisions[victim] = CachePruneDecision{
                entry_index: @as(u32, @intCast(victim)),
                key: entries[victim].key,
                byte_size: entries[victim].byte_size,
                decision: CachePruneDecisionKind.EvictLruQuota,
            };
            sum.kept_count -= 1;
            sum.evicted_lru_count += 1;
            sum.bytes_kept -= entries[victim].byte_size;
            sum.bytes_reclaimed += entries[victim].byte_size;
        }
    }

    return sum;
}

// ============================================================================
// Section 4: Deterministic Lockfile Hash & `cap pm whoami` Diagnostics
// ============================================================================

/// Canonical package tuple hashed by `computePmLockfileHash`.
pub struct PmLockHashEntry {
    name: []const u8 = "",
    version: []const u8 = "",
    resolved: []const u8 = "",
    integrity: []const u8 = "",
}

fn fnvMix(h: *u64, prime: u64, data: []const u8) {
    let i: usize = 0;
    while (i < data.len) {
        h.* = (h.* ^ @as(u64, data[i])) *% prime;
        i += 1;
    }
    h.* = (h.* ^ 0xFF) *% prime;
}

/// Computes a deterministic 128-bit (32 lowercase hex char) digest over lockfile entries
/// for `cap pm hash`.
pub fn computePmLockfileHash(
    entries: []const PmLockHashEntry,
    out_hex: []u8,
): usize! {
    if (out_hex.len < 32) return err.BufferTooSmall;
    let h1: u64 = 14695981039346656037;
    let h2: u64 = 0x6c62272e07bb0142;

    let i: usize = 0;
    while (i < entries.len) {
        const e = entries[i];
        fnvMix(&h1, 1099511628211, e.name);
        fnvMix(&h1, 1099511628211, e.version);
        fnvMix(&h1, 1099511628211, e.resolved);
        fnvMix(&h1, 1099511628211, e.integrity);

        fnvMix(&h2, 0x100000001b3, e.integrity);
        fnvMix(&h2, 0x100000001b3, e.resolved);
        fnvMix(&h2, 0x100000001b3, e.version);
        fnvMix(&h2, 0x100000001b3, e.name);
        i += 1;
    }

    let pos: usize = 0;
    pmrAppendHex64(out_hex, &pos, h1)?;
    pmrAppendHex64(out_hex, &pos, h2)?;
    return pos;
}

/// Authentication mechanism discovered by `diagnosePmWhoami`.
pub enum PmWhoamiAuthKind: u8 {
    None = 0,
    BearerToken = 1,
    BasicAuth = 2,
    EnvToken = 3,
}

/// Structured credential diagnostic for `cap pm whoami`.
pub struct PmWhoamiDiagnostic {
    authenticated: bool = false,
    auth_kind: PmWhoamiAuthKind = PmWhoamiAuthKind.None,
    registry_url: []const u8 = "",
    username_hint: []const u8 = "",
    redacted_token: []const u8 = "",
    message: []const u8 = "",
}

fn redactTokenInto(token: []const u8, buf: []u8): []const u8 {
    if (token.len == 0 || buf.len < 8) return "***";
    let pos: usize = 0;
    const prefix_len: usize = if (token.len >= 4) 4 else 1;
    let i: usize = 0;
    while (i < prefix_len && pos < buf.len) {
        buf[pos] = token[i];
        pos += 1;
        i += 1;
    }
    const stars = "***";
    let s: usize = 0;
    while (s < stars.len && pos < buf.len) {
        buf[pos] = stars[s];
        pos += 1;
        s += 1;
    }
    return buf[0..pos];
}

/// Diagnoses registry authentication credentials for `cap pm whoami` with automatic token redaction.
pub fn diagnosePmWhoami(
    registry_url: []const u8,
    npmrc_token: []const u8,
    npmrc_basic_user: []const u8,
    env_token: []const u8,
    redact_buf: []u8,
): PmWhoamiDiagnostic {
    const reg = if (registry_url.len > 0) registry_url else "https://registry.npmjs.org";

    if (npmrc_token.len > 0) {
        return PmWhoamiDiagnostic{
            authenticated: true,
            auth_kind: PmWhoamiAuthKind.BearerToken,
            registry_url: reg,
            redacted_token: redactTokenInto(npmrc_token, redact_buf),
            message: "authenticated via .npmrc / cap.toml bearer token",
        };
    }
    if (env_token.len > 0) {
        return PmWhoamiDiagnostic{
            authenticated: true,
            auth_kind: PmWhoamiAuthKind.EnvToken,
            registry_url: reg,
            redacted_token: redactTokenInto(env_token, redact_buf),
            message: "authenticated via environment token (NPM_TOKEN / CAP_AUTH_TOKEN)",
        };
    }
    if (npmrc_basic_user.len > 0) {
        return PmWhoamiDiagnostic{
            authenticated: true,
            auth_kind: PmWhoamiAuthKind.BasicAuth,
            registry_url: reg,
            username_hint: npmrc_basic_user,
            message: "authenticated via basic credentials",
        };
    }

    return PmWhoamiDiagnostic{
        authenticated: false,
        auth_kind: PmWhoamiAuthKind.None,
        registry_url: reg,
        message: "not logged in; configure _authToken in .npmrc or set CAP_AUTH_TOKEN",
    };
}
