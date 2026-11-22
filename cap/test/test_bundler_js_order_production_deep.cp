// cap/test/test_bundler_js_order_production_deep.cp
// 深度生产级回归测试：cap:bundler JS Chunk 部件拓扑排序与线性化引擎 (lib/bundler/js_order.cp)

import { test } from "cap:test";
import {
    JS_ORDER_MAX_PARTS_PER_FILE,
    JS_ORDER_MAX_IMPORTS_PER_PART,
    JS_ORDER_RUNTIME_SOURCE_INDEX,
    JS_ORDER_INVALID_SOURCE_INDEX,
    JS_ORDER_NAMESPACE_EXPORT_PART,
    JsPartWrapKind,
    JsOrderImportKind,
    JsChunkOrderEntry,
    JsOrderGraph,
} from "cap:bundler";

test("js_order: diamond dependency DFS postorder, tie-breaker sorting & adjacent range coalescing") {
    let g = JsOrderGraph.init();

    // 菱形依赖：Entry(1) -> B(2), C(3); B(2) -> D(4); C(3) -> D(4)
    // D 有 3 个连续存活 part (1, 2, 3)，应自动合并为单个 JsPartRange [1, 4)
    const s_d = g.addFile(4, 2, 40, 0b1, JsPartWrapKind.None)?;
    * = g.addPart(s_d, false, true)?; // part 0: namespace export (dead)
    * = g.addPart(s_d, true, true)?;  // part 1
    * = g.addPart(s_d, true, true)?;  // part 2
    * = g.addPart(s_d, true, true)?;  // part 3

    const s_b = g.addFile(2, 1, 20, 0b1, JsPartWrapKind.None)?;
    * = g.addPart(s_b, false, true)?;
    const p_b1 = g.addPart(s_b, true, true)?;
    g.addPartImport(s_b, p_b1, 4, JsOrderImportKind.Stmt, false)?;

    const s_c = g.addFile(3, 1, 10, 0b1, JsPartWrapKind.None)?; // tie_breaker=10 < B(20)
    * = g.addPart(s_c, false, true)?;
    const p_c1 = g.addPart(s_c, true, true)?;
    g.addPartImport(s_c, p_c1, 4, JsOrderImportKind.Stmt, false)?;

    const s_entry = g.addFile(1, 0, 1, 0b1, JsPartWrapKind.None)?;
    * = g.addPart(s_entry, false, true)?;
    const p_e1 = g.addPart(s_entry, true, true)?;
    g.addPartImport(s_entry, p_e1, 2, JsOrderImportKind.Stmt, false)?;
    g.addPartImport(s_entry, p_e1, 3, JsOrderImportKind.Stmt, false)?;

    // 故意乱序传入 chunk_files，验证确定性 (distance, tie_breaker) 排序
    const chunk_files: [4]u32 = [4, 3, 2, 1];
    const res = g.computeChunkOrder(0b1, chunk_files[0..], 0, false, false)?;

    // Entry(distance=0) 最先被访问 -> DFS 访问 B(2) -> D(4)，再访问 C(3)（D 已访问）-> 最后 Entry(1)
    const files = res.filesSlice();
    if (files.len != 4) return err.UnexpectedFileCount;
    if (files[0] != 4 || files[1] != 2 || files[2] != 3 || files[3] != 1) {
        return err.UnexpectedFileOrder;
    }

    // 验证 D 的 3 个相邻 part 合并为单个 range [1, 4)
    const ranges = res.partRangesSlice();
    if (ranges.len != 4) return err.UnexpectedRangeCount;
    if (ranges[0].source_index != 4 || ranges[0].part_index_begin != 1 || ranges[0].part_index_end != 4) {
        return err.RangeCoalesceFailed;
    }
    if (ranges[0].len() != 3) return err.RangeLenMismatch;
    if (res.totalParts() != 6) return err.TotalPartsMismatch;

    // 校验 JsChunkOrderEntry.isBefore 弱序比较
    const e_a = JsChunkOrderEntry{ source_index: 2, distance: 1, tie_breaker: 20 };
    const e_b = JsChunkOrderEntry{ source_index: 3, distance: 1, tie_breaker: 10 };
    if (!JsChunkOrderEntry.isBefore(e_b, e_a)) return err.TieBreakerOrderFailed;
    if (JsChunkOrderEntry.isBefore(e_a, e_b)) return err.TieBreakerOrderInverted;
}

