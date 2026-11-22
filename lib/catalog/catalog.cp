// Source: Absorbed from /data/dev/bun/src/install/lockfile/CatalogMap.cp | Synced: 2026-11-22T12:49:17Z (Zero-Alloc Monorepo Dependency Catalogs Protocol & Map)

const CATALOG_PREFIX: []const u8 = "catalog:";

/// Error status returned by `CatalogMap` mutation operations.
pub enum CatalogError: u8 {
    ok = 0,
    capacity_full = 1,
    duplicate_entry = 2,
    invalid_package_name = 3,
    invalid_catalog_name = 4,
    nested_catalog_ref = 5,
    invalid_version = 6,
    not_found = 7,
}

/// Parsed `catalog:` protocol reference (`catalog:` for default, `catalog:<name>` for named group).
pub struct CatalogRef {
    catalog_name: []const u8,
    is_default: bool,
}

/// Single catalog entry mapping `(catalog_name, package_name) -> version`.
/// Empty `catalog_name` represents the default `catalog` (singular).
pub struct CatalogEntry {
    catalog_name: []const u8,
    package_name: []const u8,
    version: []const u8,
}

fn isSpaceOrCtrl(c: u8): bool {
    return c <= 0x20 || c == 0x7F;
}

/// Returns `true` if `spec` starts with the `"catalog:"` protocol prefix.
pub fn isCatalogProtocol(spec: []const u8): bool {
    return spec.len >= CATALOG_PREFIX.len && std.mem.eql(u8, spec[0..CATALOG_PREFIX.len], CATALOG_PREFIX);
}

