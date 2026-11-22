// cap/test/test_bundler_chunk_planner_production_deep.cp
// 深度生产级回归测试：cap:bundler Chunk 规划与确定性排序引擎 (lib/bundler/chunk_planner.cp)

import { test } from "cap:test";
import {
    CHUNK_PLAN_MAX_CSS_DEPS,
    CHUNK_PLAN_INVALID_INDEX,
    ChunkPlanContentKind,
    ChunkPlanLoaderKind,
    ChunkPlanEntryKind,
    ChunkPlanTarget,
    ChunkPlanTemplateKind,
    ChunkPlanOptions,
    ChunkPlanFileInput,
    hashCssOrderKey,
    formatChunkUniqueKey,
    ChunkPlanner,
} from "cap:bundler";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

test("chunk_planner: code splitting extracts shared chunk and preserves primary entry order") {
    let p = ChunkPlanner.init();
    * = p.addFile(ChunkPlanFileInput{ source_index: 10, entry_bits: 0b01, output_base_name: "entry-a" })?;
    * = p.addFile(ChunkPlanFileInput{ source_index: 20, entry_bits: 0b10, output_base_name: "entry-b" })?;
    * = p.addFile(ChunkPlanFileInput{ source_index: 30, entry_bits: 0b11, output_base_name: "shared" })?;
    * = p.addFile(ChunkPlanFileInput{ source_index: 40, entry_bits: 0b11, is_live: false, output_base_name: "dead" })?;

    const e0 = p.addEntryPoint(10)?;
    const e1 = p.addEntryPoint(20)?;
    if (e0 != 0 || e1 != 1) { return err.EntryIdMismatch; }

    const res = p.computeChunks(ChunkPlanOptions{
        code_splitting: true,
        unique_key: 0xbeef,
    })?;

    if (res.chunk_count != 3 || res.js_chunk_count != 3) { return err.ChunkCountMismatch; }
    const chunks = res.slice();

    // 主入口 entry_point_id==0 排在首位
    if (!chunks[0].is_entry_point || chunks[0].source_index != 10) { return err.PrimaryOrderMismatch; }
    if (!chunks[0].containsFile(10) || chunks[0].containsFile(30)) { return err.EntryAFileMismatch; }
    if (chunks[0].template_kind != ChunkPlanTemplateKind.EntryFile) { return err.EntryATemplateMismatch; }

    // 次入口 entry_point_id==1
    if (!chunks[1].is_entry_point || chunks[1].source_index != 20) { return err.EntryBOrderMismatch; }
    if (!chunks[1].containsFile(20) || chunks[1].containsFile(30)) { return err.EntryBFileMismatch; }

    // 共享 Chunk（entry_bits == 0b11，包含 30 且不包含死文件 40）
    if (chunks[2].is_entry_point || chunks[2].entry_bits != 0b11) { return err.SharedChunkMetaMismatch; }
    if (!chunks[2].containsFile(30) || chunks[2].containsFile(40)) { return err.SharedChunkFilesMismatch; }
    if (chunks[2].template_kind != ChunkPlanTemplateKind.SharedChunk) { return err.SharedTemplateMismatch; }

    // 反向映射校验
    if (c0 := res.findChunkForEntrySource(10)) {
        if (c0 != 0) { return err.EntryMap0Mismatch; }
    } else {
        return err.EntryMap0Missing;
    }
    if (c1 := res.findChunkForEntrySource(20)) {
        if (c1 != 1) { return err.EntryMap1Mismatch; }
    } else {
        return err.EntryMap1Missing;
    }
    if (res.findChunkForEntrySource(30) != null) { return err.SharedShouldNotBeEntry; }
}

test("chunk_planner: non-splitting fans out shared live files and skips dead files") {
    let p = ChunkPlanner.init();
    * = p.addFile(ChunkPlanFileInput{ source_index: 1, entry_bits: 0b01, output_base_name: "a" })?;
    * = p.addFile(ChunkPlanFileInput{ source_index: 2, entry_bits: 0b10, output_base_name: "b" })?;
    * = p.addFile(ChunkPlanFileInput{ source_index: 3, entry_bits: 0b11, output_base_name: "common" })?;
    * = p.addFile(ChunkPlanFileInput{ source_index: 4, entry_bits: 0b11, is_live: false, output_base_name: "unused" })?;
    * = p.addEntryPoint(1)?;
    * = p.addEntryPoint(2)?;

    const res = p.computeChunks(ChunkPlanOptions{
        code_splitting: false,
        unique_key: 0x1234,
    })?;

    if (res.chunk_count != 2 || res.js_chunk_count != 2) { return err.NonSplitCountMismatch; }
    const chunks = res.slice();
    if (!chunks[0].containsFile(1) || !chunks[0].containsFile(3) || chunks[0].containsFile(4)) {
        return err.NonSplitChunk0Mismatch;
    }
    if (!chunks[1].containsFile(2) || !chunks[1].containsFile(3) || chunks[1].containsFile(4)) {
        return err.NonSplitChunk1Mismatch;
    }
}

