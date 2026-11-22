// Source: cp/lib/radix/radix.cp (Pure CP Compressed Patricia Radix Tree)
// Zero-C-dependency compressed radix tree (Patricia trie) supporting:
//   - Common-prefix edge compression & automatic split on insert
//   - Automatic single-child merge compression on delete
//   - O(k) exact lookup, upsert, longestPrefix (CIDR / rule matching),
//     and lexicographical scanPrefix

const std = @import("std");

/// Error set for RadixTree and RadixRouter operations.
pub err RadixError {
    KeyNotFound,
    DuplicateKey,
    OutOfMemory,
}

/// Result returned by longestPrefix().
/// `key` is a borrowed slice pointing to the matched key stored in the tree.
pub struct RadixMatch<T> {
    pub key: []const u8,
    pub value: T,
}

/// Entry returned by scanPrefix().
/// `key` is a borrowed slice pointing to the entry key stored in the tree.
pub struct RadixEntry<T> {
    pub key: []const u8,
    pub value: T,
}

struct RadixNode<T> {
    prefix: []u8,
    full_key: []u8,
    value: T?,
    is_terminal: bool,
    children: std.ArrayList(*@This()) = .empty,
}

fn dupeBytes(alloc: std.mem.Allocator, s: []const u8): []u8 ! RadixError {
    if (s.len == 0) return [];
    return alloc.dupe(u8, s) catch return RadixError.OutOfMemory;
}

fn freeBytes(alloc: std.mem.Allocator, s: []u8) {
    if (s.len > 0) {
        alloc.free(s);
    }
}

fn commonPrefixLen(a: []const u8, b: []const u8): usize {
    const n = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < n && a[i] == b[i]) {
        i += 1;
    }
    return i;
}

/// Generic Compressed Patricia Radix Tree mapping byte-slice keys (`[]const u8`) to `T`.
pub struct RadixTree<T> {
    const NodeType = RadixNode(T);
    alloc: std.mem.Allocator,
    root: (*RadixNode<T>)? = null,
    len: usize = 0,

    /// Initialize an empty RadixTree. Does not allocate until the first insert.
    pub fn init(allocator: std.mem.Allocator): RadixTree<T> {
        return RadixTree<T>{
            alloc: allocator,
            root: null,
            len: 0,
        };
    }

    /// Release all nodes, compressed edge prefixes, and stored keys.
    pub fn deinit(*_) {
        if (r := _.root) {
            _.destroySubtree(r);
            _.root = null;
        }
        _.len = 0;
    }

    /// Remove all entries while keeping the tree ready for reuse.
    pub fn clear(*_) {
        _.deinit();
    }

    /// Return the number of stored key-value entries.
    pub fn count(): usize {
        return _.len;
    }

    /// Return true if the tree contains no entries.
    pub fn isEmpty(): bool {
        return _.len == 0;
    }

    /// Insert a new key-value pair. Returns `RadixError.DuplicateKey` if `key` already exists.
    pub fn insert(*_, key: []const u8, value: T): void ! RadixError {
        return _.insertOrUpsert(key, value, false);
    }

    /// Insert a new key-value pair or overwrite the value if `key` already exists.
    pub fn upsert(*_, key: []const u8, value: T): void ! RadixError {
        return _.insertOrUpsert(key, value, true);
    }

    /// Exact lookup for `key`. Returns `null` if not found.
    pub fn get(key: []const u8): T? {
        const node = _.findExactNode(key) orelse return null;
        return node.value;
    }

    /// Return true if `key` is present in the tree.
    pub fn contains(key: []const u8): bool {
        return _.findExactNode(key) != null;
    }

    /// Delete `key` from the tree and return its removed value, or `null` if absent.
    /// Automatically merges isolated non-terminal single-child nodes to keep tree height minimal.
    pub fn delete(*_, key: []const u8): T? {
        if (_.root == null) return null;
        return _.deleteRecursive(_.root.?, key);
    }

