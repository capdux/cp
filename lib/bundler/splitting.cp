// Source: Absorbed from /data/dev/bun/src/bundler/LinkerGraph.cp | Synced: 2026-11-22T13:34:57Z
// lib/bundler/splitting.cp
// Multi-entry CodeSplitter using u64 reachability bitmasks, shared chunk extraction, content hashing & AssetManifest

const std = @import("std");
import { ChunkKind, ChunkOrder, INVALID_MODULE_INDEX, ModuleGraph } from "./bundle.cp";

pub err SplittingError {
    OutOfMemory,
    TooManyEntryPoints,
}

pub struct SplitChunk {
    pub id: usize = 0,
    pub name: []u8 = [],
    pub kind: ChunkKind = .EntryPoint,
    pub entry_mask: u64 = 0,
    pub content_hash: [8]u8 = [0; 8],
    pub module_ids: []u32 = [],
    pub total_bytes: usize = 0,
    pub imports_chunks: []usize = [],
}

pub struct AssetManifestEntry {
    pub logical_path: []const u8 = "",
    pub output_path: []u8 = [],
    pub content_hash: [8]u8 = [0; 8],
    pub size_bytes: usize = 0,
    pub is_entry: bool = false,
}

pub struct AssetManifest {
    pub entries: []AssetManifestEntry = [],
    allocator: std.mem.Allocator = std.heap.page_allocator,

    pub fn findByLogical(logical_path: []const u8): AssetManifestEntry? {
        for (e of _.entries) {
            if (std.mem.eql(u8, e.logical_path, logical_path)) {
                return e;
            }
        }
        return null;
    }

    pub fn count(): usize {
        return _.entries.len;
    }

    pub fn toJsonAlloc(alloc: std.mem.Allocator): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        let num_buf: [32]u8 = undefined;

        out.appendSlice(alloc, "{\n") catch return SplittingError.OutOfMemory;
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            const e = _.entries[i];
            if (i > 0) out.appendSlice(alloc, ",\n") catch return SplittingError.OutOfMemory;
            out.appendSlice(alloc, "  \"") catch return SplittingError.OutOfMemory;
            out.appendSlice(alloc, e.logical_path) catch return SplittingError.OutOfMemory;
            out.appendSlice(alloc, "\": { \"output\": \"") catch return SplittingError.OutOfMemory;
            out.appendSlice(alloc, e.output_path) catch return SplittingError.OutOfMemory;
            out.appendSlice(alloc, "\", \"hash\": \"") catch return SplittingError.OutOfMemory;
            out.appendSlice(alloc, e.content_hash[0..8]) catch return SplittingError.OutOfMemory;
            out.appendSlice(alloc, "\", \"bytes\": ") catch return SplittingError.OutOfMemory;
            const bstr = std.fmt.bufPrint(&num_buf, "{d}", .{e.size_bytes}) catch "0";
            out.appendSlice(alloc, bstr) catch return SplittingError.OutOfMemory;
            out.appendSlice(alloc, ", \"isEntry\": ") catch return SplittingError.OutOfMemory;
            out.appendSlice(alloc, if (e.is_entry) "true" else "false") catch return SplittingError.OutOfMemory;
            out.appendSlice(alloc, " }") catch return SplittingError.OutOfMemory;
        }
        out.appendSlice(alloc, "\n}\n") catch return SplittingError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return SplittingError.OutOfMemory;
    }
}

pub struct SplitResult {
    pub chunks: []SplitChunk = [],
    pub manifest: AssetManifest = AssetManifest{},
    pub entry_chunk_count: usize = 0,
    pub shared_chunk_count: usize = 0,
    pub dead_module_count: usize = 0,
    allocator: std.mem.Allocator = std.heap.page_allocator,

