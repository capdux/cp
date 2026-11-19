// Source: /data/dev/bun/src/bundler/
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