    /// Find the longest key stored in the tree that is a prefix of `key`.
    /// Useful for CIDR matching, route prefix matching, and rule engines.
    pub fn longestPrefix(key: []const u8): RadixMatch<T>? {
        if (_.root == null) return null;
        let curr = _.root.?;
        let rem = key;
        let best: RadixMatch<T>? = null;

        while (true) {
            if (curr.is_terminal) {
                best = RadixMatch<T>{
                    key: curr.full_key,
                    value: curr.value.?,
                };
            }
            if (rem.len == 0) break;

            const idx = findChildIndex(curr, rem[0]) orelse break;
            const child = curr.children.items[idx];
            if (rem.len < child.prefix.len) break;
            if (!std.mem.eql(u8, rem[0..child.prefix.len], child.prefix)) break;

            rem = rem[child.prefix.len..];
            curr = child;
        }
        return best;
    }

    /// Return all entries whose keys start with `prefix`, sorted in lexicographical order.
    /// Caller owns the returned slice and must free it with `allocator.free(entries)`.
    pub fn scanPrefix(allocator: std.mem.Allocator, prefix: []const u8): []RadixEntry<T> ! RadixError {
        let results: std.ArrayList(RadixEntry(T)) = .empty;
        errdefer results.deinit(allocator);

        if (_.root == null) {
            return results.toOwnedSlice(allocator) catch return RadixError.OutOfMemory;
        }

        let curr = _.root.?;
        let rem = prefix;
        while (rem.len > 0) {
            const idx = findChildIndex(curr, rem[0]) orelse {
                return results.toOwnedSlice(allocator) catch return RadixError.OutOfMemory;
            };
            const child = curr.children.items[idx];
            const lcp = commonPrefixLen(child.prefix, rem);
            if (lcp < rem.len) {
                if (lcp < child.prefix.len) {
                    return results.toOwnedSlice(allocator) catch return RadixError.OutOfMemory;
                }
                rem = rem[lcp..];
                curr = child;
            } else {
                curr = child;
                rem = "";
                break;
            }
        }

        collectSubtree(allocator, curr, &results)?;
        return results.toOwnedSlice(allocator) catch return RadixError.OutOfMemory;
    }

    /// Return the total number of internal and terminal nodes in the tree.
    pub fn nodeCount(): usize {
        if (_.root == null) return 0;
        return countNodes(_.root.?);
    }

    /// Verify structural invariants:
    ///   - Every child has non-empty prefix and children are strictly sorted by `prefix[0]`
    ///   - No non-root, non-terminal node has fewer than 2 children (compression invariant)
    ///   - Terminal node count equals `_.len`
    pub fn verifyInvariants(): bool {
        if (_.root == null) return _.len == 0;
        const r = _.root.?;
        if (r.prefix.len != 0) return false;
        let terminal_count: usize = 0;
        if (!verifyNode(r, true, &terminal_count)) return false;
        return terminal_count == _.len;
    }

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    fn createNode(*_, prefix: []u8, full_key: []u8, value: T?, is_terminal: bool): (*RadixNode<T>) ! RadixError {
        let node = _.alloc.create(@This().NodeType) catch return RadixError.OutOfMemory;
        node.* = RadixNode<T>{
            prefix: prefix,
            full_key: full_key,
            value: value,
            is_terminal: is_terminal,
            children: .empty,
        };
        return node;
    }

    fn destroySubtree(*_, node: *RadixNode<T>) {
        for (child of node.children.items) {
            _.destroySubtree(child);
        }
        node.children.deinit(_.alloc);
        freeBytes(_.alloc, node.prefix);
        freeBytes(_.alloc, node.full_key);
        _.alloc.destroy(node);
    }

    fn ensureRoot(*_): (*RadixNode<T>) ! RadixError {
        if (r := _.root) return r;
        _.root = _.createNode([], [], null, false)?;
        return _.root.?;
    }

