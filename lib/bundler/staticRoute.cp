// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/StaticRouteVisitor.cp | Synced: 2026-11-18T21:40:53Z
// Pure-CP zero-allocation Static Route / SSG Analyzer for React Server Components:
// - Determines whether an entry route module is fully static (`isFullyStatic`) or
//   transitively imports any `"use client"` boundary (`hasTransitiveUseClient`)
// - Uses bounded iterative DFS with cycle-safe visited tracking and memoization cache
//   directly integrated with `ServerComponentBoundaryTable`.

import { UseDirective, ServerComponentBoundaryTable } from "./rsc";

/// 静态路由图节点与有向导入边容量上限（零堆分配）
pub const MAX_STATIC_ROUTE_NODES: usize = 256;
pub const MAX_STATIC_ROUTE_EDGES: usize = 1024;
pub const INVALID_ROUTE_SOURCE: u32 = 0xffffffff;

/// 静态路由分析器错误集
pub err StaticRouteError {
    NodeIndexOutOfRange,
    EdgeCapacityExceeded,
}

/// 单条模块导入有向边（`from_source` imports `to_source`）
pub struct StaticRouteEdge {
    from_source: u32 = 0,
    to_source: u32 = INVALID_ROUTE_SOURCE,
}

/// 静态零堆分配模块导入图
pub struct StaticRouteGraph {
    node_count: u32 = 0,
    edges: [MAX_STATIC_ROUTE_EDGES]StaticRouteEdge = [StaticRouteEdge{}; MAX_STATIC_ROUTE_EDGES],
    edge_len: usize = 0,

    static fn init(node_count: u32): StaticRouteGraph! {
        if (node_count > MAX_STATIC_ROUTE_NODES) {
            return err.NodeIndexOutOfRange;
        }
        return StaticRouteGraph{
            node_count: node_count,
            edges: [StaticRouteEdge{}; MAX_STATIC_ROUTE_EDGES],
            edge_len: 0,
        };
    }

    fn addImport(*_, from_source: u32, to_source: u32): ! {
        if (from_source >= _.node_count) {
            return err.NodeIndexOutOfRange;
        }
        if (to_source != INVALID_ROUTE_SOURCE && to_source >= _.node_count) {
            return err.NodeIndexOutOfRange;
        }
        if (_.edge_len >= MAX_STATIC_ROUTE_EDGES) {
            return err.EdgeCapacityExceeded;
        }
        _.edges[_.edge_len] = StaticRouteEdge{
            from_source: from_source,
            to_source: to_source,
        };
        _.edge_len += 1;
    }
}

// Memoization cache tri-state
const CACHE_UNKNOWN: u8 = 0;
const CACHE_NO_CLIENT: u8 = 1;
const CACHE_HAS_CLIENT: u8 = 2;

/// 带记忆化缓存与环路防护的静态路由分析器（对齐 Bun `StaticRouteVisitor`）
pub struct StaticRouteVisitor {
    cache: [MAX_STATIC_ROUTE_NODES]u8 = [CACHE_UNKNOWN; MAX_STATIC_ROUTE_NODES],
    cache_hits: u32 = 0,
    nodes_visited: u32 = 0,

    static fn init(): StaticRouteVisitor {
        return StaticRouteVisitor{
            cache: [CACHE_UNKNOWN; MAX_STATIC_ROUTE_NODES],
            cache_hits: 0,
            nodes_visited: 0,
        };
    }

    fn resetCache(*_) {
        let i: usize = 0;
        while (i < MAX_STATIC_ROUTE_NODES) {
            _.cache[i] = CACHE_UNKNOWN;
            i += 1;
        }
        _.cache_hits = 0;
        _.nodes_visited = 0;
    }

    /// 检查 `source_index` 是否为任意 `"use client"` 边界模块（含原始索引或对端引用索引）
    static fn isClientBoundary(boundaries: *const ServerComponentBoundaryTable, source_index: u32): bool {
        let i: usize = 0;
        while (i < boundaries.len) {
            const b = boundaries.items[i];
            if (b.use_directive == UseDirective.use_client) {
                if (b.reference_source_index == source_index || b.source_index == source_index) {
                    return true;
                }
            }
            i += 1;
        }
        return false;
    }

    /// 迭代 DFS 判定 `entry_source_index` 是否直接或传递导入任意 `"use client"` 边界
    fn hasTransitiveUseClient(
        *_,
        graph: *const StaticRouteGraph,
        boundaries: *const ServerComponentBoundaryTable,
        entry_source_index: u32,
    ): bool! {
        if (entry_source_index >= graph.node_count) {
            return err.NodeIndexOutOfRange;
        }
        const entry_idx: usize = @intCast(entry_source_index);
        if (_.cache[entry_idx] != CACHE_UNKNOWN) {
            _.cache_hits += 1;
            return _.cache[entry_idx] == CACHE_HAS_CLIENT;
        }

        // Check if entry itself is already a client boundary
        if (StaticRouteVisitor.isClientBoundary(boundaries, entry_source_index)) {
            _.cache[entry_idx] = CACHE_HAS_CLIENT;
            return true;
        }

        // Iterative DFS stack (zero recursion, cycle-safe via per-run `visited`)
        let visited: [MAX_STATIC_ROUTE_NODES]bool = [false; MAX_STATIC_ROUTE_NODES];
        let stack: [MAX_STATIC_ROUTE_NODES]u32 = [0; MAX_STATIC_ROUTE_NODES];
        let sp: usize = 0;

        visited[entry_idx] = true;
        stack[sp] = entry_source_index;
        sp += 1;

        let found_client = false;
        while (sp > 0) {
            sp -= 1;
            const cur = stack[sp];
            _.nodes_visited += 1;

            let e: usize = 0;
            while (e < graph.edge_len) {
                const edge = graph.edges[e];
                e += 1;
                if (edge.from_source != cur) {
                    continue;
                }
                const target = edge.to_source;
                if (target == INVALID_ROUTE_SOURCE) {
                    continue;
                }
                const t_idx: usize = @intCast(target);

                if (_.cache[t_idx] == CACHE_HAS_CLIENT) {
                    _.cache_hits += 1;
                    found_client = true;
                    break;
                }
                if (_.cache[t_idx] == CACHE_NO_CLIENT) {
                    _.cache_hits += 1;
                    continue;
                }
                if (StaticRouteVisitor.isClientBoundary(boundaries, target)) {
                    _.cache[t_idx] = CACHE_HAS_CLIENT;
                    found_client = true;
                    break;
                }
                if (!visited[t_idx]) {
                    visited[t_idx] = true;
                    stack[sp] = target;
                    sp += 1;
                }
            }
            if (found_client) {
                break;
            }
        }

        _.cache[entry_idx] = if (found_client) CACHE_HAS_CLIENT else CACHE_NO_CLIENT;
        return found_client;
    }

    /// 若入口路由及其全部传递依赖均不含 `"use client"` 边界，返回 `true`（可静态生成 SSG）
    fn isFullyStatic(
        *_,
        graph: *const StaticRouteGraph,
        boundaries: *const ServerComponentBoundaryTable,
        entry_source_index: u32,
    ): bool! {
        const has_client = _.hasTransitiveUseClient(graph, boundaries, entry_source_index)?;
        return !has_client;
    }
}
