// cap/test/test_bundler_chunk_renamer_production_deep.cp
// 深度生产级回归测试：cap:bundler Chunk 级符号重命名与字符频次压缩协调器 (lib/bundler/chunk_renamer.cp)

import { test } from "cap:test";
import {
    CHUNK_RENAME_MAX_RESERVED,
    ChunkStableRef,
    ChunkSlotCounts,
    ChunkCharFreq,
    ChunkRenameSymbolKind,
    ChunkRenameSymbolInput,
    ChunkRenameOptions,
    ChunkSymbolRenamer,
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

test("chunk_renamer: frequency-driven minification assigns top char-freq letters to hottest symbols") {
    let ren = ChunkSymbolRenamer.init();
    let f = ChunkCharFreq.init();
    // 让 'z' 成为最高频字母，'q' 次之
    f.scan("zzzzzzzzzzqqqqq");
    ren.includeCharFreq(f);

    const r_cold = ChunkStableRef{ stable_source_index: 1, inner_index: 0 };
    const r_hot = ChunkStableRef{ stable_source_index: 2, inner_index: 0 };
    * = ren.addSymbol(ChunkRenameSymbolInput{ sym_ref: r_cold, original_name: "coldVar", use_count: 2 })?;
    * = ren.addSymbol(ChunkRenameSymbolInput{ sym_ref: r_hot, original_name: "hotVar", use_count: 50 })?;

    const res = ren.rename(ChunkRenameOptions{
        minify_identifiers: true,
        use_char_freq_alphabet: true,
    })?;

    if (n_hot := res.lookupName(r_hot)) {
        if (!bytesEq(n_hot, "z")) { return err.HotSymbolShouldGetTopFreqChar; }
    } else { return err.HotSymbolMissing; }

    if (n_cold := res.lookupName(r_cold)) {
        if (!bytesEq(n_cold, "q")) { return err.ColdSymbolShouldGetSecondFreqChar; }
    } else { return err.ColdSymbolMissing; }
}

test("chunk_renamer: nested slot high-water mark offsets top-level minified slots") {
    let ren = ChunkSymbolRenamer.init();
    ren.mergeNestedSlots(ChunkSlotCounts{ default_slots: 2, label_slots: 1 });
    ren.mergeNestedSlots(ChunkSlotCounts{ default_slots: 3, label_slots: 0 });

    const r0 = ChunkStableRef{ stable_source_index: 1, inner_index: 0 };
    * = ren.addSymbol(ChunkRenameSymbolInput{ sym_ref: r0, original_name: "topVar", use_count: 10 })?;

    const res = ren.rename(ChunkRenameOptions{
        minify_identifiers: true,
        use_char_freq_alphabet: false,
    })?;

    // 起始槽位为 3 -> 默认字母表第 3 位是 'd' (0=a, 1=b, 2=c, 3=d)
    if (res.starting_top_level_slot != 3) { return err.StartSlotMismatch; }
    if (n0 := res.lookupName(r0)) {
        if (!bytesEq(n0, "d")) { return err.OffsetMinifiedNameMismatch; }
    } else { return err.OffsetSymbolMissing; }
}

test("chunk_renamer: non-minified mode resolves collisions with numeric suffixes and prioritizes cross-chunk imports") {
    let ren = ChunkSymbolRenamer.init();
    // 模块 2 的跨 Chunk 导入 `helper` 应优先保留原名 `helper`
    const r_local = ChunkStableRef{ stable_source_index: 1, inner_index: 0 };
    const r_import = ChunkStableRef{ stable_source_index: 2, inner_index: 0 };
    const r_local2 = ChunkStableRef{ stable_source_index: 3, inner_index: 0 };

    * = ren.addSymbol(ChunkRenameSymbolInput{
        sym_ref: r_local,
        original_name: "helper",
        kind: ChunkRenameSymbolKind.TopLevel,
    })?;
    * = ren.addSymbol(ChunkRenameSymbolInput{
        sym_ref: r_import,
        original_name: "helper",
        kind: ChunkRenameSymbolKind.CrossChunkImport,
    })?;
    * = ren.addSymbol(ChunkRenameSymbolInput{
        sym_ref: r_local2,
        original_name: "helper",
        kind: ChunkRenameSymbolKind.TopLevel,
    })?;

    const res = ren.rename(ChunkRenameOptions{ minify_identifiers: false })?;
    if (n_imp := res.lookupName(r_import)) {
        if (!bytesEq(n_imp, "helper")) { return err.CrossChunkImportShouldKeepOriginal; }
    } else { return err.ImportMissing; }
    if (n_l1 := res.lookupName(r_local)) {
        if (!bytesEq(n_l1, "helper2")) { return err.FirstCollisionShouldBe2; }
    } else { return err.Local1Missing; }
    if (n_l2 := res.lookupName(r_local2)) {
        if (!bytesEq(n_l2, "helper3")) { return err.SecondCollisionShouldBe3; }
    } else { return err.Local2Missing; }
}

test("chunk_renamer: skips reserved words, CJS globals and must_not_be_renamed symbols") {
    let ren = ChunkSymbolRenamer.init();
    * = ren.addReservedName("a")?;

    const r_fixed = ChunkStableRef{ stable_source_index: 1, inner_index: 0 };
    const r_normal = ChunkStableRef{ stable_source_index: 2, inner_index: 0 };
    const r_cjs_conflict = ChunkStableRef{ stable_source_index: 3, inner_index: 0 };

    * = ren.addSymbol(ChunkRenameSymbolInput{
        sym_ref: r_fixed,
        original_name: "b",
        must_not_be_renamed: true,
    })?;
    * = ren.addSymbol(ChunkRenameSymbolInput{
        sym_ref: r_normal,
        original_name: "myVal",
        use_count: 10,
    })?;

    // Minify 模式：'a' 在 reserved 中，'b' 被 must_not_be_renamed 锁定 -> r_normal 必须跳过 a/b 获得 'c'
    const min_res = ren.rename(ChunkRenameOptions{
        minify_identifiers: true,
        use_char_freq_alphabet: false,
    })?;
    if (nf := min_res.lookupName(r_fixed)) {
        if (!bytesEq(nf, "b")) { return err.FixedSymbolWasRenamed; }
    } else { return err.FixedMissing; }
    if (nn := min_res.lookupName(r_normal)) {
        if (!bytesEq(nn, "c")) { return err.ShouldSkipReservedAndFixed; }
    } else { return err.NormalMissing; }

    // CJS 输出模式：原名为 "exports" 时非压缩模式必须避让为 "exports2"
    * = ren.addSymbol(ChunkRenameSymbolInput{
        sym_ref: r_cjs_conflict,
        original_name: "exports",
    })?;
    const cjs_res = ren.rename(ChunkRenameOptions{
        minify_identifiers: false,
        is_cjs_output: true,
    })?;
    if (ne := cjs_res.lookupName(r_cjs_conflict)) {
        if (!bytesEq(ne, "exports2")) { return err.CjsGlobalNotAvoided; }
    } else { return err.CjsConflictMissing; }
}

test("chunk_renamer: deterministic tie-breaking produces identical output regardless of insertion order") {
    const r1 = ChunkStableRef{ stable_source_index: 1, inner_index: 0 };
    const r2 = ChunkStableRef{ stable_source_index: 2, inner_index: 0 };

    let a = ChunkSymbolRenamer.init();
    * = a.addSymbol(ChunkRenameSymbolInput{ sym_ref: r1, original_name: "alpha", use_count: 5 })?;
    * = a.addSymbol(ChunkRenameSymbolInput{ sym_ref: r2, original_name: "beta", use_count: 5 })?;
    const ra = a.rename(ChunkRenameOptions{ minify_identifiers: true, use_char_freq_alphabet: false })?;

    let b = ChunkSymbolRenamer.init();
    * = b.addSymbol(ChunkRenameSymbolInput{ sym_ref: r2, original_name: "beta", use_count: 5 })?;
    * = b.addSymbol(ChunkRenameSymbolInput{ sym_ref: r1, original_name: "alpha", use_count: 5 })?;
    const rb = b.rename(ChunkRenameOptions{ minify_identifiers: true, use_char_freq_alphabet: false })?;

    const a1 = ra.lookupName(r1) ?? return err.A1Missing;
    const b1 = rb.lookupName(r1) ?? return err.B1Missing;
    const a2 = ra.lookupName(r2) ?? return err.A2Missing;
    const b2 = rb.lookupName(r2) ?? return err.B2Missing;
    if (!bytesEq(a1, b1) || !bytesEq(a2, b2)) {
        return err.NonDeterministicTieBreak;
    }
}

test("chunk_renamer: empty chunk and malformed/capacity boundary guards") {
    let ren = ChunkSymbolRenamer.init();
    const empty_res = ren.rename(ChunkRenameOptions{ minify_identifiers: true })?;
    if (empty_res.entry_count != 0) { return err.EmptyChunkShouldYieldZero; }

    // 空符号名拒绝
    let empty_name_err = false;
    ren.addSymbol(ChunkRenameSymbolInput{
        sym_ref: ChunkStableRef{ stable_source_index: 1, inner_index: 0 },
        original_name: "",
    }) catch {
        empty_name_err = true;
    };
    if (!empty_name_err) { return err.EmptySymbolNameNotRejected; }

    // 重复 StableRef 拒绝
    const r_dup = ChunkStableRef{ stable_source_index: 9, inner_index: 1 };
    * = ren.addSymbol(ChunkRenameSymbolInput{ sym_ref: r_dup, original_name: "x" })?;
    let dup_ref_err = false;
    ren.addSymbol(ChunkRenameSymbolInput{ sym_ref: r_dup, original_name: "y" }) catch {
        dup_ref_err = true;
    };
    if (!dup_ref_err) { return err.DuplicateRefNotRejected; }

    // 保留名容量超限拦截
    const rnames: [33]([]const u8) = [
        "r01", "r02", "r03", "r04", "r05", "r06", "r07", "r08",
        "r09", "r10", "r11", "r12", "r13", "r14", "r15", "r16",
        "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24",
        "r25", "r26", "r27", "r28", "r29", "r30", "r31", "r32",
        "r33",
    ];
    let i: usize = 0;
    while (i < CHUNK_RENAME_MAX_RESERVED) {
        * = ren.addReservedName(rnames[i])?;
        i += 1;
    }
    let res_overflow = false;
    ren.addReservedName(rnames[32]) catch {
        res_overflow = true;
    };
    if (!res_overflow) { return err.ReservedOverflowNotRejected; }
}