    fn insertOrUpsert(*_, key: []const u8, value: T, allow_overwrite: bool): void ! RadixError {
        let curr = _.ensureRoot()?;
        let rem = key;

        while (true) {
            if (rem.len == 0) {
                if (curr.is_terminal) {
                    if (!allow_overwrite) return RadixError.DuplicateKey;
                    curr.value = value;
                    return;
                }
                const fk = dupeBytes(_.alloc, key)?;
                curr.full_key = fk;
                curr.value = value;
                curr.is_terminal = true;
                _.len += 1;
                return;
            }

            const maybe_idx = findChildIndex(curr, rem[0]);
            if (maybe_idx == null) {
                const pfx = dupeBytes(_.alloc, rem)?;
                errdefer freeBytes(_.alloc, pfx);
                const fk = dupeBytes(_.alloc, key)?;
                errdefer freeBytes(_.alloc, fk);
                const leaf = _.createNode(pfx, fk, value, true)?;
                errdefer {
                    leaf.children.deinit(_.alloc);
                    _.alloc.destroy(leaf);
                }
                insertChildSorted(_.alloc, curr, leaf)?;
                _.len += 1;
                return;
            }

            const c_idx = maybe_idx.?;
            let child = curr.children.items[c_idx];
            const lcp = commonPrefixLen(child.prefix, rem);

            if (lcp < child.prefix.len) {
                // Split `child` at `lcp`:
                //   curr -> split_node(child.prefix[0..lcp]) -> child(child.prefix[lcp..])
                const split_pfx = dupeBytes(_.alloc, child.prefix[0..lcp])?;
                errdefer freeBytes(_.alloc, split_pfx);

                const new_child_pfx = dupeBytes(_.alloc, child.prefix[lcp..])?;
                errdefer freeBytes(_.alloc, new_child_pfx);

                if (lcp == rem.len) {
                    const fk = dupeBytes(_.alloc, key)?;
                    errdefer freeBytes(_.alloc, fk);

                    const split_node = _.createNode(split_pfx, fk, value, true)?;
                    errdefer {
                        split_node.children.deinit(_.alloc);
                        _.alloc.destroy(split_node);
                    }
                    split_node.children.append(_.alloc, child) catch return RadixError.OutOfMemory;

                    // Commit mutation
                    freeBytes(_.alloc, child.prefix);
                    child.prefix = new_child_pfx;
                    curr.children.items[c_idx] = split_node;
                    _.len += 1;
                    return;
                } else {
                    const leaf_pfx = dupeBytes(_.alloc, rem[lcp..])?;
                    errdefer freeBytes(_.alloc, leaf_pfx);
                    const fk = dupeBytes(_.alloc, key)?;
                    errdefer freeBytes(_.alloc, fk);

                    const split_node = _.createNode(split_pfx, [], null, false)?;
                    errdefer {
                        split_node.children.deinit(_.alloc);
                        _.alloc.destroy(split_node);
                    }
                    const leaf = _.createNode(leaf_pfx, fk, value, true)?;
                    errdefer {
                        leaf.children.deinit(_.alloc);
                        _.alloc.destroy(leaf);
                    }

                    if (new_child_pfx[0] < leaf_pfx[0]) {
                        split_node.children.append(_.alloc, child) catch return RadixError.OutOfMemory;
                        split_node.children.append(_.alloc, leaf) catch return RadixError.OutOfMemory;
                    } else {
                        split_node.children.append(_.alloc, leaf) catch return RadixError.OutOfMemory;
                        split_node.children.append(_.alloc, child) catch return RadixError.OutOfMemory;
                    }

                    // Commit mutation
                    freeBytes(_.alloc, child.prefix);
                    child.prefix = new_child_pfx;
                    curr.children.items[c_idx] = split_node;
                    _.len += 1;
                    return;
                }
            }

            // Full edge matched: descend into child
            rem = rem[lcp..];
            curr = child;
        }
    }

    fn findExactNode(key: []const u8): (*RadixNode<T>)? {
        if (_.root == null) return null;
        let curr = _.root.?;
        let rem = key;

        while (rem.len > 0) {
            const idx = findChildIndex(curr, rem[0]) orelse return null;
            const child = curr.children.items[idx];
            if (rem.len < child.prefix.len) return null;
            if (!std.mem.eql(u8, rem[0..child.prefix.len], child.prefix)) return null;
            rem = rem[child.prefix.len..];
            curr = child;
        }

        if (curr.is_terminal) return curr;
        return null;
    }

