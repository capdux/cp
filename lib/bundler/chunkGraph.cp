// Source: Absorbed from /data/dev/bun/src/bundler/LinkerContext.cp | Synced: 2026-11-23T14:44:58Z
// ============================================================================
// lib/bundler/chunkGraph.cp — Zero-Alloc Linker Chunk Graph & Code-Splitting Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven bundler linker engine for `cap build`:
// Tree-Shaking liveness & side-effect propagation (`computeLiveModules`),
// arbitrary-entry-count external-bitset Code-Splitting (`computeCodeSplitChunks`)
// with Entry / Dynamic Async / Shared Common chunk synthesis & content-hash naming
// (`[name]-[hash8].js/.wasm/.nest`), Kahn topological parallel compile-wave
// scheduling with cycle diagnostics (`planParallelCompileWaves`), and Metafile
// JSON report rendering (`formatChunkGraphReport`).

/// Sentinel invalid module/chunk identifier.
pub const CHUNK_GRAPH_INVALID_ID: u32 = 4294967295;

/// Truncated lowercase hex hash length appended to content-hashed chunk filenames.
pub const CHUNK_GRAPH_HASH_HEX_LEN: usize = 8;

/// Chunk graph analysis, splitting, scheduling, and formatting error set.
pub err ChunkGraphError {
    EmptyModuleGraph,
    InvalidModuleId,
    NoEntryPoints,
    ScratchTooSmall,
    SliceCapacityExceeded,
    BufferTooSmall,
    BitsetCapacityExceeded,
}

/// Dependency edge kind between two modules in the bundler graph.
pub enum BundleEdgeKind: u8 {
    StaticImport = 0,
    DynamicImport = 1,
    ReExport = 2,
    SideEffectOnly = 3,
}

/// Classification of a synthesized output chunk.
pub enum SplitChunkRole: u8 {
    EntryChunk = 0,
    DynamicAsyncChunk = 1,
    SharedCommonChunk = 2,
}

/// Module node descriptor in the bundler dependency graph.
pub struct BundleModuleNode {
    id: u32 = 0,
    path: []const u8 = "",
    byte_size: usize = 0,
    has_side_effects: bool = true,
    is_entry: bool = false,
    content_hash64: u64 = 0,
}

/// Directed import/re-export edge between two modules.
pub struct BundleImportEdge {
    from_module: u32 = 0,
    to_module: u32 = 0,
    kind: BundleEdgeKind = BundleEdgeKind.StaticImport,
    is_used_export: bool = true,
}

/// Aggregate Tree-Shaking reachability and byte-savings statistics.
pub struct LiveModuleSummary {
    total_modules: u32 = 0,
    live_modules: u32 = 0,
    pruned_modules: u32 = 0,
    total_bytes: usize = 0,
    live_bytes: usize = 0,
    saved_bytes: usize = 0,
}

/// Configuration for `computeCodeSplitChunks`.
pub struct ChunkSplitOptions {
    extension: []const u8 = ".js",
    enable_splitting: bool = true,
    include_hash_in_name: bool = true,

    static fn defaults(): ChunkSplitOptions {
        return ChunkSplitOptions{};
    }
}

/// Synthesized output chunk descriptor.
pub struct ChunkGraphNode {
    chunk_id: u32 = 0,
    role: SplitChunkRole = SplitChunkRole.EntryChunk,
    primary_module_id: u32 = 0,
    module_count: u32 = 0,
    total_bytes: usize = 0,
    content_hash64: u64 = 0,
    filename: []const u8 = "",
    entry_root_count: u32 = 0,

    static fn empty(): ChunkGraphNode {
        return ChunkGraphNode{};
    }
}

/// Directed dependency between two synthesized chunks.
pub struct ChunkDepEdge {
    from_chunk: u32 = 0,
    to_chunk: u32 = 0,
    is_dynamic: bool = false,

    static fn empty(): ChunkDepEdge {
        return ChunkDepEdge{};
    }
}