test("chunk_planner: css chunking deduplicates identical ordered css deps and remaps js->css indices") {
    let p = ChunkPlanner.init();
    * = p.addFile(ChunkPlanFileInput{ source_index: 1, output_base_name: "page1" })?;
    * = p.addFile(ChunkPlanFileInput{ source_index: 2, output_base_name: "page2" })?;
    * = p.addFile(ChunkPlanFileInput{ source_index: 3, output_base_name: "page3" })?;
    * = p.addEntryPoint(1)?;
    * = p.addEntryPoint(2)?;
    * = p.addEntryPoint(3)?;

    // page1 和 page2 引用完全相同的有序 CSS 列表 [100, 101] -> 应共享 1 个 CSS Chunk
    * = p.attachCssDep(1, 100)?;
    * = p.attachCssDep(1, 101)?;
    * = p.attachCssDep(2, 100)?;
    * = p.attachCssDep(2, 101)?;
    // page3 引用不同的 CSS 列表 [102] -> 独立的第 2 个 CSS Chunk
    * = p.attachCssDep(3, 102)?;

    const res = p.computeChunks(ChunkPlanOptions{
        code_splitting: true,
        css_chunking: true,
        unique_key: 0xab,
    })?;

    if (res.js_chunk_count != 3 || res.css_chunk_count != 2 || res.chunk_count != 5) {
        return err.CssChunkDedupCountMismatch;
    }

    const chunks = res.slice();
    // JS chunk 0 和 1 必须指向同一个重映射后的 CSS chunk 索引（且 >= 3）
    const css_idx_0 = chunks[0].css_chunk_index;
    const css_idx_1 = chunks[1].css_chunk_index;
    const css_idx_2 = chunks[2].css_chunk_index;
    if (css_idx_0 == CHUNK_PLAN_INVALID_INDEX || css_idx_0 != css_idx_1) {
        return err.SharedCssRemapMismatch;
    }
    if (css_idx_2 == CHUNK_PLAN_INVALID_INDEX || css_idx_2 == css_idx_0) {
        return err.DistinctCssRemapMismatch;
    }

    // 验证重映射索引确实指向对应含正确 CSS 源文件的 CSS Chunk，且 CSS Chunk 按 css_hash 升序排列
    if (chunks[@as(usize, css_idx_0)].content != ChunkPlanContentKind.Css) { return err.RemapTargetNotCss; }
    if (!chunks[@as(usize, css_idx_0)].containsFile(100) || !chunks[@as(usize, css_idx_0)].containsFile(101)) {
        return err.RemapSharedCssFilesMismatch;
    }
    if (!chunks[@as(usize, css_idx_2)].containsFile(102)) {
        return err.RemapDistinctCssFilesMismatch;
    }
    if (chunks[3].css_hash > chunks[4].css_hash) {
        return err.CssHashSortOrderViolated;
    }

    // 验证有序哈希敏感性：[100, 101] != [101, 100]
    const ord_a: [2]u32 = [100, 101];
    const ord_b: [2]u32 = [101, 100];
    if (hashCssOrderKey(ord_a[0..]) == hashCssOrderKey(ord_b[0..])) {
        return err.CssOrderHashCollision;
    }
}

test("chunk_planner: css_chunking disabled separates css chunks by entry_bits and pure css entry maps back") {
    let p = ChunkPlanner.init();
    * = p.addFile(ChunkPlanFileInput{
        source_index: 50,
        loader: ChunkPlanLoaderKind.Css,
        output_base_name: "theme-light",
    })?;
    * = p.addFile(ChunkPlanFileInput{
        source_index: 51,
        loader: ChunkPlanLoaderKind.Css,
        output_base_name: "theme-dark",
    })?;
    * = p.addEntryPoint(50)?;
    * = p.addEntryPoint(51)?;
    * = p.attachCssDep(50, 200)?;
    * = p.attachCssDep(51, 200)?;

    const res = p.computeChunks(ChunkPlanOptions{
        code_splitting: false,
        css_chunking: false,
        unique_key: 0x7,
    })?;

    if (res.js_chunk_count != 0 || res.css_chunk_count != 2 || res.chunk_count != 2) {
        return err.PureCssCountMismatch;
    }
    if (res.findChunkForEntrySource(50) == null || res.findChunkForEntrySource(51) == null) {
        return err.PureCssEntryMapMissing;
    }
    if (!bytesEq(res.slice()[0].ext, "css") || !bytesEq(res.slice()[1].ext, "css")) {
        return err.PureCssExtMismatch;
    }
}