test("js_order: runtime & CJS wrapper hoisted to parts_prefix, namespace_export_part prioritized") {
    let g = JsOrderGraph.init();

    // Runtime 模块 (source_index = 0)
    const s_rt = g.addFile(JS_ORDER_RUNTIME_SOURCE_INDEX, 0, 0, 0b1, JsPartWrapKind.None)?;
    * = g.addPart(s_rt, false, true)?;
    * = g.addPart(s_rt, true, true)?; // runtime part 1
    * = g.addPart(s_rt, true, true)?; // runtime part 2 -> coalesced [1, 3) in prefix

    // CommonJS 包装模块（不可拆分，所有 3 个 part 必须整体置于 parts_prefix）
    const s_cjs = g.addFile(10, 1, 10, 0b1, JsPartWrapKind.Cjs)?;
    * = g.addPart(s_cjs, true, true)?;
    * = g.addPart(s_cjs, true, true)?;
    * = g.addPart(s_cjs, true, true)?;

    // ESM 入口模块（含存活的 namespace_export_part = 0，且依赖 CJS 模块）
    const s_esm = g.addFile(20, 0, 20, 0b1, JsPartWrapKind.None)?;
    * = g.addPart(s_esm, true, true)?; // part 0: namespace_export_part (live)
    const p_esm1 = g.addPart(s_esm, true, true)?;
    g.addPartImport(s_esm, p_esm1, 10, JsOrderImportKind.Require, false)?;

    const chunk_files: [2]u32 = [20, 10];
    const res = g.computeChunkOrder(0b1, chunk_files[0..], 0, false, false)?;

    // prefix 中应有 2 个区间：runtime [1,3) 和 CJS [0,3)
    const prefix = res.prefixRangesSlice();
    if (prefix.len != 2) return err.UnexpectedPrefixCount;
    if (prefix[0].source_index != JS_ORDER_RUNTIME_SOURCE_INDEX ||
        prefix[0].part_index_begin != 1 || prefix[0].part_index_end != 3)
    {
        return err.RuntimePrefixMismatch;
    }
    if (prefix[1].source_index != 10 || prefix[1].part_index_begin != 0 || prefix[1].part_index_end != 3) {
        return err.CjsPrefixMismatch;
    }

    // main 中应有 ESM 的 namespace_export_part (0) 和 part 1 -> 合并为 [0, 2)
    const main = res.mainRangesSlice();
    if (main.len != 1) return err.UnexpectedMainCount;
    if (main[0].source_index != 20 || main[0].part_index_begin != JS_ORDER_NAMESPACE_EXPORT_PART || main[0].part_index_end != 2) {
        return err.NamespaceExportMergeFailed;
    }
    if (res.totalParts() != 7) return err.TotalPartsMismatch;
}

test("js_order: code splitting exact entry_bits vs non-splitting intersection & CSS exemption") {
    let g = JsOrderGraph.init();

    // 入口 A: entry_bits = 0b01
    // 共享模块 S: entry_bits = 0b11 (同时被入口 A 和 B 引用)
    // CSS 模块 C: entry_bits = 0b11, is_css = true (CSS 在 splitting 模式下仍按交集判定)
    const s_shared = g.addFile(2, 1, 2, 0b11, JsPartWrapKind.None)?;
    * = g.addPart(s_shared, false, true)?;
    * = g.addPart(s_shared, true, true)?;

    const s_css = g.addFile(3, 1, 3, 0b11, JsPartWrapKind.None)?;
    g.setFileFlags(s_css, true, false)?;
    * = g.addPart(s_css, false, true)?;
    * = g.addPart(s_css, true, true)?;

    const s_entry_a = g.addFile(1, 0, 1, 0b01, JsPartWrapKind.None)?;
    * = g.addPart(s_entry_a, false, true)?;
    const p_a1 = g.addPart(s_entry_a, true, true)?;
    g.addPartImport(s_entry_a, p_a1, 2, JsOrderImportKind.Stmt, false)?;
    g.addPartImport(s_entry_a, p_a1, 3, JsOrderImportKind.Stmt, false)?;

    const chunk_files: [1]u32 = [1];

    // 1. 启用 code_splitting = true：入口 A 的 chunk (0b01) 不包含共享 JS 模块 (0b11)，但包含 CSS 模块 (0b11 & 0b01 != 0)
    const res_split = g.computeChunkOrder(0b01, chunk_files[0..], 0, true, false)?;
    const files_split = res_split.filesSlice();
    if (files_split.len != 2) return err.SplitFileCountMismatch;
    if (files_split[0] != 3 || files_split[1] != 1) return err.SplitOrderMismatch;

    // 2. 关闭 code_splitting = false：按交集判定，共享 JS 模块 (0b11 & 0b01 != 0) 被内联进本 Chunk
    const res_nosplit = g.computeChunkOrder(0b01, chunk_files[0..], 0, false, false)?;
    const files_nosplit = res_nosplit.filesSlice();
    if (files_nosplit.len != 3) return err.NoSplitFileCountMismatch;
    if (files_nosplit[0] != 2 || files_nosplit[1] != 3 || files_nosplit[2] != 1) {
        return err.NoSplitOrderMismatch;
    }
}