/// Summary returned by `computeCodeSplitChunks`.
pub struct ChunkSplitResult {
    chunks_len: usize = 0,
    chunk_edges_len: usize = 0,
    entry_chunks: u32 = 0,
    async_chunks: u32 = 0,
    shared_chunks: u32 = 0,
    root_count: u32 = 0,
    words_per_module: usize = 0,
}

/// Single module scheduled into a parallel topological compile wave.
pub struct CompileWaveEntry {
    module_id: u32 = 0,
    wave_index: u32 = 0,
    in_cycle: bool = false,

    static fn empty(): CompileWaveEntry {
        return CompileWaveEntry{};
    }
}

/// Cycle detection diagnostic produced by `planParallelCompileWaves`.
pub struct CycleReport {
    cycle_count: u32 = 0,
    modules_in_cycles: u32 = 0,
    first_cycle_module: u32 = CHUNK_GRAPH_INVALID_ID,
}

/// Complete parallel compile wave schedule and cycle summary.
pub struct CompileWavePlan {
    scheduled_count: usize = 0,
    wave_count: u32 = 0,
    max_wave_width: u32 = 0,
    cycles: CycleReport = CycleReport{},
}

// ============================================================================
// Internal Hash, Bitset & Formatting Utilities (Zero-Alloc)
// ============================================================================

fn fnv1a64UpdateBytes(hash: u64, data: []const u8): u64 {
    let h = hash;
    let i: usize = 0;
    while (i < data.len) {
        h = h ^ @as(u64, data[i]);
        h = h *% 1099511628211;
        i += 1;
    }
    return h;
}

fn fnv1a64UpdateU64(hash: u64, val: u64): u64 {
    let h = hash;
    let shift: u6 = 0;
    let step: usize = 0;
    while (step < 8) {
        const b: u64 = (val >> shift) & 0xFF;
        h = (h ^ b) *% 1099511628211;
        if (step < 7) shift += 8;
        step += 1;
    }
    return h;
}

fn appendBytes(out_buf: []u8, pos: *usize, data: []const u8)! {
    if (pos.* + data.len > out_buf.len) return ChunkGraphError.BufferTooSmall;
    let i: usize = 0;
    while (i < data.len) {
        out_buf[pos.* + i] = data[i];
        i += 1;
    }
    pos.* += data.len;
}

fn appendByte(out_buf: []u8, pos: *usize, ch: u8)! {
    if (pos.* >= out_buf.len) return ChunkGraphError.BufferTooSmall;
    out_buf[pos.*] = ch;
    pos.* += 1;
}

fn appendUsizeDecimal(out_buf: []u8, pos: *usize, value: usize)! {
    let digits: [24]u8 = [0; 24];
    let len: usize = 0;
    let v = value;
    if (v == 0) {
        digits[0] = @as(u8, 48);
        len = 1;
    } else {
        while (v > 0) {
            digits[len] = @intCast(48 + (v % 10));
            v = v / 10;
            len += 1;
        }
    }
    if (pos.* + len > out_buf.len) return ChunkGraphError.BufferTooSmall;
    let i: usize = 0;
    while (i < len) {
        out_buf[pos.* + i] = digits[len - 1 - i];
        i += 1;
    }
    pos.* += len;
}

fn appendHex8(out_buf: []u8, pos: *usize, hash: u64)! {
    const hex: []const u8 = "0123456789abcdef";
    if (pos.* + 8 > out_buf.len) return ChunkGraphError.BufferTooSmall;
    // Emit top 32 bits as 8 lowercase hex digits
    let i: usize = 0;
    while (i < 8) {
        const shift: u6 = @intCast(60 - i * 4);
        const nibble: usize = @intCast((hash >> shift) & 0xF);
        out_buf[pos.* + i] = hex[nibble];
        i += 1;
    }
    pos.* += 8;
}

fn appendJsonEscapedString(out_buf: []u8, pos: *usize, s: []const u8)! {
    appendByte(out_buf, pos, @as(u8, 34))?; // '"'
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            appendBytes(out_buf, pos, "\\\"")?;
        } else if (c == '\\') {
            appendBytes(out_buf, pos, "\\\\")?;
        } else if (c == '\n') {
            appendBytes(out_buf, pos, "\\n")?;
        } else {
            appendByte(out_buf, pos, c)?;
        }
        i += 1;
    }
    appendByte(out_buf, pos, @as(u8, 34))?; // '"'
}