    pub fn deinit(*_) {
        for (ch of _.chunks) {
            if (ch.name.len > 0) _.allocator.free(ch.name);
            if (ch.module_ids.len > 0) _.allocator.free(ch.module_ids);
            if (ch.imports_chunks.len > 0) _.allocator.free(ch.imports_chunks);
        }
        if (_.chunks.len > 0) _.allocator.free(_.chunks);
        for (me of _.manifest.entries) {
            if (me.output_path.len > 0) _.allocator.free(me.output_path);
        }
        if (_.manifest.entries.len > 0) _.allocator.free(_.manifest.entries);
        _.chunks = [];
        _.manifest.entries = [];
    }

    pub fn findChunk(name: []const u8): SplitChunk? {
        for (ch of _.chunks) {
            if (std.mem.eql(u8, ch.name, name)) {
                return ch;
            }
        }
        return null;
    }

    pub fn findSharedChunks(alloc: std.mem.Allocator): ([]SplitChunk)! {
        let out: std.ArrayList(SplitChunk) = .empty;
        errdefer out.deinit(alloc);
        for (ch of _.chunks) {
            if (ch.kind == .SharedChunk) {
                out.append(alloc, ch) catch return SplittingError.OutOfMemory;
            }
        }
        return out.toOwnedSlice(alloc) catch return SplittingError.OutOfMemory;
    }
}

pub struct CodeSplitter {
    /// Computes deterministic 8-char lowercase hex FNV-1a/mix content hash.
    pub static fn contentHashHex(data: []const u8, out_buf: []u8): []const u8 {
        const h = CodeSplitter.hashBytes64(data);
        return CodeSplitter.u32ToHex8((h ^ (h >> 32)) as u32, out_buf);
    }

    static fn hashBytes64(data: []const u8): u64 {
        zig {
            var h: u64 = 0xcbf29ce484222325;
            for (data) |b| {
                h ^= @as(u64, b);
                h *%= 0x100000001b3;
            }
            return h;
        }
    }

