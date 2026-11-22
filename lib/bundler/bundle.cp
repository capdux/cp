// Source: Absorbed from /data/dev/bun/src/bundler/ | Synced: 2026-11-22T12:49:17Z
// lib/bundler/bundle.cp
// High-performance Bundler configuration, Chunk topology ordering & BundleV2 pipeline state
// Absorbed from Bun src/bundler/bundle_v2.cp & src/bundler/Chunk.cp

const std = @import("std");

pub enum BundleTarget: u8 {
    Browser = 0,
    Node = 1,
    Bun = 2,
    CpNative = 3,
}

pub enum OutputFormat: u8 {
    Esm = 0,
    Cjs = 1,
    Iife = 2,
}

pub enum ChunkKind: u8 {
    EntryPoint = 0,
    SharedChunk = 1,
    Asset = 2,
}

/// Deterministic chunk ordering record (absorbed from Bun src/bundler/Chunk.cp)
/// Orders modules closest to an entry point first, breaking ties by DFS source index.
pub struct ChunkOrder {
    source_index: u32 = 0;
    distance: u32 = 0;
    tie_breaker: u32 = 0;

    pub static fn compare(a: ChunkOrder, b: ChunkOrder): bool {
        return (a.distance < b.distance) ||
            (a.distance == b.distance && a.tie_breaker < b.tie_breaker);
    }

    pub static fn sort(items: []ChunkOrder) {
        if (items.len < 2) {
            return;
        }
        let i: usize = 1;
        while (i < items.len) : (i += 1) {
            const key = items[i];
            let j: usize = i;
            while (j > 0 && ChunkOrder.compare(key, items[j - 1])) {
                items[j] = items[j - 1];
                j -= 1;
            }
            items[j] = key;
        }
    }
}

pub struct ChunkEntry {
    name: []const u8 = "";
    kind: ChunkKind = ChunkKind.EntryPoint;
    source_index: u32 = 0;
    size_bytes: usize = 0;
    is_executable: bool = false;
}

pub struct BundleConfig {
    target: BundleTarget = BundleTarget.CpNative;
    format: OutputFormat = OutputFormat.Esm;
    minify: bool = false;
    sourcemap: bool = true;
    splitting: bool = false;
    out_dir: []const u8 = "dist";

    pub static fn defaultConfig(): BundleConfig {
        return BundleConfig {
            target: BundleTarget.CpNative,
            format: OutputFormat.Esm,
            minify: false,
            sourcemap: true,
            splitting: false,
            out_dir: "dist",
        };
    }

    pub static fn productionBrowser(out_dir: []const u8): BundleConfig {
        return BundleConfig {
            target: BundleTarget.Browser,
            format: OutputFormat.Esm,
            minify: true,
            sourcemap: true,
            splitting: true,
            out_dir: out_dir,
        };
    }
}

pub struct BundleV2 {
    config: BundleConfig = BundleConfig {};
    entry_count: usize = 0;
    chunks: [32]ChunkEntry = undefined;
    chunk_len: usize = 0;
    source_code_length: usize = 0;
    total_output_bytes: usize = 0;
    has_top_level_await: bool = false;

    pub static fn init(config: BundleConfig): BundleV2 {
        return BundleV2 {
            config: config,
            entry_count: 0,
            chunks: undefined,
            chunk_len: 0,
            source_code_length: 0,
            total_output_bytes: 0,
            has_top_level_await: false,
        };
    }

    pub fn addSourceBytes(*_, bytes: usize) {
        _.source_code_length += bytes;
    }

    pub fn markTopLevelAwait(*_) {
        _.has_top_level_await = true;
    }

    pub fn emitChunk(*_, entry: ChunkEntry): bool {
        if (_.chunk_len >= 32) {
            return false;
        }
        _.chunks[_.chunk_len] = entry;
        _.chunk_len += 1;
        _.total_output_bytes += entry.size_bytes;
        if (entry.kind == ChunkKind.EntryPoint) {
            _.entry_count += 1;
        }
        return true;
    }

    pub fn chunkCount(): usize {
        return _.chunk_len;
    }

    pub fn findChunk(name: []const u8): ChunkEntry? {
        let i: usize = 0;
        while (i < _.chunk_len) : (i += 1) {
            if (std.mem.eql(u8, _.chunks[i].name, name)) {
                return _.chunks[i];
            }
        }
        return null;
    }