test("chunk_planner: html entry interleaves paired html chunk after js chunk and selects templates") {
    let p = ChunkPlanner.init();
    * = p.addFile(ChunkPlanFileInput{
        source_index: 7,
        loader: ChunkPlanLoaderKind.Html,
        output_base_name: "index",
    })?;
    * = p.addFile(ChunkPlanFileInput{
        source_index: 8,
        loader: ChunkPlanLoaderKind.Js,
        entry_kind: ChunkPlanEntryKind.DynamicImport,
        output_base_name: "lazy",
    })?;
    * = p.addEntryPoint(7)?;
    * = p.addEntryPoint(8)?;

    const res = p.computeChunks(ChunkPlanOptions{
        code_splitting: true,
        unique_key: 0x42,
    })?;

    if (res.js_chunk_count != 2 || res.html_chunk_count != 1 || res.chunk_count != 3) {
        return err.HtmlInterleaveCountMismatch;
    }
    const chunks = res.slice();
    // chunks[0] 是 HTML 入口对应的 JS Chunk（has_html_chunk=true -> 命名模板退化为 SharedChunk）
    if (chunks[0].content != ChunkPlanContentKind.Javascript || !chunks[0].has_html_chunk) {
        return err.HtmlPrimaryJsMismatch;
    }
    if (chunks[0].template_kind != ChunkPlanTemplateKind.SharedChunk) {
        return err.HtmlJsTemplateMismatch;
    }
    // chunks[1] 紧随其后为配对的 HTML Chunk（命名模板为 EntryFile）
    if (chunks[1].content != ChunkPlanContentKind.Html || chunks[1].source_index != 7) {
        return err.PairedHtmlNotInterleaved;
    }
    if (chunks[1].template_kind != ChunkPlanTemplateKind.EntryFile || !bytesEq(chunks[1].ext, "html")) {
        return err.PairedHtmlTemplateMismatch;
    }
    // chunks[2] 是动态 import 入口 -> 命名模板为 SharedChunk
    if (chunks[2].source_index != 8 || chunks[2].template_kind != ChunkPlanTemplateKind.SharedChunk) {
        return err.DynamicImportTemplateMismatch;
    }
}

test("chunk_planner: server build with html imports propagates browser flag and target templates") {
    let p = ChunkPlanner.init();
    * = p.addFile(ChunkPlanFileInput{
        source_index: 1,
        target: ChunkPlanTarget.Bun,
        entry_bits: 0b01,
        output_base_name: "server",
    })?;
    * = p.addFile(ChunkPlanFileInput{
        source_index: 2,
        target: ChunkPlanTarget.Browser,
        entry_bits: 0b10,
        output_base_name: "client",
    })?;
    // 共享模块在服务端与浏览器入口间共享，其自身 target 为 Browser -> 应向共享 Chunk 传播 browser 标记
    * = p.addFile(ChunkPlanFileInput{
        source_index: 3,
        target: ChunkPlanTarget.Bun,
        entry_bits: 0b11,
        output_base_name: "shared-s",
    })?;
    * = p.addFile(ChunkPlanFileInput{
        source_index: 4,
        target: ChunkPlanTarget.Browser,
        entry_bits: 0b11,
        output_base_name: "shared-b",
    })?;
    * = p.addEntryPoint(1)?;
    * = p.addEntryPoint(2)?;

    const res = p.computeChunks(ChunkPlanOptions{
        code_splitting: true,
        target: ChunkPlanTarget.Bun,
        has_html_imports: true,
        has_server_html_imports: true,
        has_custom_entry_naming: false,
        has_custom_chunk_naming: false,
        unique_key: 0x99,
    })?;

    if (res.chunk_count != 3) { return err.ServerBuildChunkCountMismatch; }
    const chunks = res.slice();

    // chunks[0] = server (bun)
    if (chunks[0].is_browser_chunk_from_server_build) { return err.ServerChunkShouldNotBeBrowser; }
    if (chunks[0].template_kind != ChunkPlanTemplateKind.EntryFileWithTarget || !chunks[0].template_kind.needsTarget()) {
        return err.ServerEntryTemplateMismatch;
    }
    if (!bytesEq(chunks[0].target_label, "bun")) { return err.ServerTargetLabelMismatch; }

    // chunks[1] = client (browser)
    if (!chunks[1].is_browser_chunk_from_server_build) { return err.ClientChunkShouldBeBrowser; }
    if (!bytesEq(chunks[1].target_label, "browser")) { return err.ClientTargetLabelMismatch; }

    // chunks[2] = shared (先遇 source 3 target=Bun，再遇 source 4 target=Browser -> 传播置位)
    if (!chunks[2].is_browser_chunk_from_server_build) { return err.SharedBrowserPropFailed; }
    if (chunks[2].template_kind != ChunkPlanTemplateKind.SharedChunkWithTarget) {
        return err.SharedWithTargetTemplateMismatch;
    }
}

