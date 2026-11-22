// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/computeCrossChunkDependencies.cp | Synced: 2026-11-22T07:31:30Z
// Pure-CP zero-allocation bundler code-splitting cross-chunk dependency analyzer:
// - Resolves cross-chunk top-level symbol references into deterministic chunk import/export tables
// - Follows CJS `namespace_alias` indirections and skips unbound / missing-import symbols
// - Injects entry-point `SideEffect` imports for all JS chunks sharing the entry's `entry_bits`
// - Tracks cross-chunk dynamic `import()` edges in deterministic ascending chunk order for content hashing.

/// 支持的最大 Chunk 数量
pub const CROSS_CHUNK_MAX_CHUNKS: usize = 32;

/// 支持的最大顶层符号数量
pub const CROSS_CHUNK_MAX_SYMBOLS: usize = 128;

/// 支持的最大符号使用记录数
pub const CROSS_CHUNK_MAX_USES: usize = 256;

/// 计算结果允许的最大跨 Chunk 导入/导出边数
pub const CROSS_CHUNK_MAX_EDGES: usize = 128;

/// 跨 Chunk 依赖分析错误集
pub err CrossChunkError {
    TooManyChunks,
    TooManySymbols,
    TooManyUses,
    TooManyEdges,
    InvalidChunkId,
    InvalidSymbolId,
}

/// 跨 Chunk 导入边类别
pub enum CrossChunkImportKind: u8 {
    /// 静态符号导入（`import { x } from "./chunk-N.js"`）
    Stmt = 0,
    /// 入口点无符号副作用导入（`import "./chunk-N.js"`，确保共享 chunk 副作用被执行）
    SideEffect = 1,
    /// 动态导入（`import("./chunk-N.js")`，纳入依赖哈希计算）
    Dynamic = 2,
}

/// Chunk 节点元数据
pub struct CrossChunkNode {
    id: usize = 0,
    is_js: bool = true,
    is_entry_point: bool = false,
    entry_point_bit: u32 = 0,
    entry_bits: u64 = 0,
}

/// 顶层符号元数据
pub struct CrossChunkSymbol {
    id: usize = 0,
    name: []const u8 = "",
    declared_chunk: usize = 0,
    is_unbound: bool = false,
    is_missing_import: bool = false,
    /// 若为从 CJS 模块导入的 ESM 符号，重定向至其 `require()` 命名空间符号
    namespace_alias_sym: usize? = null,
}

struct SymbolUseRecord {
    chunk_id: usize = 0,
    symbol_id: usize = 0,
}

struct DynamicImportRecord {
    from_chunk: usize = 0,
    to_chunk: usize = 0,
}

/// 计算产出的单条跨 Chunk 导入边
pub struct CrossChunkImportEdge {
    from_chunk: usize = 0,
    to_chunk: usize = 0,
    kind: CrossChunkImportKind = .Stmt,
    symbol_id: usize? = null,
}

/// 计算产出的单条跨 Chunk 导出条目
pub struct CrossChunkExportEntry {
    chunk_id: usize = 0,
    symbol_id: usize = 0,
    symbol_name: []const u8 = "",
}

/// 跨 Chunk 依赖计算确定性结果表
pub struct CrossChunkResult {
    imports: [CROSS_CHUNK_MAX_EDGES]CrossChunkImportEdge = [CrossChunkImportEdge{}; CROSS_CHUNK_MAX_EDGES],
    import_count: usize = 0,
    exports: [CROSS_CHUNK_MAX_EDGES]CrossChunkExportEntry = [CrossChunkExportEntry{}; CROSS_CHUNK_MAX_EDGES],
    export_count: usize = 0,

    fn hasImportEdge(from_chunk: usize, to_chunk: usize, kind: CrossChunkImportKind, symbol_id: usize?): bool {
        let i: usize = 0;
        while (i < _.import_count) {
            const e = _.imports[i];
            if (e.from_chunk == from_chunk && e.to_chunk == to_chunk && e.kind == kind) {
                if (e.symbol_id == null && symbol_id == null) return true;
                if (ea := e.symbol_id) {
                    if (sa := symbol_id) {
                        if (ea == sa) return true;
                    }
                }
            }
            i += 1;
        }
        return false;
    }