    pub fn compressionRatioPermille(): u32 {
        if (_.source_code_length == 0) {
            return 1000;
        }
        return @as(u32, @intCast((_.total_output_bytes * 1000) / _.source_code_length));
    }

    pub fn reset(*_) {
        _.entry_count = 0;
        _.chunk_len = 0;
        _.source_code_length = 0;
        _.total_output_bytes = 0;
        _.has_top_level_await = false;
    }
}

pub const INVALID_MODULE_INDEX: u32 = 0xFFFF_FFFF;

/// Import statement classification (absorbed from Bun `options_types/import_record.zig`)
pub enum ImportKind: u8 {
    EntryPoint = 0,
    ImportStmt = 1,
    RequireCall = 2,
    DynamicImport = 3,
    CssAtImport = 4,
    AssetUrl = 5,
    Internal = 6,

    pub fn isCommonJS(): bool {
        return _ == .RequireCall;
    }

    pub fn isFromCSS(): bool {
        return _ == .CssAtImport || _ == .AssetUrl;
    }

    pub fn label(): []const u8 {
        return switch (_) {
            .EntryPoint => "entry-point",
            .ImportStmt => "import-statement",
            .RequireCall => "require-call",
            .DynamicImport => "dynamic-import",
            .CssAtImport => "import-rule",
            .AssetUrl => "url-token",
            .Internal => "internal",
        };
    }
}

/// Directed dependency record inside a module AST (absorbed from Bun `ImportRecord`)
pub struct ImportRecord {
    specifier: []const u8 = "";
    kind: ImportKind = .ImportStmt;
    resolved_index: u32 = INVALID_MODULE_INDEX;
    is_external: bool = false;
    side_effects: bool = true;
    range_start: u32 = 0;
    range_end: u32 = 0;
}

pub struct ModuleNode {
    id: u32 = 0;
    path: []const u8 = "";
    is_entry: bool = false;
    size_bytes: usize = 0;
    has_side_effects: bool = true;
    reachable: bool = false;
    imports: std.ArrayList(ImportRecord) = .empty;
}

pub err GraphError {
    InvalidNode,
    CyclicDependency,
    OutOfMemory,
}

/// Deterministic dependency graph supporting node registration, edge tracking,
/// cycle detection, dependency-first topological sorting, and tree-shaking reachability.
pub struct ModuleGraph {
    nodes: std.ArrayList(ModuleNode) = .empty;
    allocator: std.mem.Allocator = std.heap.page_allocator;

    pub static fn init(allocator: std.mem.Allocator): ModuleGraph {
        return ModuleGraph{
            nodes: .empty,
            allocator: allocator,
        };
    }

    pub fn deinit(*_) {
        for (*node of _.nodes.items) {
            node.imports.deinit(_.allocator);
        }
        _.nodes.deinit(_.allocator);
    }

    pub fn nodeCount(): usize {
        return _.nodes.items.len;
    }

