// Source: Absorbed from /data/dev/bun/src/js_parser/ast/TS.cp | Synced: 2026-11-22T19:06:57Z
// ============================================================================
// lib/js/tsNamespace.cp — Zero-Alloc TypeScript Namespace & Enum Sibling Merge
// ============================================================================
// Implements Bun's `js_parser/ast/TS.cp` (`TSNamespaceScope` &
// `TSNamespaceMember`) in 100% pure CP with zero dynamic heap allocation:
// - Merges exported members across sibling `namespace Foo {}` or `enum Foo {}`
//   declarations sharing a common `group_id`.
// - Enforces TypeScript's isolation rule for implicit sibling lookups:
//   only enum-to-enum and namespace-to-namespace implicit references resolve
//   (enum-to-namespace and namespace-to-enum are rejected).
// - Lazily caches per-block IIFE parameter property-access proxies (`x3.y`).

pub const TS_NS_MAX_SCOPES: usize = 32;
pub const TS_NS_MAX_MEMBERS: usize = 128;
pub const TS_NS_MAX_PROXIES: usize = 128;

pub err TsNamespaceError {
    ScopesFull,
    MembersFull,
    ProxiesFull,
    InvalidScopeId,
    BufferTooSmall,
}

/// Kind of exported member inside a TypeScript `namespace` or `enum`
/// (1:1 port of Bun's `TSNamespaceMemberData`).
pub enum TsNamespaceMemberKind: u8 {
    property = 0,
    nested_namespace = 1,
    enum_number = 2,
    enum_string = 3,
    enum_property = 4,

    fn isEnum(): bool {
        return _ == TsNamespaceMemberKind.enum_number ||
            _ == TsNamespaceMemberKind.enum_string ||
            _ == TsNamespaceMemberKind.enum_property;
    }
}

fn hashIdent(s: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < s.len) {
        h = h ^ @as(u64, s[i]);
        h = h *% 1099511628211;
        i += 1;
    }
    return h;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// Exported member shared across all sibling blocks of a `namespace` or `enum`.
pub struct TsNamespaceMember {
    in_use: bool = false,
    group_id: u32 = 0,
    name_hash: u64 = 0,
    name: []const u8 = "",
    loc_start: u32 = 0,
    kind: TsNamespaceMemberKind = TsNamespaceMemberKind.property,
    num_value: f64 = 0.0,
    str_value: []const u8 = "",
    child_group_id: u32 = 0,

    fn isEnum(): bool {
        return _.kind.isEnum();
    }
}

/// Single `namespace Foo {}` or `enum Foo {}` block instance (aligned with `TSNamespaceScope`).
pub struct TsNamespaceBlock {
    in_use: bool = false,
    group_id: u32 = 0,
    arg_ref: u32 = 0,
    arg_name: []const u8 = "",
    is_enum_scope: bool = false,
}

/// Per-block cached proxy ref representing an implicit property access (`arg_name.member_name`).
pub struct TsPropertyAccessProxy {
    in_use: bool = false,
    block_id: u32 = 0,
    name_hash: u64 = 0,
    member_name: []const u8 = "",
    proxy_ref: u32 = 0,
}

/// Fixed-capacity zero-allocation registry managing sibling TypeScript
/// `namespace`/`enum` blocks, shared exported members, and per-block access proxies.
pub struct TsNamespaceRegistry {
    next_group_id: u32 = 1,
    blocks: [TS_NS_MAX_SCOPES]TsNamespaceBlock = [TsNamespaceBlock{}; TS_NS_MAX_SCOPES],
    block_count: usize = 0,
    members: [TS_NS_MAX_MEMBERS]TsNamespaceMember = [TsNamespaceMember{}; TS_NS_MAX_MEMBERS],
    member_count: usize = 0,
    proxies: [TS_NS_MAX_PROXIES]TsPropertyAccessProxy = [TsPropertyAccessProxy{}; TS_NS_MAX_PROXIES],
    proxy_count: usize = 0,

    static fn init(): TsNamespaceRegistry {
        return TsNamespaceRegistry{};
    }

    fn createGroup(*_): u32 {
        const gid = _.next_group_id;
        _.next_group_id += 1;
        return gid;
    }

    fn blockCount(): usize {
        return _.block_count;
    }

    fn memberCount(): usize {
        return _.member_count;
    }

    fn proxyCount(): usize {
        return _.proxy_count;
    }

    /// Registers a new `namespace` or `enum` block participating in `group_id`.
    fn openBlock(
        *_,
        group_id: u32,
        arg_ref: u32,
        arg_name: []const u8,
        is_enum_scope: bool,
    ): TsNamespaceError!u32 {
        if (_.block_count >= TS_NS_MAX_SCOPES) return TsNamespaceError.ScopesFull;
        const bid = @as(u32, @intCast(_.block_count));
        _.blocks[_.block_count] = TsNamespaceBlock{
            in_use: true,
            group_id: group_id,
            arg_ref: arg_ref,
            arg_name: arg_name,
            is_enum_scope: is_enum_scope,
        };
        _.block_count += 1;
        return bid;
    }