    fn deleteRecursive(*_, curr: *RadixNode<T>, rem: []const u8): T? {
        if (rem.len == 0) {
            if (!curr.is_terminal) return null;
            const old_val = curr.value;
            freeBytes(_.alloc, curr.full_key);
            curr.full_key = [];
            curr.value = null;
            curr.is_terminal = false;
            _.len -= 1;
            return old_val;
        }

        const idx = findChildIndex(curr, rem[0]) orelse return null;
        let child = curr.children.items[idx];
        if (rem.len < child.prefix.len) return null;
        if (!std.mem.eql(u8, rem[0..child.prefix.len], child.prefix)) return null;

        const removed = _.deleteRecursive(child, rem[child.prefix.len..]);
        if (removed == null) return null;

        if (!child.is_terminal) {
            if (child.children.items.len == 0) {
                // Prune dead non-terminal leaf
                * = curr.children.orderedRemove(idx);
                child.children.deinit(_.alloc);
                freeBytes(_.alloc, child.prefix);
                freeBytes(_.alloc, child.full_key);
                _.alloc.destroy(child);
            } else if (child.children.items.len == 1) {
                // Merge isolated single-child node to preserve compressed Patricia height
                let gc = child.children.items[0];
                const merged = _.alloc.alloc(u8, child.prefix.len + gc.prefix.len) catch return removed;
                @memcpy(merged[0..child.prefix.len], child.prefix);
                @memcpy(merged[child.prefix.len..], gc.prefix);

                freeBytes(_.alloc, gc.prefix);
                gc.prefix = merged;
                curr.children.items[idx] = gc;

                child.children.deinit(_.alloc);
                freeBytes(_.alloc, child.prefix);
                freeBytes(_.alloc, child.full_key);
                _.alloc.destroy(child);
            }
        }

        return removed;
    }

    fn findChildIndex(node: *RadixNode<T>, first_byte: u8): usize? {
        const items = node.children.items;
        let lo: usize = 0;
        let hi: usize = items.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            const b = items[mid].prefix[0];
            if (b == first_byte) return mid;
            if (b < first_byte) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return null;
    }

    fn insertChildSorted(alloc: std.mem.Allocator, parent: *RadixNode<T>, child: *RadixNode<T>): void ! RadixError {
        const items = parent.children.items;
        const target = child.prefix[0];
        let lo: usize = 0;
        let hi: usize = items.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (items[mid].prefix[0] < target) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        parent.children.insert(alloc, lo, child) catch return RadixError.OutOfMemory;
    }

    fn collectSubtree(alloc: std.mem.Allocator, node: *RadixNode<T>, out: *std.ArrayList(RadixEntry(T))): void ! RadixError {
        if (node.is_terminal) {
            out.append(alloc, RadixEntry<T>{
                key: node.full_key,
                value: node.value.?,
            }) catch return RadixError.OutOfMemory;
        }
        for (child of node.children.items) {
            collectSubtree(alloc, child, out)?;
        }
    }

    fn countNodes(node: *RadixNode<T>): usize {
        let total: usize = 1;
        for (child of node.children.items) {
            total += countNodes(child);
        }
        return total;
    }

    fn verifyNode(node: *RadixNode<T>, is_root: bool, terminal_count: *usize): bool {
        if (!is_root) {
            if (node.prefix.len == 0) return false;
            if (!node.is_terminal && node.children.items.len < 2) return false;
        }
        if (node.is_terminal) {
            if (node.value == null) return false;
            terminal_count.* += 1;
        }
        const items = node.children.items;
        let i: usize = 0;
        while (i < items.len) : (i += 1) {
            if (items[i].prefix.len == 0) return false;
            if (i > 0 && items[i - 1].prefix[0] >= items[i].prefix[0]) return false;
            if (!verifyNode(items[i], false, terminal_count)) return false;
        }
        return true;
    }
}