fn bitsetSet(bits: []u64, mod_idx: usize, words_per_mod: usize, root_bit: usize) {
    const word_idx = mod_idx * words_per_mod + (root_bit >> 6);
    const bit_off: u6 = @intCast(root_bit & 63);
    bits[word_idx] = bits[word_idx] | (@as(u64, 1) << bit_off);
}

fn bitsetTest(bits: []const u64, mod_idx: usize, words_per_mod: usize, root_bit: usize): bool {
    const word_idx = mod_idx * words_per_mod + (root_bit >> 6);
    const bit_off: u6 = @intCast(root_bit & 63);
    return (bits[word_idx] & (@as(u64, 1) << bit_off)) != 0;
}

fn bitsetEqual(bits: []const u64, mod_a: usize, mod_b: usize, words_per_mod: usize): bool {
    const base_a = mod_a * words_per_mod;
    const base_b = mod_b * words_per_mod;
    let w: usize = 0;
    while (w < words_per_mod) {
        if (bits[base_a + w] != bits[base_b + w]) return false;
        w += 1;
    }
    return true;
}

fn popcountU64(val: u64): u32 {
    let v = val;
    let c: u32 = 0;
    while (v != 0) {
        v = v & (v - 1);
        c += 1;
    }
    return c;
}

fn bitsetPopcount(bits: []const u64, mod_idx: usize, words_per_mod: usize): u32 {
    const base = mod_idx * words_per_mod;
    let total: u32 = 0;
    let w: usize = 0;
    while (w < words_per_mod) {
        total += popcountU64(bits[base + w]);
        w += 1;
    }
    return total;
}

fn validateEdges(modules_len: usize, edges: []const BundleImportEdge)! {
    let i: usize = 0;
    while (i < edges.len) {
        if (@as(usize, edges[i].from_module) >= modules_len || @as(usize, edges[i].to_module) >= modules_len) {
            return ChunkGraphError.InvalidModuleId;
        }
        i += 1;
    }
}

// ============================================================================
// 1. Tree-Shaking Reachability & Side-Effect Liveness (`computeLiveModules`)
// ============================================================================

/// Performs external-queue BFS from all entry modules to mark live modules.
/// Prunes unreachable modules and pure side-effect-free modules whose exports are unused.
pub fn computeLiveModules(
    modules: []const BundleModuleNode,
    edges: []const BundleImportEdge,
    live_flags_out: []bool,
    queue_scratch: []u32,
): LiveModuleSummary! {
    if (modules.len == 0) return ChunkGraphError.EmptyModuleGraph;
    if (live_flags_out.len < modules.len || queue_scratch.len < modules.len) {
        return ChunkGraphError.ScratchTooSmall;
    }
    validateEdges(modules.len, edges)?;

    let i: usize = 0;
    while (i < modules.len) {
        live_flags_out[i] = false;
        i += 1;
    }

    let q_head: usize = 0;
    let q_tail: usize = 0;
    i = 0;
    while (i < modules.len) {
        if (modules[i].is_entry) {
            live_flags_out[i] = true;
            queue_scratch[q_tail] = @intCast(i);
            q_tail += 1;
        }
        i += 1;
    }

    if (q_tail == 0) return ChunkGraphError.NoEntryPoints;

    while (q_head < q_tail) {
        const cur = queue_scratch[q_head];
        q_head += 1;

        let ei: usize = 0;
        while (ei < edges.len) {
            const e = edges[ei];
            if (e.from_module == cur) {
                const target_idx: usize = @intCast(e.to_module);
                if (!live_flags_out[target_idx]) {
                    const target_mod = modules[target_idx];
                    const keep =
                        (e.kind == BundleEdgeKind.SideEffectOnly && target_mod.has_side_effects) ||
                        (e.kind != BundleEdgeKind.SideEffectOnly && (e.is_used_export || target_mod.has_side_effects));
                    if (keep) {
                        live_flags_out[target_idx] = true;
                        queue_scratch[q_tail] = e.to_module;
                        q_tail += 1;
                    }
                }
            }
            ei += 1;
        }
    }

    let summary = LiveModuleSummary{
        total_modules: @intCast(modules.len),
    };
    i = 0;
    while (i < modules.len) {
        summary.total_bytes += modules[i].byte_size;
        if (live_flags_out[i]) {
            summary.live_modules += 1;
            summary.live_bytes += modules[i].byte_size;
        } else {
            summary.pruned_modules += 1;
            summary.saved_bytes += modules[i].byte_size;
        }
        i += 1;
    }
    return summary;
}