    pub fn findModule(path: []const u8): u32? {
        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.nodes.items[i].path, path)) {
                return i as u32;
            }
        }
        return null;
    }

    pub fn addModule(*_, path: []const u8, is_entry: bool, size_bytes: usize): u32! {
        if (existing := _.findModule(path)) {
            if (is_entry) {
                _.nodes.items[existing as usize].is_entry = true;
            }
            return existing;
        }
        const id = _.nodes.items.len as u32;
        _.nodes.append(_.allocator, ModuleNode{
            id: id,
            path: path,
            is_entry: is_entry,
            size_bytes: size_bytes,
            has_side_effects: true,
            reachable: false,
            imports: .empty,
        })?;
        return id;
    }

    pub fn addImport(*_, from_id: u32, record: ImportRecord): !void {
        const from_idx = from_id as usize;
        if (from_idx >= _.nodes.items.len) {
            return error.InvalidNode;
        }
        if (!record.is_external && record.resolved_index != INVALID_MODULE_INDEX) {
            if ((record.resolved_index as usize) >= _.nodes.items.len) {
                return error.InvalidNode;
            }
        }
        _.nodes.items[from_idx].imports.append(_.allocator, record)?;
    }

    pub fn addDependency(*_, from_id: u32, to_id: u32, specifier: []const u8, kind: ImportKind): !void {
        return _.addImport(from_id, ImportRecord{
            specifier: specifier,
            kind: kind,
            resolved_index: to_id,
            is_external: false,
            side_effects: true,
        });
    }

    fn dfsCycle(idx: usize, color: []u8): bool {
        color[idx] = 1; // Visiting (Gray)
        for (imp of _.nodes.items[idx].imports.items) {
            if (imp.is_external || imp.resolved_index == INVALID_MODULE_INDEX) continue;
            const dep = imp.resolved_index as usize;
            if (dep >= _.nodes.items.len) continue;
            if (color[dep] == 1) return true;
            if (color[dep] == 0) {
                if (_.dfsCycle(dep, color)) return true;
            }
        }
        color[idx] = 2; // Visited (Black)
        return false;
    }

    /// Detects whether any directed dependency cycle exists in the module graph.
    pub fn hasCycle(): bool {
        const n = _.nodes.items.len;
        if (n == 0) return false;
        let color = _.allocator.alloc(u8, n) catch return false;
        defer _.allocator.free(color);
        @memset(color, 0);

        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (color[i] == 0) {
                if (_.dfsCycle(i, color)) return true;
            }
        }
        return false;
    }

    fn dfsTopo(idx: usize, visited: []bool, out: *std.ArrayList(u32), allocator: std.mem.Allocator): !void {
        visited[idx] = true;
        for (imp of _.nodes.items[idx].imports.items) {
            if (imp.is_external || imp.resolved_index == INVALID_MODULE_INDEX) continue;
            const dep = imp.resolved_index as usize;
            if (dep >= _.nodes.items.len) continue;
            if (!visited[dep]) {
                _.dfsTopo(dep, visited, out, allocator)?;
            }
        }
        out.append(allocator, idx as u32)?;
    }

    /// Returns a deterministic dependency-first topological ordering of module node IDs.
    /// Fails with `error.CyclicDependency` if a cycle is present.
    pub fn topoSort(allocator: std.mem.Allocator): ([]u32)! {
        if (_.hasCycle()) {
            return error.CyclicDependency;
        }
        const n = _.nodes.items.len;
        let out: std.ArrayList(u32) = .empty;
        errdefer out.deinit(allocator);
        if (n == 0) return out.toOwnedSlice(allocator);

        let visited = allocator.alloc(bool, n)?;
        defer allocator.free(visited);
        @memset(visited, false);

        // Process entry points first in deterministic order, then any remaining nodes
        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (_.nodes.items[i].is_entry && !visited[i]) {
                _.dfsTopo(i, visited, *out, allocator)?;
            }
        }
        i = 0;
        while (i < n) : (i += 1) {
            if (!visited[i]) {
                _.dfsTopo(i, visited, *out, allocator)?;
            }
        }
        return out.toOwnedSlice(allocator);
    }

    fn dfsMarkReachable(*_, idx: usize) {
        if (idx >= _.nodes.items.len) return;
        if (_.nodes.items[idx].reachable) return;
        _.nodes.items[idx].reachable = true;

        const imp_len = _.nodes.items[idx].imports.items.len;
        let k: usize = 0;
        while (k < imp_len) : (k += 1) {
            const imp = _.nodes.items[idx].imports.items[k];
            if (imp.is_external || imp.resolved_index == INVALID_MODULE_INDEX) continue;
            _.dfsMarkReachable(imp.resolved_index as usize);
        }
    }

    /// Marks `entry_id` and all its transitive dependencies as reachable for tree-shaking.
    pub fn markReachable(*_, entry_id: u32): !void {
        const idx = entry_id as usize;
        if (idx >= _.nodes.items.len) return error.InvalidNode;
        _.dfsMarkReachable(idx);
    }

    /// Clears reachability flags and marks all `is_entry` nodes and their transitive dependencies.
    pub fn markAllEntryPointsReachable(*_) {
        for (*node of _.nodes.items) {
            node.reachable = false;
        }
        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            if (_.nodes.items[i].is_entry) {
                _.dfsMarkReachable(i);
            }
        }
    }

    pub fn isReachable(id: u32): bool {
        const idx = id as usize;
        if (idx >= _.nodes.items.len) return false;
        return _.nodes.items[idx].reachable;
    }

    pub fn reachableCount(): usize {
        let count: usize = 0;
        for (node of _.nodes.items) {
            if (node.reachable) count += 1;
        }
        return count;
    }
}
