// Source: Absorbed from /data/dev/bun/src/bake/DevServer/Assets.cp | Synced: 2026-11-22T17:10:20Z
// Pure-CP zero-allocation content-addressable reference-counted static asset store
// for DevServer (`/_bun/asset/{hash}.ext`): deduplicates identical content hashes across
// multiple source paths, supports in-place replacement when `refs == 1`, revokes stale
// hashes on overwrite, and preserves dense O(1) swap-remove with path index fixup.

pub const ASSET_STORE_MAX_PATHS: usize = 128;
pub const ASSET_STORE_MAX_FILES: usize = 128;
pub const ASSET_URL_PREFIX: []const u8 = "/_bun/asset/";

pub error AssetStoreError {
    PathsCapacityExceeded,
    FilesCapacityExceeded,
    AssetNotFound,
    DoubleUnref,
    InvalidDecrement,
    BufferTooSmall,
}

pub struct AssetEntry {
    content_hash: u64 = 0,
    size_bytes: usize = 0,
    mime_type: []const u8 = "application/octet-stream",
    refs: u32 = 0,
}

pub struct AssetPathMapping {
    path: []const u8 = "",
    entry_index: usize = 0,
}

pub struct AssetReplaceOutcome {
    entry_index: usize = 0,
    replaced_in_place: bool = false,
    shared_existing: bool = false,
    evicted_hash: u64? = null,
}

/// Formats a content-addressed asset URL `/_bun/asset/<16-lower-hex><ext>` into `out`.
/// `ext` may be empty, start with `'.'` (e.g. `".png"`), or omit the dot (e.g. `"png"`).
pub fn formatAssetUrl(content_hash: u64, ext: []const u8, out: []u8): []const u8 ! AssetStoreError {
    const need_dot = ext.len > 0 && ext[0] != '.';
    const total = ASSET_URL_PREFIX.len + 16 + (if (need_dot) 1 else 0) + ext.len;
    if (total > out.len) return err.BufferTooSmall;

    let pos: usize = 0;
    let i: usize = 0;
    while (i < ASSET_URL_PREFIX.len) {
        out[pos] = ASSET_URL_PREFIX[i];
        pos += 1;
        i += 1;
    }

    const hex_digits = "0123456789abcdef";
    let shift: u6 = 60;
    let d: usize = 0;
    while (d < 16) {
        const nibble: usize = @as(usize, (content_hash >> shift) & 0xF);
        out[pos] = hex_digits[nibble];
        pos += 1;
        if (shift >= 4) shift -= 4;
        d += 1;
    }

    if (need_dot) {
        out[pos] = '.';
        pos += 1;
    }
    i = 0;
    while (i < ext.len) {
        out[pos] = ext[i];
        pos += 1;
        i += 1;
    }
    return out[0..pos];
}

/// Zero-allocation content-addressable reference-counted static asset store.
pub struct AssetStore {
    paths: [ASSET_STORE_MAX_PATHS]AssetPathMapping = [AssetPathMapping{}; ASSET_STORE_MAX_PATHS],
    path_count: usize = 0,

    files: [ASSET_STORE_MAX_FILES]AssetEntry = [AssetEntry{}; ASSET_STORE_MAX_FILES],
    file_count: usize = 0,

    static fn init(): AssetStore {
        return AssetStore{};
    }

    fn pathCount(): usize {
        return _.path_count;
    }

    fn uniqueAssetCount(): usize {
        return _.file_count;
    }