    fn putMember(
        *_,
        group_id: u32,
        name: []const u8,
        loc_start: u32,
        kind: TsNamespaceMemberKind,
        num_value: f64,
        str_value: []const u8,
        child_group_id: u32,
    ): TsNamespaceError!bool {
        const h = hashIdent(name);
        let i: usize = 0;
        while (i < _.member_count) {
            if (_.members[i].in_use &&
                _.members[i].group_id == group_id &&
                _.members[i].name_hash == h &&
                strEq(_.members[i].name, name))
            {
                _.members[i].loc_start = loc_start;
                _.members[i].kind = kind;
                _.members[i].num_value = num_value;
                _.members[i].str_value = str_value;
                _.members[i].child_group_id = child_group_id;
                return false; // updated existing
            }
            i += 1;
        }
        if (_.member_count >= TS_NS_MAX_MEMBERS) return TsNamespaceError.MembersFull;
        _.members[_.member_count] = TsNamespaceMember{
            in_use: true,
            group_id: group_id,
            name_hash: h,
            name: name,
            loc_start: loc_start,
            kind: kind,
            num_value: num_value,
            str_value: str_value,
            child_group_id: child_group_id,
        };
        _.member_count += 1;
        return true; // inserted new
    }

    fn exportProperty(*_, group_id: u32, name: []const u8, loc_start: u32): TsNamespaceError!bool {
        return _.putMember(group_id, name, loc_start, TsNamespaceMemberKind.property, 0.0, "", 0);
    }

    fn exportNestedNamespace(
        *_,
        group_id: u32,
        name: []const u8,
        child_group_id: u32,
        loc_start: u32,
    ): TsNamespaceError!bool {
        return _.putMember(group_id, name, loc_start, TsNamespaceMemberKind.nested_namespace, 0.0, "", child_group_id);
    }

    fn exportEnumNumber(
        *_,
        group_id: u32,
        name: []const u8,
        val: f64,
        loc_start: u32,
    ): TsNamespaceError!bool {
        return _.putMember(group_id, name, loc_start, TsNamespaceMemberKind.enum_number, val, "", 0);
    }

    fn exportEnumString(
        *_,
        group_id: u32,
        name: []const u8,
        val: []const u8,
        loc_start: u32,
    ): TsNamespaceError!bool {
        return _.putMember(group_id, name, loc_start, TsNamespaceMemberKind.enum_string, 0.0, val, 0);
    }

    fn exportEnumProperty(*_, group_id: u32, name: []const u8, loc_start: u32): TsNamespaceError!bool {
        return _.putMember(group_id, name, loc_start, TsNamespaceMemberKind.enum_property, 0.0, "", 0);
    }

    fn lookupMember(group_id: u32, name: []const u8): TsNamespaceMember? {
        const h = hashIdent(name);
        let i: usize = 0;
        while (i < _.member_count) {
            if (_.members[i].in_use &&
                _.members[i].group_id == group_id &&
                _.members[i].name_hash == h &&
                strEq(_.members[i].name, name))
            {
                return _.members[i];
            }
            i += 1;
        }
        return null;
    }

    /// Resolves an implicit identifier inside `block_id` against sibling exported members.
    /// Enforces Bun/TS rule: enum blocks only see enum members, and namespace blocks only see
    /// namespace members (preventing enum-to-namespace and namespace-to-enum leakage).
    fn lookupImplicitInBlock(block_id: u32, name: []const u8): TsNamespaceMember? {
        const bidx = @as(usize, block_id);
        if (bidx >= _.block_count || !_.blocks[bidx].in_use) return null;
        const blk = _.blocks[bidx];
        const m = _.lookupMember(blk.group_id, name) ?? return null;
        if (blk.is_enum_scope != m.isEnum()) return null;
        return m;
    }

    /// Lazily retrieves or creates a per-block proxy symbol ref for `member_name`
    /// (aligned with Bun's `TSNamespaceScope.property_accesses`).
    fn getOrCreatePropertyProxy(
        *_,
        block_id: u32,
        member_name: []const u8,
        allocated_ref: u32,
    ): TsNamespaceError!u32 {
        const bidx = @as(usize, block_id);
        if (bidx >= _.block_count || !_.blocks[bidx].in_use) {
            return TsNamespaceError.InvalidScopeId;
        }
        const h = hashIdent(member_name);
        let i: usize = 0;
        while (i < _.proxy_count) {
            if (_.proxies[i].in_use &&
                _.proxies[i].block_id == block_id &&
                _.proxies[i].name_hash == h &&
                strEq(_.proxies[i].member_name, member_name))
            {
                return _.proxies[i].proxy_ref;
            }
            i += 1;
        }
        if (_.proxy_count >= TS_NS_MAX_PROXIES) return TsNamespaceError.ProxiesFull;
        _.proxies[_.proxy_count] = TsPropertyAccessProxy{
            in_use: true,
            block_id: block_id,
            name_hash: h,
            member_name: member_name,
            proxy_ref: allocated_ref,
        };
        _.proxy_count += 1;
        return allocated_ref;
    }

    /// Formats the lowered IIFE property access `<arg_name>.<member_name>` (e.g. `"ns3.y"`).
    fn formatPropertyAccess(block_id: u32, member_name: []const u8, out: []u8): TsNamespaceError!usize {
        const bidx = @as(usize, block_id);
        if (bidx >= _.block_count || !_.blocks[bidx].in_use) {
            return TsNamespaceError.InvalidScopeId;
        }
        const arg = _.blocks[bidx].arg_name;
        const total = arg.len + 1 + member_name.len;
        if (total > out.len) return TsNamespaceError.BufferTooSmall;
        let pos: usize = 0;
        let i: usize = 0;
        while (i < arg.len) {
            out[pos] = arg[i];
            pos += 1;
            i += 1;
        }
        out[pos] = '.';
        pos += 1;
        i = 0;
        while (i < member_name.len) {
            out[pos] = member_name[i];
            pos += 1;
            i += 1;
        }
        return pos;
    }
}