// ============================================================================
// 2. Content-Hashed Chunk Naming & External-Bitset Code-Splitting
// ============================================================================

fn extractBasenameNoExt(path: []const u8): []const u8 {
    let start: usize = 0;
    let i: usize = 0;
    while (i < path.len) {
        if (path[i] == '/' || path[i] == '\\') {
            start = i + 1;
        }
        i += 1;
    }
    const base = path[start..];
    let end = base.len;
    let j = base.len;
    while (j > 0) {
        j -= 1;
        if (base[j] == '.') {
            if (j > 0) end = j;
            break;
        }
    }
    return if (end > 0) base[0..end] else "chunk";
}

/// Formats a deterministic content-hashed chunk filename such as
/// `app-1a2b3c4d.js`, `chunk-deadbeef.wasm`, or `lazy-89abcdef.nest`.
pub fn formatChunkFilename(
    primary_path: []const u8,
    role: SplitChunkRole,
    content_hash64: u64,
    extension: []const u8,
    include_hash: bool,
    out_buf: []u8,
): usize! {
    let pos: usize = 0;
    const stem = if (role == SplitChunkRole.SharedCommonChunk) "chunk" else extractBasenameNoExt(primary_path);
    appendBytes(out_buf, &pos, stem)?;
    if (include_hash) {
        appendByte(out_buf, &pos, @as(u8, 45))?; // '-'
        appendHex8(out_buf, &pos, content_hash64)?;
    }
    appendBytes(out_buf, &pos, extension)?;
    return pos;
}

fn findOrAddSplitRoot(roots: []u32, root_is_static: []bool, root_count: *usize, mod_id: u32, is_static: bool)! {
    let i: usize = 0;
    while (i < root_count.*) {
        if (roots[i] == mod_id) {
            if (is_static) root_is_static[i] = true;
            return;
        }
        i += 1;
    }
    if (root_count.* >= roots.len) return ChunkGraphError.SliceCapacityExceeded;
    roots[root_count.*] = mod_id;
    root_is_static[root_count.*] = is_static;
    root_count.* += 1;
}