    fn totalMemoryBytes(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.file_count) {
            total += _.files[i].size_bytes;
            i += 1;
        }
        return total;
    }

    /// Returns the content hash currently mapped to `path`, if registered.
    fn getHash(path: []const u8): u64? {
        if (pidx := _.findPathIndex(path)) {
            const fidx = _.paths[pidx].entry_index;
            if (fidx < _.file_count) {
                return _.files[fidx].content_hash;
            }
        }
        return null;
    }

    /// Looks up a content-addressed asset entry by `content_hash`.
    fn getByHash(content_hash: u64): AssetEntry? {
        if (fidx := _.findFileIndexByHash(content_hash)) {
            return _.files[fidx];
        }
        return null;
    }

    /// Returns the live reference count for `content_hash` (0 if absent).
    fn getRefCount(content_hash: u64): u32 {
        if (fidx := _.findFileIndexByHash(content_hash)) {
            return _.files[fidx].refs;
        }
        return 0;
    }

    /// Inserts or replaces the asset mapped to `abs_path` with `content_hash`.
    /// Mirrors Bun `Assets.replacePath`:
    /// - Reuses the existing file slot in-place when `refs == 1` and `content_hash` is not already stored elsewhere;
    /// - Otherwise decrements the previous entry's ref count (evicting via swap-remove if it hits 0)
    ///   and binds `abs_path` to an existing or newly created content entry.
    fn replacePath(
        *_,
        abs_path: []const u8,
        content_hash: u64,
        size_bytes: usize,
        mime_type: []const u8,
    ): AssetReplaceOutcome ! AssetStoreError {
        if (pidx := _.findPathIndex(abs_path)) {
            const old_fidx = _.paths[pidx].entry_index;
            const old_hash = _.files[old_fidx].content_hash;

            // Identical hash overwrite: refresh metadata in-place without touching refs
            if (old_hash == content_hash) {
                _.files[old_fidx].size_bytes = size_bytes;
                _.files[old_fidx].mime_type = mime_type;
                return AssetReplaceOutcome{
                    entry_index: old_fidx,
                    replaced_in_place: true,
                    shared_existing: false,
                    evicted_hash: null,
                };
            }

            const existing_target = _.findFileIndexByHash(content_hash);
            if (_.files[old_fidx].refs == 1 && existing_target == null) {
                // Sole owner and new hash is fresh -> replace in-place & revoke old hash immediately
                _.files[old_fidx] = AssetEntry{
                    content_hash: content_hash,
                    size_bytes: size_bytes,
                    mime_type: mime_type,
                    refs: 1,
                };
                return AssetReplaceOutcome{
                    entry_index: old_fidx,
                    replaced_in_place: true,
                    shared_existing: false,
                    evicted_hash: old_hash,
                };
            }

            // Detach path from old entry first, then bind to existing or new entry
            if (target_fidx := existing_target) {
                _.files[target_fidx].refs += 1;
                _.paths[pidx].entry_index = target_fidx;
                const evicted = _.unrefByIndexInternal(old_fidx, 1)?;
                // Note: if old_fidx was swapped with target_fidx during eviction, path_map was fixed up
                const final_fidx = _.paths[pidx].entry_index;
                return AssetReplaceOutcome{
                    entry_index: final_fidx,
                    replaced_in_place: false,
                    shared_existing: true,
                    evicted_hash: if (evicted) old_hash else null,
                };
            } else {
                if (_.file_count >= ASSET_STORE_MAX_FILES) {
                    return err.FilesCapacityExceeded;
                }
                const new_fidx = _.file_count;
                _.files[new_fidx] = AssetEntry{
                    content_hash: content_hash,
                    size_bytes: size_bytes,
                    mime_type: mime_type,
                    refs: 1,
                };
                _.file_count += 1;
                _.paths[pidx].entry_index = new_fidx;
                const evicted = _.unrefByIndexInternal(old_fidx, 1)?;
                const final_fidx = _.paths[pidx].entry_index;
                return AssetReplaceOutcome{
                    entry_index: final_fidx,
                    replaced_in_place: false,
                    shared_existing: false,
                    evicted_hash: if (evicted) old_hash else null,
                };
            }
        }

        // Brand-new path mapping
        if (_.path_count >= ASSET_STORE_MAX_PATHS) {
            return err.PathsCapacityExceeded;
        }
        if (existing_fidx := _.findFileIndexByHash(content_hash)) {
            _.files[existing_fidx].refs += 1;
            _.paths[_.path_count] = AssetPathMapping{
                path: abs_path,
                entry_index: existing_fidx,
            };
            _.path_count += 1;
            return AssetReplaceOutcome{
                entry_index: existing_fidx,
                replaced_in_place: false,
                shared_existing: true,
                evicted_hash: null,
            };
        }
        if (_.file_count >= ASSET_STORE_MAX_FILES) {
            return err.FilesCapacityExceeded;
        }
        const new_fidx = _.file_count;
        _.files[new_fidx] = AssetEntry{
            content_hash: content_hash,
            size_bytes: size_bytes,
            mime_type: mime_type,
            refs: 1,
        };
        _.file_count += 1;
        _.paths[_.path_count] = AssetPathMapping{
            path: abs_path,
            entry_index: new_fidx,
        };
        _.path_count += 1;
        return AssetReplaceOutcome{
            entry_index: new_fidx,
            replaced_in_place: false,
            shared_existing: false,
            evicted_hash: null,
        };
    }

    /// Inserts a content-addressed asset with `ref_count` or increments its existing ref count.
    /// Returns `true` if newly inserted, `false` if already present (mirrors Bun `putOrIncrementRefCount`).
    fn putOrIncrementRefCount(
        *_,
        content_hash: u64,
        size_bytes: usize,
        mime_type: []const u8,
        ref_count: u32,
    ): bool ! AssetStoreError {
        if (ref_count == 0) return err.InvalidDecrement;
        if (fidx := _.findFileIndexByHash(content_hash)) {
            _.files[fidx].refs += ref_count;
            return false;
        }
        if (_.file_count >= ASSET_STORE_MAX_FILES) {
            return err.FilesCapacityExceeded;
        }
        _.files[_.file_count] = AssetEntry{
            content_hash: content_hash,
            size_bytes: size_bytes,
            mime_type: mime_type,
            refs: ref_count,
        };
        _.file_count += 1;
        return true;
    }

    /// Decrements the reference count of `content_hash` by `dec_count`.
    /// Evicts via swap-remove and fixes up `paths` indices when count reaches 0.
    /// Returns `true` if the asset was evicted.
    fn unrefByHash(*_, content_hash: u64, dec_count: u32): bool ! AssetStoreError {
        if (dec_count == 0) return err.InvalidDecrement;
        if (fidx := _.findFileIndexByHash(content_hash)) {
            return _.unrefByIndexInternal(fidx, dec_count);
        }
        return err.AssetNotFound;
    }

    /// Removes `abs_path` from the path map and decrements the referenced asset by 1.
    /// Returns `true` if the underlying content entry was evicted.
    fn removePath(*_, abs_path: []const u8): bool ! AssetStoreError {
        if (pidx := _.findPathIndex(abs_path)) {
            const fidx = _.paths[pidx].entry_index;
            // Swap-remove path mapping first so fixup won't inspect a stale entry
            const last_pidx = _.path_count - 1;
            if (pidx != last_pidx) {
                _.paths[pidx] = _.paths[last_pidx];
            }
            _.path_count -= 1;
            return _.unrefByIndexInternal(fidx, 1);
        }
        return err.AssetNotFound;
    }

    // --- Internal helpers ---

    fn findPathIndex(path: []const u8): usize? {
        let i: usize = 0;
        while (i < _.path_count) {
            if (_.paths[i].path == path) return i;
            i += 1;
        }
        return null;
    }

    fn findFileIndexByHash(content_hash: u64): usize? {
        let i: usize = 0;
        while (i < _.file_count) {
            if (_.files[i].content_hash == content_hash) return i;
            i += 1;
        }
        return null;
    }

    fn unrefByIndexInternal(*_, index: usize, dec_count: u32): bool ! AssetStoreError {
        if (dec_count == 0) return err.InvalidDecrement;
        if (index >= _.file_count) return err.AssetNotFound;
        if (dec_count > _.files[index].refs) return err.DoubleUnref;
        _.files[index].refs -= dec_count;
        if (_.files[index].refs == 0) {
            const moved_from = _.file_count - 1;
            if (moved_from != index) {
                _.files[index] = _.files[moved_from];
                // Fix up all path mappings pointing to moved_from -> index (mirrors Bun Assets.unrefByIndex)
                let i: usize = 0;
                while (i < _.path_count) {
                    if (_.paths[i].entry_index == moved_from) {
                        _.paths[i].entry_index = index;
                    }
                    i += 1;
                }
            }
            _.file_count -= 1;
            return true;
        }
        return false;
    }
}