test("js_order: external dynamic import cutoff, cycle safety & dead/excluded part filtering") {
    let g = JsOrderGraph.init();

    // 循环依赖 A(1) <-> B(2)，且 A 有一条外部动态 import() 指向 Lazy(3)
    // Lazy(3) 不应被静态 DFS 拉入本 Chunk
    const s_a = g.addFile(1, 0, 1, 0b1, JsPartWrapKind.None)?;
    * = g.addPart(s_a, false, true)?; // part 0: dead
    const p_a1 = g.addPart(s_a, true, true)?;  // part 1: live & included
    * = g.addPart(s_a, false, true)?;          // part 2: dead (breaks coalescing)
    * = g.addPart(s_a, true, false)?;          // part 3: live but should_include=false (filtered)
    const p_a4 = g.addPart(s_a, true, true)?;  // part 4: live & included

    const s_b = g.addFile(2, 1, 2, 0b1, JsPartWrapKind.None)?;
    * = g.addPart(s_b, false, true)?;
    const p_b1 = g.addPart(s_b, true, true)?;

    const s_lazy = g.addFile(3, 2, 3, 0b1, JsPartWrapKind.None)?;
    * = g.addPart(s_lazy, false, true)?;
    * = g.addPart(s_lazy, true, true)?;

    // A -> B (stmt), B -> A (stmt, cycle!), A -> Lazy (external dynamic import!)
    g.addPartImport(s_a, p_a1, 2, JsOrderImportKind.Stmt, false)?;
    g.addPartImport(s_a, p_a4, 3, JsOrderImportKind.DynamicImport, true)?;
    g.addPartImport(s_b, p_b1, 1, JsOrderImportKind.Stmt, false)?;

    const chunk_files: [1]u32 = [1];
    const res = g.computeChunkOrder(0b1, chunk_files[0..], 0, false, false)?;

    // Lazy(3) 必须被忽略，仅输出 B(2) 和 A(1) 且不死循环
    const files = res.filesSlice();
    if (files.len != 2) return err.CycleOrDynamicImportError;
    if (files[0] != 2 || files[1] != 1) return err.CycleOrderMismatch;

    // A 的 part 1 与 part 4 之间隔了 dead/excluded part，不能合并，应形成 3 个区间：B[1,2), A[1,2), A[4,5)
    const ranges = res.partRangesSlice();
    if (ranges.len != 3) return err.FilteredRangeCountMismatch;
    if (ranges[0].source_index != 2 || ranges[0].part_index_begin != 1 || ranges[0].part_index_end != 2) {
        return err.Range0Mismatch;
    }
    if (ranges[1].source_index != 1 || ranges[1].part_index_begin != 1 || ranges[1].part_index_end != 2) {
        return err.Range1Mismatch;
    }
    if (ranges[2].source_index != 1 || ranges[2].part_index_begin != 4 || ranges[2].part_index_end != 5) {
        return err.Range2Mismatch;
    }
}

test("js_order: Server Component Boundary (SCB) owning chunk index assignment") {
    let g = JsOrderGraph.init();

    const s_client_comp = g.addFile(100, 1, 100, 0b10, JsPartWrapKind.None)?;
    g.setFileFlags(s_client_comp, false, true)?; // is_scb = true
    * = g.addPart(s_client_comp, false, true)?;
    * = g.addPart(s_client_comp, true, true)?;

    // 未遍历前应为 null
    if (g.getScbChunkIndex(100) != null) return err.ScbShouldBeUnassigned;

    const chunk_files: [1]u32 = [100];

    // with_scb = false 时不回填
    * = g.computeChunkOrder(0b10, chunk_files[0..], 7, true, false)?;
    if (g.getScbChunkIndex(100) != null) return err.ScbModifiedWhenDisabled;

    // with_scb = true 时回填 chunk_index = 7
    * = g.computeChunkOrder(0b10, chunk_files[0..], 7, true, true)?;
    if (idx := g.getScbChunkIndex(100)) {
        if (idx != 7) return err.ScbWrongChunkIndex;
    } else {
        return err.ScbNotAssigned;
    }
}