fn hasChunkDepEdge(edges: []const ChunkDepEdge, len: usize, from_c: u32, to_c: u32, is_dyn: bool): bool {
    let i: usize = 0;
    while (i < len) {
        if (edges[i].from_chunk == from_c && edges[i].to_chunk == to_c && edges[i].is_dynamic == is_dyn) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Computes Code-Splitting chunks using Bun `LinkerContext`'s Entry-Bitset algorithm,
/// backed by an arbitrary-width external `bitset_scratch` (supports >64 entries).
/// Groups live modules with identical reachability bitsets into Entry, Dynamic Async,
/// or Shared Common chunks, computes content hashes & filenames, and builds the
/// cross-chunk dependency graph.
pub fn computeCodeSplitChunks(
    modules: []const BundleModuleNode,
    edges: []const BundleImportEdge,
    live_flags: []const bool,
    opts: ChunkSplitOptions,
    bitset_scratch: []u64,
    queue_scratch: []u32,
    mod_to_chunk_out: []u32,
    chunks_out: []ChunkGraphNode,
    chunk_edges_out: []ChunkDepEdge,
    name_arena: []u8,
    arena_pos: *usize,
): ChunkSplitResult! {
    if (modules.len == 0) return ChunkGraphError.EmptyModuleGraph;
    if (live_flags.len < modules.len || queue_scratch.len < modules.len || mod_to_chunk_out.len < modules.len) {
        return ChunkGraphError.ScratchTooSmall;
    }
    validateEdges(modules.len, edges)?;

    // 1. Collect split roots: all static entries + all live DynamicImport targets
    let roots_buf: [256]u32 = [0; 256];
    let root_static_buf: [256]bool = [false; 256];
    let root_count: usize = 0;

    let mi: usize = 0;
    while (mi < modules.len) {
        if (live_flags[mi] && modules[mi].is_entry) {
            findOrAddSplitRoot(roots_buf[0..], root_static_buf[0..], &root_count, @intCast(mi), true)?;
        }
        mi += 1;
    }
    if (root_count == 0) return ChunkGraphError.NoEntryPoints;

    if (opts.enable_splitting) {
        let ei: usize = 0;
        while (ei < edges.len) {
            const e = edges[ei];
            if (e.kind == BundleEdgeKind.DynamicImport &&
                live_flags[@as(usize, e.from_module)] &&
                live_flags[@as(usize, e.to_module)])
            {
                findOrAddSplitRoot(roots_buf[0..], root_static_buf[0..], &root_count, e.to_module, false)?;
            }
            ei += 1;
        }
    }

    // 2. Allocate external bitset words: words_per_mod = ceil(root_count / 64)
    const words_per_mod: usize = (root_count + 63) / 64;
    const total_words = modules.len * words_per_mod;
    if (total_words > bitset_scratch.len) {
        return ChunkGraphError.BitsetCapacityExceeded;
    }
    let wi: usize = 0;
    while (wi < total_words) {
        bitset_scratch[wi] = 0;
        wi += 1;
    }

    // 3. Propagate each split root's bit along non-dynamic edges (or all edges if splitting disabled)
    let r: usize = 0;
    while (r < root_count) {
        const root_mod = roots_buf[r];
        let q_head: usize = 0;
        let q_tail: usize = 0;
        bitsetSet(bitset_scratch, @as(usize, root_mod), words_per_mod, r);
        queue_scratch[q_tail] = root_mod;
        q_tail += 1;

        while (q_head < q_tail) {
            const cur = queue_scratch[q_head];
            q_head += 1;

            let ei: usize = 0;
            while (ei < edges.len) {
                const e = edges[ei];
                if (e.from_module == cur) {
                    const cross_dynamic = (e.kind == BundleEdgeKind.DynamicImport) && opts.enable_splitting;
                    if (!cross_dynamic) {
                        const target_idx: usize = @intCast(e.to_module);
                        if (live_flags[target_idx] && !bitsetTest(bitset_scratch, target_idx, words_per_mod, r)) {
                            bitsetSet(bitset_scratch, target_idx, words_per_mod, r);
                            queue_scratch[q_tail] = e.to_module;
                            q_tail += 1;
                        }
                    }
                }
                ei += 1;
            }
        }
        r += 1;
    }

    // 4. Partition live modules with identical entry-bitsets into unique chunks
    mi = 0;
    while (mi < modules.len) {
        mod_to_chunk_out[mi] = CHUNK_GRAPH_INVALID_ID;
        mi += 1;
    }

    let chunks_len: usize = 0;
    let res = ChunkSplitResult{
        root_count: @intCast(root_count),
        words_per_module: words_per_mod,
    };

    mi = 0;
    while (mi < modules.len) {
        if (!live_flags[mi]) {
            mi += 1;
            continue;
        }
        const bits_count = bitsetPopcount(bitset_scratch, mi, words_per_mod);
        if (bits_count == 0) {
            mi += 1;
            continue;
        }

        // Check if an existing chunk already has this exact entry-bitset
        let matched_chunk: usize? = null;
        let ci: usize = 0;
        while (ci < chunks_len) {
            const rep_mod: usize = @intCast(chunks_out[ci].primary_module_id);
            if (bitsetEqual(bitset_scratch, mi, rep_mod, words_per_mod)) {
                matched_chunk = ci;
                break;
            }
            ci += 1;
        }

        if (cid := matched_chunk) {
            mod_to_chunk_out[mi] = @intCast(cid);
            let ch = chunks_out[cid];
            ch.module_count += 1;
            ch.total_bytes += modules[mi].byte_size;
            ch.content_hash64 = fnv1a64UpdateU64(ch.content_hash64, modules[mi].content_hash64);
            ch.content_hash64 = fnv1a64UpdateBytes(ch.content_hash64, modules[mi].path);
            // Prefer an actual entry/root module as primary_module_id for clean chunk naming
            if (modules[mi].is_entry && !modules[@as(usize, ch.primary_module_id)].is_entry) {
                ch.primary_module_id = @intCast(mi);
            }
            chunks_out[cid] = ch;
        } else {
            if (chunks_len >= chunks_out.len) return ChunkGraphError.SliceCapacityExceeded;

            // Classify chunk role from bitset
            let role = SplitChunkRole.SharedCommonChunk;
            if (bits_count == 1) {
                // Find the single set root bit
                let single_root: usize = 0;
                let rk: usize = 0;
                while (rk < root_count) {
                    if (bitsetTest(bitset_scratch, mi, words_per_mod, rk)) {
                        single_root = rk;
                        break;
                    }
                    rk += 1;
                }
                role = if (root_static_buf[single_root]) SplitChunkRole.EntryChunk else SplitChunkRole.DynamicAsyncChunk;
            }

            let h: u64 = 14695981039346656037;
            h = fnv1a64UpdateU64(h, modules[mi].content_hash64);
            h = fnv1a64UpdateBytes(h, modules[mi].path);

            chunks_out[chunks_len] = ChunkGraphNode{
                chunk_id: @intCast(chunks_len),
                role: role,
                primary_module_id: @intCast(mi),
                module_count: 1,
                total_bytes: modules[mi].byte_size,
                content_hash64: h,
                filename: "",
                entry_root_count: bits_count,
            };
            mod_to_chunk_out[mi] = @intCast(chunks_len);

            if (role == SplitChunkRole.EntryChunk) res.entry_chunks += 1;
            else if (role == SplitChunkRole.DynamicAsyncChunk) res.async_chunks += 1;
            else res.shared_chunks += 1;

            chunks_len += 1;
        }
        mi += 1;
    }

    // 5. Finalize deterministic content-hashed filenames for each chunk
    let ci: usize = 0;
    while (ci < chunks_len) {
        let ch = chunks_out[ci];
        const fname_start = arena_pos.*;
        const primary_path = modules[@as(usize, ch.primary_module_id)].path;
        const flen = formatChunkFilename(
            primary_path,
            ch.role,
            ch.content_hash64,
            opts.extension,
            opts.include_hash_in_name,
            name_arena[fname_start..],
        )?;
        arena_pos.* += flen;
        ch.filename = name_arena[fname_start .. fname_start + flen];
        chunks_out[ci] = ch;
        ci += 1;
    }

    // 6. Build deduplicated cross-chunk dependency graph
    let chunk_edges_len: usize = 0;
    let ei: usize = 0;
    while (ei < edges.len) {
        const e = edges[ei];
        const from_c = mod_to_chunk_out[@as(usize, e.from_module)];
        const to_c = mod_to_chunk_out[@as(usize, e.to_module)];
        if (from_c != CHUNK_GRAPH_INVALID_ID && to_c != CHUNK_GRAPH_INVALID_ID && from_c != to_c) {
            const is_dyn = (e.kind == BundleEdgeKind.DynamicImport);
            if (!hasChunkDepEdge(chunk_edges_out, chunk_edges_len, from_c, to_c, is_dyn)) {
                if (chunk_edges_len >= chunk_edges_out.len) return ChunkGraphError.SliceCapacityExceeded;
                chunk_edges_out[chunk_edges_len] = ChunkDepEdge{
                    from_chunk: from_c,
                    to_chunk: to_c,
                    is_dynamic: is_dyn,
                };
                chunk_edges_len += 1;
            }
        }
        ei += 1;
    }

    res.chunks_len = chunks_len;
    res.chunk_edges_len = chunk_edges_len;
    return res;
}

// ============================================================================
// 3. Cycle Detection & Parallel Topological Compile Waves (`planParallelCompileWaves`)
// ============================================================================

/// Schedules live modules into parallel compile waves using layered Kahn topological
/// sorting (dependencies compile before dependents). Any modules trapped in import
/// cycles are detected, reported in `CycleReport`, and placed in a final cycle wave.
pub fn planParallelCompileWaves(
    modules: []const BundleModuleNode,
    edges: []const BundleImportEdge,
    live_flags: []const bool,
    indegree_scratch: []u32,
    queue_scratch: []u32,
    waves_out: []CompileWaveEntry,
): CompileWavePlan! {
    if (modules.len == 0) return ChunkGraphError.EmptyModuleGraph;
    if (live_flags.len < modules.len || indegree_scratch.len < modules.len || queue_scratch.len < modules.len) {
        return ChunkGraphError.ScratchTooSmall;
    }
    validateEdges(modules.len, edges)?;

    // In compile order, if A imports B (`from=A, to=B`), B must compile before A -> indegree[A]++
    let live_total: usize = 0;
    let i: usize = 0;
    while (i < modules.len) {
        indegree_scratch[i] = 0;
        if (live_flags[i]) live_total += 1;
        i += 1;
    }
    if (live_total > waves_out.len) return ChunkGraphError.SliceCapacityExceeded;

    let ei: usize = 0;
    while (ei < edges.len) {
        const e = edges[ei];
        const from_i: usize = @intCast(e.from_module);
        const to_i: usize = @intCast(e.to_module);
        if (live_flags[from_i] && live_flags[to_i] && from_i != to_i) {
            indegree_scratch[from_i] += 1;
        }
        ei += 1;
    }

    let q_head: usize = 0;
    let q_tail: usize = 0;
    i = 0;
    while (i < modules.len) {
        if (live_flags[i] && indegree_scratch[i] == 0) {
            queue_scratch[q_tail] = @intCast(i);
            q_tail += 1;
        }
        i += 1;
    }

    let plan = CompileWavePlan{};
    let scheduled: usize = 0;
    let cur_wave: u32 = 0;

    while (q_head < q_tail) {
        const wave_end = q_tail;
        const width: u32 = @intCast(wave_end - q_head);
        if (width > plan.max_wave_width) plan.max_wave_width = width;

        while (q_head < wave_end) {
            const mod_id = queue_scratch[q_head];
            q_head += 1;

            waves_out[scheduled] = CompileWaveEntry{
                module_id: mod_id,
                wave_index: cur_wave,
                in_cycle: false,
            };
            scheduled += 1;

            // Decrement indegree of all live dependents (`from` depends on `mod_id`)
            ei = 0;
            while (ei < edges.len) {
                const e = edges[ei];
                if (e.to_module == mod_id) {
                    const dep_i: usize = @intCast(e.from_module);
                    if (live_flags[dep_i] && dep_i != @as(usize, mod_id) && indegree_scratch[dep_i] > 0) {
                        indegree_scratch[dep_i] -= 1;
                        if (indegree_scratch[dep_i] == 0) {
                            queue_scratch[q_tail] = e.from_module;
                            q_tail += 1;
                        }
                    }
                }
                ei += 1;
            }
        }
        cur_wave += 1;
    }

    // Any remaining live modules with indegree > 0 are part of or blocked by cycles
    if (scheduled < live_total) {
        let cycle_width: u32 = 0;
        i = 0;
        while (i < modules.len) {
            if (live_flags[i] && indegree_scratch[i] > 0) {
                if (plan.cycles.first_cycle_module == CHUNK_GRAPH_INVALID_ID) {
                    plan.cycles.first_cycle_module = @intCast(i);
                }
                waves_out[scheduled] = CompileWaveEntry{
                    module_id: @intCast(i),
                    wave_index: cur_wave,
                    in_cycle: true,
                };
                scheduled += 1;
                cycle_width += 1;
            }
            i += 1;
        }
        plan.cycles.modules_in_cycles = cycle_width;
        plan.cycles.cycle_count = if (cycle_width > 0) 1 else 0;
        if (cycle_width > plan.max_wave_width) plan.max_wave_width = cycle_width;
        cur_wave += 1;
    }

    plan.scheduled_count = scheduled;
    plan.wave_count = cur_wave;
    return plan;
}

// ============================================================================
// 4. Metafile JSON Summary Renderer (`formatChunkGraphReport`)
// ============================================================================

/// Renders a structured JSON Metafile summary of Tree-Shaking, Code-Splitting chunks,
/// cross-chunk edges, and parallel compile waves into `out_buf`.
pub fn formatChunkGraphReport(
    live_summary: LiveModuleSummary,
    split_res: ChunkSplitResult,
    chunks: []const ChunkGraphNode,
    chunk_edges: []const ChunkDepEdge,
    wave_plan: CompileWavePlan,
    out_buf: []u8,
): usize! {
    let pos: usize = 0;
    appendBytes(out_buf, &pos, "{\"modules\":{\"total\":")?;
    appendUsizeDecimal(out_buf, &pos, @intCast(live_summary.total_modules))?;
    appendBytes(out_buf, &pos, ",\"live\":")?;
    appendUsizeDecimal(out_buf, &pos, @intCast(live_summary.live_modules))?;
    appendBytes(out_buf, &pos, ",\"pruned\":")?;
    appendUsizeDecimal(out_buf, &pos, @intCast(live_summary.pruned_modules))?;
    appendBytes(out_buf, &pos, ",\"savedBytes\":")?;
    appendUsizeDecimal(out_buf, &pos, live_summary.saved_bytes)?;
    appendBytes(out_buf, &pos, "},\"waves\":{\"count\":")?;
    appendUsizeDecimal(out_buf, &pos, @intCast(wave_plan.wave_count))?;
    appendBytes(out_buf, &pos, ",\"maxWidth\":")?;
    appendUsizeDecimal(out_buf, &pos, @intCast(wave_plan.max_wave_width))?;
    appendBytes(out_buf, &pos, ",\"modulesInCycles\":")?;
    appendUsizeDecimal(out_buf, &pos, @intCast(wave_plan.cycles.modules_in_cycles))?;
    appendBytes(out_buf, &pos, "},\"chunks\":[")?;

    let ci: usize = 0;
    while (ci < split_res.chunks_len && ci < chunks.len) {
        if (ci > 0) appendBytes(out_buf, &pos, ",")?;
        const ch = chunks[ci];
        const role_str: []const u8 = if (ch.role == SplitChunkRole.EntryChunk)
            "entry"
        else if (ch.role == SplitChunkRole.DynamicAsyncChunk)
            "async"
        else
            "shared";
        appendBytes(out_buf, &pos, "{\"file\":")?;
        appendJsonEscapedString(out_buf, &pos, ch.filename)?;
        appendBytes(out_buf, &pos, ",\"role\":")?;
        appendJsonEscapedString(out_buf, &pos, role_str)?;
        appendBytes(out_buf, &pos, ",\"modules\":")?;
        appendUsizeDecimal(out_buf, &pos, @intCast(ch.module_count))?;
        appendBytes(out_buf, &pos, ",\"bytes\":")?;
        appendUsizeDecimal(out_buf, &pos, ch.total_bytes)?;
        appendBytes(out_buf, &pos, ",\"imports\":[")?;

        let imp_count: usize = 0;
        let ei: usize = 0;
        while (ei < split_res.chunk_edges_len && ei < chunk_edges.len) {
            if (chunk_edges[ei].from_chunk == ch.chunk_id) {
                if (imp_count > 0) appendBytes(out_buf, &pos, ",")?;
                const target_c: usize = @intCast(chunk_edges[ei].to_chunk);
                appendBytes(out_buf, &pos, "{\"path\":")?;
                appendJsonEscapedString(out_buf, &pos, chunks[target_c].filename)?;
                appendBytes(out_buf, &pos, ",\"dynamic\":")?;
                appendBytes(out_buf, &pos, if (chunk_edges[ei].is_dynamic) "true" else "false")?;
                appendBytes(out_buf, &pos, "}")?;
                imp_count += 1;
            }
            ei += 1;
        }
        appendBytes(out_buf, &pos, "]}")?;
        ci += 1;
    }

    appendBytes(out_buf, &pos, "]}")?;
    return pos;
}