test("chunk_planner: unique key formatting and round-trip lookup across edge values") {
    const k0 = formatChunkUniqueKey(0, 0);
    if (!bytesEq(k0.asSlice(), "0C00000000") || !bytesEq(k0.prefix(), "0")) {
        return err.UniqueKeyZeroMismatch;
    }
    const k1 = formatChunkUniqueKey(0xdeadbeef, 42);
    if (!bytesEq(k1.asSlice(), "deadbeefC00000042") || !bytesEq(k1.prefix(), "deadbeef")) {
        return err.UniqueKeyHexMismatch;
    }
    const k2 = formatChunkUniqueKey(0xFFFFFFFFFFFFFFFF, 123456789);
    if (!bytesEq(k2.asSlice(), "ffffffffffffffffC123456789")) {
        return err.UniqueKeyMaxMismatch;
    }

    // Round-trip: 通过 findByUniqueKey 精确检索规划产物
    let p = ChunkPlanner.init();
    * = p.addFile(ChunkPlanFileInput{ source_index: 11, output_base_name: "main" })?;
    * = p.addEntryPoint(11)?;
    const res = p.computeChunks(ChunkPlanOptions{ unique_key: 0xcafe })?;
    if (found := res.findByUniqueKey("cafeC00000000")) {
        if (found.source_index != 11 || !bytesEq(found.base_name, "main")) {
            return err.RoundTripPayloadMismatch;
        }
    } else {
        return err.RoundTripLookupFailed;
    }
    if (res.findByUniqueKey("cafeC00000001") != null) {
        return err.NonExistentKeyShouldReturnNull;
    }
}

test("chunk_planner: malformed input and capacity boundary guards") {
    let p = ChunkPlanner.init();
    // 空图规划返回 0 chunks 不报错
    const empty_res = p.computeChunks(ChunkPlanOptions{})?;
    if (empty_res.chunk_count != 0) { return err.EmptyPlanShouldBeZero; }

    // 重复注册同一 source_index 被拒绝
    * = p.addFile(ChunkPlanFileInput{ source_index: 5 })?;
    let dup_file_rejected = false;
    p.addFile(ChunkPlanFileInput{ source_index: 5 }) catch {
        dup_file_rejected = true;
    };
    if (!dup_file_rejected) { return err.DuplicateFileNotRejected; }

    // 未注册的 source_index 不能添加为入口或附加 CSS 依赖
    let invalid_entry_rejected = false;
    p.addEntryPoint(999) catch {
        invalid_entry_rejected = true;
    };
    if (!invalid_entry_rejected) { return err.InvalidEntryNotRejected; }

    let invalid_css_rejected = false;
    p.attachCssDep(999, 1) catch {
        invalid_css_rejected = true;
    };
    if (!invalid_css_rejected) { return err.InvalidCssAttachNotRejected; }

    // 重复声明同一入口被拒绝
    * = p.addEntryPoint(5)?;
    let dup_entry_rejected = false;
    p.addEntryPoint(5) catch {
        dup_entry_rejected = true;
    };
    if (!dup_entry_rejected) { return err.DuplicateEntryNotRejected; }

    // 超出单文件最大 CSS 依赖数被安全拦截
    let i: usize = 0;
    while (i < CHUNK_PLAN_MAX_CSS_DEPS) {
        * = p.attachCssDep(5, @as(u32, @intCast(1000 + i)))?;
        i += 1;
    }
    let css_overflow_rejected = false;
    p.attachCssDep(5, 9999) catch {
        css_overflow_rejected = true;
    };
    if (!css_overflow_rejected) { return err.CssDepOverflowNotRejected; }
}