    static fn u32ToHex8(val: u32, out_buf: []u8): []const u8 {
        const hex_digits: []const u8 = "0123456789abcdef";
        if (out_buf.len < 8) return "";
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            const shift: u5 = @as(u5, @intCast((7 - i) * 4));
            out_buf[i] = hex_digits[((val >> shift) & 0xF) as usize];
        }
        return out_buf[0..8];
    }

    static fn pathStem(path: []const u8): []const u8 {
        let start: usize = 0;
        let i: usize = 0;
        while (i < path.len) : (i += 1) {
            if (path[i] == '/' || path[i] == '\\') {
                start = i + 1;
            }
        }
        let end: usize = path.len;
        let k: usize = path.len;
        while (k > start) {
            k -= 1;
            if (path[k] == '.') {
                end = k;
                break;
            }
        }
        return if (end > start) path[start..end] else "entry";
    }

    static fn dfsMarkEntry(
        graph: *const ModuleGraph,
        node_idx: usize,
        entry_bit: u64,
        depth: u32,
        masks: []u64,
        min_dist: []u32,
        visited: []bool,
    ) {
        if (node_idx >= graph.nodes.items.len || visited[node_idx]) {
            return;
        }
        visited[node_idx] = true;
        masks[node_idx] |= entry_bit;
        if (depth < min_dist[node_idx]) {
            min_dist[node_idx] = depth;
        }
        for (imp of graph.nodes.items[node_idx].imports.items) {
            if (imp.is_external || imp.resolved_index == INVALID_MODULE_INDEX) {
                continue;
            }
            const dep = imp.resolved_index as usize;
            if (dep < graph.nodes.items.len) {
                CodeSplitter.dfsMarkEntry(graph, dep, entry_bit, depth + 1, masks, min_dist, visited);
            }
        }
    }

    /// Splits a multi-entry `ModuleGraph` into entry chunks & extracted shared `chunk-<hash>.<ext>` chunks.
    pub static fn split(
        alloc: std.mem.Allocator,
        graph: *const ModuleGraph,
        ext: []const u8 = "js",
    ): SplitResult! {
        const n = graph.nodes.items.len;
        if (n == 0) {
            return SplitResult{ allocator: alloc };
        }

        let entry_ids: std.ArrayList(usize) = .empty;
        defer entry_ids.deinit(alloc);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (graph.nodes.items[i].is_entry) {
                if (entry_ids.items.len >= 64) {
                    return SplittingError.TooManyEntryPoints;
                }
                entry_ids.append(alloc, i) catch return SplittingError.OutOfMemory;
            }
        }

        let masks = alloc.alloc(u64, n) catch return SplittingError.OutOfMemory;
        defer alloc.free(masks);
        @memset(masks, 0);

        let min_dist = alloc.alloc(u32, n) catch return SplittingError.OutOfMemory;
        defer alloc.free(min_dist);
        @memset(min_dist, std.math.maxInt(u32));

        let visited = alloc.alloc(bool, n) catch return SplittingError.OutOfMemory;
        defer alloc.free(visited);

        let e_idx: usize = 0;
        while (e_idx < entry_ids.items.len) : (e_idx += 1) {
            @memset(visited, false);
            let bit: u64 = 1;
            zig {
                bit = @as(u64, 1) << @as(u6, @intCast(e_idx));
            }
            CodeSplitter.dfsMarkEntry(graph, entry_ids.items[e_idx], bit, 0, masks, min_dist, visited);
        }

        // Collect unique non-zero entry masks in deterministic discovery order
        let unique_masks: std.ArrayList(u64) = .empty;
        defer unique_masks.deinit(alloc);
        let dead_count: usize = 0;
        i = 0;
        while (i < n) : (i += 1) {
            const m = masks[i];
            if (m == 0) {
                dead_count += 1;
                continue;
            }
            let seen = false;
            for (um of unique_masks.items) {
                if (um == m) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                unique_masks.append(alloc, m) catch return SplittingError.OutOfMemory;
            }
        }

        // Map each module to its assigned chunk index
        let mod_to_chunk = alloc.alloc(usize, n) catch return SplittingError.OutOfMemory;
        defer alloc.free(mod_to_chunk);
        @memset(mod_to_chunk, std.math.maxInt(usize));

        let chunks_list: std.ArrayList(SplitChunk) = .empty;
        errdefer chunks_list.deinit(alloc);

        let entry_chunks: usize = 0;
        let shared_chunks: usize = 0;

        let cm_idx: usize = 0;
        while (cm_idx < unique_masks.items.len) : (cm_idx += 1) {
            const mask = unique_masks.items[cm_idx];
            let orders: std.ArrayList(ChunkOrder) = .empty;
            defer orders.deinit(alloc);

            let entry_node_id: usize? = null;
            let total_bytes: usize = 0;
            let hash_acc: u64 = 0;
            zig {
                hash_acc = 0xcbf29ce484222325;
            }

            i = 0;
            while (i < n) : (i += 1) {
                if (masks[i] == mask) {
                    const node = graph.nodes.items[i];
                    if (node.is_entry && entry_node_id == null) {
                        entry_node_id = i as usize;
                    }
                    orders.append(alloc, ChunkOrder{
                        source_index: node.id,
                        distance: min_dist[i],
                        tie_breaker: node.id,
                    }) catch return SplittingError.OutOfMemory;
                    total_bytes += node.size_bytes;
                    hash_acc ^= CodeSplitter.hashBytes64(node.path);
                    hash_acc = hash_acc *% 0x100000001b3;
                    hash_acc ^= node.size_bytes as u64;
                    hash_acc = hash_acc *% 0x100000001b3;
                }
            }

            ChunkOrder.sort(orders.items);

            let mod_ids = alloc.alloc(u32, orders.items.len) catch return SplittingError.OutOfMemory;
            let oi: usize = 0;
            while (oi < orders.items.len) : (oi += 1) {
                const mid = orders.items[oi].source_index;
                mod_ids[oi] = mid;
                mod_to_chunk[mid as usize] = cm_idx;
            }

            let hex8: [8]u8 = undefined;
            * = CodeSplitter.u32ToHex8((hash_acc ^ (hash_acc >> 32)) as u32, &hex8);

            const bit_pop = @popCount(mask);
            const kind: ChunkKind = if (bit_pop == 1 && entry_node_id != null) .EntryPoint else .SharedChunk;
            if (kind == .EntryPoint) {
                entry_chunks += 1;
            } else {
                shared_chunks += 1;
            }

            const chunk_name = if (kind == .EntryPoint)
                std.fmt.allocPrint(alloc, "{s}-{s}.{s}", .{ CodeSplitter.pathStem(graph.nodes.items[entry_node_id.?].path), hex8[0..8], ext }) catch return SplittingError.OutOfMemory
            else
                std.fmt.allocPrint(alloc, "chunk-{s}.{s}", .{ hex8[0..8], ext }) catch return SplittingError.OutOfMemory;

            chunks_list.append(alloc, SplitChunk{
                id: cm_idx,
                name: chunk_name,
                kind: kind,
                entry_mask: mask,
                content_hash: hex8,
                module_ids: mod_ids,
                total_bytes: total_bytes,
                imports_chunks: [],
            }) catch return SplittingError.OutOfMemory;
        }

        // Compute deterministic cross-chunk imports for every chunk
        let ci: usize = 0;
        while (ci < chunks_list.items.len) : (ci += 1) {
            let deps_list: std.ArrayList(usize) = .empty;
            errdefer deps_list.deinit(alloc);
            for (mid of chunks_list.items[ci].module_ids) {
                for (imp of graph.nodes.items[mid as usize].imports.items) {
                    if (imp.is_external || imp.resolved_index == INVALID_MODULE_INDEX) continue;
                    const dep_chunk = mod_to_chunk[imp.resolved_index as usize];
                    if (dep_chunk == std.math.maxInt(usize) || dep_chunk == ci) continue;
                    let already = false;
                    for (existing_dep of deps_list.items) {
                        if (existing_dep == dep_chunk) {
                            already = true;
                            break;
                        }
                    }
                    if (!already) {
                        deps_list.append(alloc, dep_chunk) catch return SplittingError.OutOfMemory;
                    }
                }
            }
            chunks_list.items[ci].imports_chunks = deps_list.toOwnedSlice(alloc) catch return SplittingError.OutOfMemory;
        }

        // Build AssetManifest mapping each reachable module / entry to its output chunk
        let manifest_list: std.ArrayList(AssetManifestEntry) = .empty;
        errdefer manifest_list.deinit(alloc);
        i = 0;
        while (i < n) : (i += 1) {
            const ch_idx = mod_to_chunk[i];
            if (ch_idx == std.math.maxInt(usize)) continue;
            const ch = chunks_list.items[ch_idx];
            const node = graph.nodes.items[i];
            const out_copy = alloc.dupe(u8, ch.name) catch return SplittingError.OutOfMemory;
            manifest_list.append(alloc, AssetManifestEntry{
                logical_path: node.path,
                output_path: out_copy,
                content_hash: ch.content_hash,
                size_bytes: ch.total_bytes,
                is_entry: node.is_entry,
            }) catch return SplittingError.OutOfMemory;
        }

        return SplitResult{
            chunks: chunks_list.toOwnedSlice(alloc) catch return SplittingError.OutOfMemory,
            manifest: AssetManifest{
                entries: manifest_list.toOwnedSlice(alloc) catch return SplittingError.OutOfMemory,
                allocator: alloc,
            },
            entry_chunk_count: entry_chunks,
            shared_chunk_count: shared_chunks,
            dead_module_count: dead_count,
            allocator: alloc,
        };
    }
}