test("js_order: malformed input & capacity overflow protection") {
    let g = JsOrderGraph.init();

    // 1. 非法源索引哨兵拒绝注册
    let caught_invalid = false;
    * = g.addFile(JS_ORDER_INVALID_SOURCE_INDEX, 0, 0, 0b1, JsPartWrapKind.None) catch {
        caught_invalid = true;
    };
    if (!caught_invalid) return err.ExpectedInvalidSourceIndex;

    // 2. 重复 source_index 拒绝注册
    const s0 = g.addFile(5, 0, 5, 0b1, JsPartWrapKind.None)?;
    let caught_dup = false;
    * = g.addFile(5, 0, 5, 0b1, JsPartWrapKind.None) catch {
        caught_dup = true;
    };
    if (!caught_dup) return err.ExpectedDuplicateFile;

    // 3. 超出单文件最大 Part 数量保护
    let p: usize = 0;
    while (p < JS_ORDER_MAX_PARTS_PER_FILE) {
        * = g.addPart(s0, true, true)?;
        p += 1;
    }
    let caught_parts = false;
    * = g.addPart(s0, true, true) catch {
        caught_parts = true;
    };
    if (!caught_parts) return err.ExpectedTooManyParts;

    // 4. 超出单 Part 最大导入数保护
    let m: usize = 0;
    while (m < JS_ORDER_MAX_IMPORTS_PER_PART) {
        g.addPartImport(s0, 0, 999, JsOrderImportKind.Stmt, false)?;
        m += 1;
    }
    let caught_imports = false;
    g.addPartImport(s0, 0, 999, JsOrderImportKind.Stmt, false) catch {
        caught_imports = true;
    };
    if (!caught_imports) return err.ExpectedTooManyImports;

    // 5. 越界 part_index 与未知 chunk_files 拒绝
    let caught_bad_part = false;
    g.addPartImport(s0, 99, 5, JsOrderImportKind.Stmt, false) catch {
        caught_bad_part = true;
    };
    if (!caught_bad_part) return err.ExpectedInvalidPartIndex;

    const bad_chunk: [1]u32 = [404];
    let caught_missing_src = false;
    * = g.computeChunkOrder(0b1, bad_chunk[0..], 0, false, false) catch {
        caught_missing_src = true;
    };
    if (!caught_missing_src) return err.ExpectedMissingSourceError;

    // 6. 空 chunk_files 安全返回空结果
    const empty_chunk: [0]u32 = [];
    const empty_res = g.computeChunkOrder(0b1, empty_chunk[0..], 0, false, false)?;
    if (empty_res.filesSlice().len != 0 || empty_res.partRangesSlice().len != 0 || empty_res.totalParts() != 0) {
        return err.ExpectedEmptyResult;
    }
}

test("js_order: multi-part interleaved import round-trip integrity") {
    let g = JsOrderGraph.init();

    // 验证细粒度 Part 级交错输出：
    // File A(1) 有 part 1 (无依赖), part 2 (依赖 File B(2)), part 3 (无依赖)
    // 期望输出区间顺序：A[1,2) -> B[1,3) -> A[2,4)
    const s_b = g.addFile(2, 1, 2, 0b1, JsPartWrapKind.None)?;
    * = g.addPart(s_b, false, true)?;
    * = g.addPart(s_b, true, true)?; // B part 1
    * = g.addPart(s_b, true, true)?; // B part 2 -> coalesced [1, 3)

    const s_a = g.addFile(1, 0, 1, 0b1, JsPartWrapKind.None)?;
    * = g.addPart(s_a, false, true)?;
    * = g.addPart(s_a, true, true)?; // A part 1 (before B)
    const p_a2 = g.addPart(s_a, true, true)?; // A part 2 (imports B, emitted after B)
    * = g.addPart(s_a, true, true)?; // A part 3 (coalesced with A part 2 -> [2, 4))
    g.addPartImport(s_a, p_a2, 2, JsOrderImportKind.Stmt, false)?;

    const chunk_files: [2]u32 = [1, 2];
    const res = g.computeChunkOrder(0b1, chunk_files[0..], 0, false, false)?;

    const ranges = res.partRangesSlice();
    if (ranges.len != 3) return err.InterleavedRangeCountMismatch;
    if (ranges[0].source_index != 1 || ranges[0].part_index_begin != 1 || ranges[0].part_index_end != 2) {
        return err.InterleavedRange0Mismatch;
    }
    if (ranges[1].source_index != 2 || ranges[1].part_index_begin != 1 || ranges[1].part_index_end != 3) {
        return err.InterleavedRange1Mismatch;
    }
    if (ranges[2].source_index != 1 || ranges[2].part_index_begin != 2 || ranges[2].part_index_end != 4) {
        return err.InterleavedRange2Mismatch;
    }
    // 总部件数 = 1 + 2 + 2 = 5，与所有存活非 namespace 部件总数精确一致
    if (res.totalParts() != 5) return err.InterleavedTotalMismatch;
    if (res.prefixRangesSlice().len + res.mainRangesSlice().len != ranges.len) {
        return err.PartitionSumMismatch;
    }
}