    fn hasAnyStaticImportBetween(from_chunk: usize, to_chunk: usize): bool {
        let i: usize = 0;
        while (i < _.import_count) {
            const e = _.imports[i];
            if (e.from_chunk == from_chunk && e.to_chunk == to_chunk &&
                (e.kind == .Stmt || e.kind == .SideEffect))
            {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn isSymbolExportedByChunk(chunk_id: usize, symbol_id: usize): bool {
        let i: usize = 0;
        while (i < _.export_count) {
            if (_.exports[i].chunk_id == chunk_id && _.exports[i].symbol_id == symbol_id) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn importCountForChunk(from_chunk: usize): usize {
        let cnt: usize = 0;
        let i: usize = 0;
        while (i < _.import_count) {
            if (_.imports[i].from_chunk == from_chunk) {
                cnt += 1;
            }
            i += 1;
        }
        return cnt;
    }

    fn exportCountForChunk(chunk_id: usize): usize {
        let cnt: usize = 0;
        let i: usize = 0;
        while (i < _.export_count) {
            if (_.exports[i].chunk_id == chunk_id) {
                cnt += 1;
            }
            i += 1;
        }
        return cnt;
    }

    fn pushImport(*_, edge: CrossChunkImportEdge)! {
        if (_.hasImportEdge(edge.from_chunk, edge.to_chunk, edge.kind, edge.symbol_id)) {
            return;
        }
        if (_.import_count >= CROSS_CHUNK_MAX_EDGES) {
            return err.TooManyEdges;
        }
        _.imports[_.import_count] = edge;
        _.import_count += 1;
    }

    fn pushExport(*_, entry: CrossChunkExportEntry)! {
        if (_.isSymbolExportedByChunk(entry.chunk_id, entry.symbol_id)) {
            return;
        }
        if (_.export_count >= CROSS_CHUNK_MAX_EDGES) {
            return err.TooManyEdges;
        }
        _.exports[_.export_count] = entry;
        _.export_count += 1;
    }
}

fn importEdgeLess(a: CrossChunkImportEdge, b: CrossChunkImportEdge): bool {
    if (a.from_chunk != b.from_chunk) return a.from_chunk < b.from_chunk;
    if (a.to_chunk != b.to_chunk) return a.to_chunk < b.to_chunk;
    const ka: u8 = @intFromEnum(a.kind);
    const kb: u8 = @intFromEnum(b.kind);
    if (ka != kb) return ka < kb;
    const sa: usize = a.symbol_id ?? 0xFFFF_FFFF;
    const sb: usize = b.symbol_id ?? 0xFFFF_FFFF;
    return sa < sb;
}

fn exportEntryLess(a: CrossChunkExportEntry, b: CrossChunkExportEntry): bool {
    if (a.chunk_id != b.chunk_id) return a.chunk_id < b.chunk_id;
    return a.symbol_id < b.symbol_id;
}

/// 零分配跨 Chunk 依赖分析器（吸收自 Bun `computeCrossChunkDependencies.cp`）
pub struct CrossChunkAnalyzer {
    chunks: [CROSS_CHUNK_MAX_CHUNKS]CrossChunkNode,
    chunk_count: usize,
    symbols: [CROSS_CHUNK_MAX_SYMBOLS]CrossChunkSymbol,
    symbol_count: usize,
    uses: [CROSS_CHUNK_MAX_USES]SymbolUseRecord,
    use_count: usize,
    dynamic_imports: [CROSS_CHUNK_MAX_EDGES]DynamicImportRecord,
    dynamic_count: usize,

    static fn init(): CrossChunkAnalyzer {
        return CrossChunkAnalyzer{
            chunks: [CrossChunkNode{}; CROSS_CHUNK_MAX_CHUNKS],
            chunk_count: 0,
            symbols: [CrossChunkSymbol{}; CROSS_CHUNK_MAX_SYMBOLS],
            symbol_count: 0,
            uses: [SymbolUseRecord{}; CROSS_CHUNK_MAX_USES],
            use_count: 0,
            dynamic_imports: [DynamicImportRecord{}; CROSS_CHUNK_MAX_EDGES],
            dynamic_count: 0,
        };
    }

    fn addChunk(*_, is_js: bool, is_entry_point: bool, entry_point_bit: u32, entry_bits: u64): usize! {
        if (_.chunk_count >= CROSS_CHUNK_MAX_CHUNKS) {
            return err.TooManyChunks;
        }
        const id = _.chunk_count;
        _.chunks[id] = CrossChunkNode{
            id: id,
            is_js: is_js,
            is_entry_point: is_entry_point,
            entry_point_bit: entry_point_bit,
            entry_bits: entry_bits,
        };
        _.chunk_count += 1;
        return id;
    }

    fn addSymbol(
        *_,
        name: []const u8,
        declared_chunk: usize,
        is_unbound: bool,
        is_missing_import: bool,
        namespace_alias_sym: usize?,
    ): usize! {
        if (declared_chunk >= _.chunk_count) {
            return err.InvalidChunkId;
        }
        if (ns := namespace_alias_sym) {
            if (ns >= _.symbol_count) {
                return err.InvalidSymbolId;
            }
        }
        if (_.symbol_count >= CROSS_CHUNK_MAX_SYMBOLS) {
            return err.TooManySymbols;
        }
        const id = _.symbol_count;
        _.symbols[id] = CrossChunkSymbol{
            id: id,
            name: name,
            declared_chunk: declared_chunk,
            is_unbound: is_unbound,
            is_missing_import: is_missing_import,
            namespace_alias_sym: namespace_alias_sym,
        };
        _.symbol_count += 1;
        return id;
    }

    fn recordSymbolUse(*_, chunk_id: usize, symbol_id: usize)! {
        if (chunk_id >= _.chunk_count) return err.InvalidChunkId;
        if (symbol_id >= _.symbol_count) return err.InvalidSymbolId;
        if (_.use_count >= CROSS_CHUNK_MAX_USES) return err.TooManyUses;
        _.uses[_.use_count] = SymbolUseRecord{
            chunk_id: chunk_id,
            symbol_id: symbol_id,
        };
        _.use_count += 1;
    }

    fn recordDynamicImport(*_, from_chunk: usize, to_chunk: usize)! {
        if (from_chunk >= _.chunk_count || to_chunk >= _.chunk_count) {
            return err.InvalidChunkId;
        }
        if (_.dynamic_count >= CROSS_CHUNK_MAX_EDGES) {
            return err.TooManyEdges;
        }
        _.dynamic_imports[_.dynamic_count] = DynamicImportRecord{
            from_chunk: from_chunk,
            to_chunk: to_chunk,
        };
        _.dynamic_count += 1;
    }

    /// 计算全部跨 Chunk 静态符号导入、导出、入口副作用导入与动态导入边（确定性排序）
    fn compute(): CrossChunkResult! {
        let res = CrossChunkResult{};

        // 1. Cross-chunk symbol uses -> Stmt import + producer export
        let u: usize = 0;
        while (u < _.use_count) {
            const rec = _.uses[u];
            const consumer = _.chunks[rec.chunk_id];
            if (consumer.is_js) {
                let sym = _.symbols[rec.symbol_id];
                if (!sym.is_unbound && !sym.is_missing_import) {
                    if (ns_id := sym.namespace_alias_sym) {
                        sym = _.symbols[ns_id];
                    }
                    const producer_chunk = sym.declared_chunk;
                    if (producer_chunk != rec.chunk_id && _.chunks[producer_chunk].is_js) {
                        res.pushImport(CrossChunkImportEdge{
                            from_chunk: rec.chunk_id,
                            to_chunk: producer_chunk,
                            kind: .Stmt,
                            symbol_id: sym.id,
                        })?;
                        res.pushExport(CrossChunkExportEntry{
                            chunk_id: producer_chunk,
                            symbol_id: sym.id,
                            symbol_name: sym.name,
                        })?;
                    }
                }
            }
            u += 1;
        }

        // 2. Entry-point side-effect imports: every JS chunk with entry's bit set must be imported
        let c: usize = 0;
        while (c < _.chunk_count) {
            const entry = _.chunks[c];
            if (entry.is_js && entry.is_entry_point && entry.entry_point_bit < 64) {
                const bit_mask: u64 = (@as(u64, 1)) << @intCast(entry.entry_point_bit);
                let other: usize = 0;
                while (other < _.chunk_count) {
                    if (other != c && _.chunks[other].is_js) {
                        if ((_.chunks[other].entry_bits & bit_mask) != 0) {
                            if (!res.hasAnyStaticImportBetween(c, other)) {
                                res.pushImport(CrossChunkImportEdge{
                                    from_chunk: c,
                                    to_chunk: other,
                                    kind: .SideEffect,
                                    symbol_id: null,
                                })?;
                            }
                        }
                    }
                    other += 1;
                }
            }
            c += 1;
        }

        // 3. Cross-chunk dynamic imports (ignore self-imports and non-JS consumer)
        let d: usize = 0;
        while (d < _.dynamic_count) {
            const dyn_rec = _.dynamic_imports[d];
            if (dyn_rec.from_chunk != dyn_rec.to_chunk && _.chunks[dyn_rec.from_chunk].is_js) {
                res.pushImport(CrossChunkImportEdge{
                    from_chunk: dyn_rec.from_chunk,
                    to_chunk: dyn_rec.to_chunk,
                    kind: .Dynamic,
                    symbol_id: null,
                })?;
            }
            d += 1;
        }

        // 4. Deterministic insertion sort for imports & exports
        let i: usize = 1;
        while (i < res.import_count) {
            const key = res.imports[i];
            let j = i;
            while (j > 0 && importEdgeLess(key, res.imports[j - 1])) {
                res.imports[j] = res.imports[j - 1];
                j -= 1;
            }
            res.imports[j] = key;
            i += 1;
        }

        i = 1;
        while (i < res.export_count) {
            const key = res.exports[i];
            let j = i;
            while (j > 0 && exportEntryLess(key, res.exports[j - 1])) {
                res.exports[j] = res.exports[j - 1];
                j -= 1;
            }
            res.exports[j] = key;
            i += 1;
        }

        return res;
    }
}