/// Validates a named catalog identifier (non-empty, no ASCII control/whitespace, no `/` or `\`).
pub fn isValidCatalogName(name: []const u8): bool {
    if (name.len == 0) return false;
    let i: usize = 0;
    while (i < name.len) {
        const c = name[i];
        if (isSpaceOrCtrl(c) || c == '/' || c == '\\' || c == ':') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isValidPackageName(name: []const u8): bool {
    if (name.len == 0) return false;
    let i: usize = 0;
    while (i < name.len) {
        if (isSpaceOrCtrl(name[i])) return false;
        i += 1;
    }
    return true;
}

fn isValidVersionSpec(ver: []const u8): bool {
    if (ver.len == 0) return false;
    let i: usize = 0;
    while (i < ver.len) {
        if (ver[i] < 0x20 || ver[i] == 0x7F) return false;
        i += 1;
    }
    return true;
}

/// Parses a `"catalog:"` or `"catalog:<group>"` dependency specifier.
/// Returns `null` if `spec` is not a valid catalog reference.
pub fn parseCatalogRef(spec: []const u8): CatalogRef? {
    if (!isCatalogProtocol(spec)) return null;
    const tail = spec[CATALOG_PREFIX.len..];
    if (tail.len == 0) {
        return { catalog_name: "", is_default: true };
    }
    if (!isValidCatalogName(tail)) return null;
    return { catalog_name: tail, is_default: false };
}

/// Formats a catalog name into `"catalog:"` (when empty) or `"catalog:<name>"` into `out`.
pub fn formatCatalogRef(catalog_name: []const u8, out: *[]u8): ([]const u8)? {
    if (catalog_name.len > 0 && !isValidCatalogName(catalog_name)) {
        return null;
    }
    const need = CATALOG_PREFIX.len + catalog_name.len;
    if (out.*.len < need) return null;
    let i: usize = 0;
    while (i < CATALOG_PREFIX.len) {
        out.*[i] = CATALOG_PREFIX[i];
        i += 1;
    }
    let j: usize = 0;
    while (j < catalog_name.len) {
        out.*[CATALOG_PREFIX.len + j] = catalog_name[j];
        j += 1;
    }
    return out.*[0..need];
}

fn cmpBytes(a: []const u8, b: []const u8): i32 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
        i += 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

fn entryLessThan(a: CatalogEntry, b: CatalogEntry): bool {
    const c_cat = cmpBytes(a.catalog_name, b.catalog_name);
    if (c_cat != 0) return c_cat < 0;
    return cmpBytes(a.package_name, b.package_name) < 0;
}

fn appendStr(out: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.*.len) return false;
    let i: usize = 0;
    while (i < s.len) {
        out.*[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

/// Zero-allocation monorepo dependency catalog map backed by caller-provided `storage`.
pub struct CatalogMap {
    storage: []CatalogEntry,
    count: usize,

    fn init(storage: []CatalogEntry): CatalogMap {
        return {
            storage: storage,
            count: 0,
        };
    }

    fn hasAny(): bool {
        return _.count > 0;
    }

    fn len(): usize {
        return _.count;
    }

    fn defaultCount(): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (_.storage[i].catalog_name.len == 0) {
                n += 1;
            }
            i += 1;
        }
        return n;
    }

    fn groupCount(catalog_name: []const u8): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.storage[i].catalog_name, catalog_name)) {
                n += 1;
            }
            i += 1;
        }
        return n;
    }

    fn findIndex(catalog_name: []const u8, package_name: []const u8): usize? {
        let i: usize = 0;
        while (i < _.count) {
            const e = _.storage[i];
            if (std.mem.eql(u8, e.catalog_name, catalog_name) && std.mem.eql(u8, e.package_name, package_name)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn validateEntry(catalog_name: []const u8, package_name: []const u8, version: []const u8): CatalogError {
        if (catalog_name.len > 0 && !isValidCatalogName(catalog_name)) {
            return CatalogError.invalid_catalog_name;
        }
        if (!isValidPackageName(package_name)) {
            return CatalogError.invalid_package_name;
        }
        if (!isValidVersionSpec(version)) {
            return CatalogError.invalid_version;
        }
        if (isCatalogProtocol(version)) {
            return CatalogError.nested_catalog_ref;
        }
        return CatalogError.ok;
    }

    /// Inserts a package into the default `catalog` (rejects duplicates).
    fn putDefault(*_, package_name: []const u8, version: []const u8): CatalogError {
        return _.putInternal("", package_name, version, false);
    }

    /// Inserts a package into a named `catalogs.<catalog_name>` group (rejects empty group name & duplicates).
    fn putNamed(*_, catalog_name: []const u8, package_name: []const u8, version: []const u8): CatalogError {
        if (catalog_name.len == 0) return CatalogError.invalid_catalog_name;
        return _.putInternal(catalog_name, package_name, version, false);
    }

    /// Inserts or updates a catalog entry (`catalog_name == ""` targets the default catalog).
    fn upsert(*_, catalog_name: []const u8, package_name: []const u8, version: []const u8): CatalogError {
        return _.putInternal(catalog_name, package_name, version, true);
    }

    fn putInternal(*_, catalog_name: []const u8, package_name: []const u8, version: []const u8, allow_overwrite: bool): CatalogError {
        const v_err = _.validateEntry(catalog_name, package_name, version);
        if (v_err != CatalogError.ok) return v_err;

        if (idx := _.findIndex(catalog_name, package_name)) {
            if (!allow_overwrite) return CatalogError.duplicate_entry;
            _.storage[idx].version = version;
            return CatalogError.ok;
        }
        if (_.count >= _.storage.len) return CatalogError.capacity_full;
        _.storage[_.count] = {
            catalog_name: catalog_name,
            package_name: package_name,
            version: version,
        };
        _.count += 1;
        return CatalogError.ok;
    }

    /// Looks up `(catalog_name, package_name)`. Empty `catalog_name` queries the default catalog.
    fn get(catalog_name: []const u8, package_name: []const u8): ([]const u8)? {
        if (idx := _.findIndex(catalog_name, package_name)) {
            return _.storage[idx].version;
        }
        return null;
    }

    /// Resolves a workspace dependency specifier:
    /// - If `spec` starts with `"catalog:"`, resolves it against the corresponding catalog entry (or returns `null` if invalid/missing).
    /// - Otherwise returns `spec` unchanged (zero-copy passthrough).
    fn resolveSpecifier(package_name: []const u8, spec: []const u8): ([]const u8)? {
        if (!isCatalogProtocol(spec)) {
            return spec;
        }
        if (cref := parseCatalogRef(spec)) {
            return _.get(cref.catalog_name, package_name);
        }
        return null;
    }

    /// In-place deterministic insertion sort by `(catalog_name, package_name)` for reproducible lockfile emission.
    fn sortEntries(*_) {
        let i: usize = 1;
        while (i < _.count) {
            const key = _.storage[i];
            let j = i;
            while (j > 0 && entryLessThan(key, _.storage[j - 1])) {
                _.storage[j] = _.storage[j - 1];
                j -= 1;
            }
            _.storage[j] = key;
            i += 1;
        }
    }

    /// Serializes sorted catalog entries into a deterministic JSON-style `"catalog"` / `"catalogs"` fragment.
    /// Call `sortEntries()` beforehand for canonical lexicographical ordering.
    fn formatLockfileSection(out: *[]u8): ([]const u8)? {
        let pos: usize = 0;
        if (!appendStr(out, *pos, "{")) return null;

        let wrote_section = false;
        // 1. Default catalog
        if (_.defaultCount() > 0) {
            if (!appendStr(out, *pos, "\"catalog\":{")) return null;
            let first = true;
            let i: usize = 0;
            while (i < _.count) {
                const e = _.storage[i];
                if (e.catalog_name.len == 0) {
                    if (!first) {
                        if (!appendStr(out, *pos, ",")) return null;
                    }
                    first = false;
                    if (!appendStr(out, *pos, "\"")) return null;
                    if (!appendStr(out, *pos, e.package_name)) return null;
                    if (!appendStr(out, *pos, "\":\"")) return null;
                    if (!appendStr(out, *pos, e.version)) return null;
                    if (!appendStr(out, *pos, "\"")) return null;
                }
                i += 1;
            }
            if (!appendStr(out, *pos, "}")) return null;
            wrote_section = true;
        }

        // 2. Named catalogs
        let has_named = false;
        let chk: usize = 0;
        while (chk < _.count) {
            if (_.storage[chk].catalog_name.len > 0) {
                has_named = true;
                break;
            }
            chk += 1;
        }

        if (has_named) {
            if (wrote_section) {
                if (!appendStr(out, *pos, ",")) return null;
            }
            if (!appendStr(out, *pos, "\"catalogs\":{")) return null;

            let first_group = true;
            let g: usize = 0;
            while (g < _.count) {
                const gname = _.storage[g].catalog_name;
                if (gname.len > 0) {
                    // Check if this group name was already emitted earlier
                    let seen = false;
                    let prev: usize = 0;
                    while (prev < g) {
                        if (std.mem.eql(u8, _.storage[prev].catalog_name, gname)) {
                            seen = true;
                            break;
                        }
                        prev += 1;
                    }
                    if (!seen) {
                        if (!first_group) {
                            if (!appendStr(out, *pos, ",")) return null;
                        }
                        first_group = false;
                        if (!appendStr(out, *pos, "\"")) return null;
                        if (!appendStr(out, *pos, gname)) return null;
                        if (!appendStr(out, *pos, "\":{")) return null;

                        let first_item = true;
                        let k = g;
                        while (k < _.count) {
                            const e = _.storage[k];
                            if (std.mem.eql(u8, e.catalog_name, gname)) {
                                if (!first_item) {
                                    if (!appendStr(out, *pos, ",")) return null;
                                }
                                first_item = false;
                                if (!appendStr(out, *pos, "\"")) return null;
                                if (!appendStr(out, *pos, e.package_name)) return null;
                                if (!appendStr(out, *pos, "\":\"")) return null;
                                if (!appendStr(out, *pos, e.version)) return null;
                                if (!appendStr(out, *pos, "\"")) return null;
                            }
                            k += 1;
                        }
                        if (!appendStr(out, *pos, "}")) return null;
                    }
                }
                g += 1;
            }
            if (!appendStr(out, *pos, "}")) return null;
        }

        if (!appendStr(out, *pos, "}")) return null;
        return out.*[0..pos];
    }
}
