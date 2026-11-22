// Production Deep Tests for cap:bundler OutputFileListBuilder (lib/bundler/output_file_list.cp)
import { test, expect_eq } from "cap:test";
import {
    MAX_OUTPUT_FILES,
    OutputFileListError,
    ChunkContentKind,
    ChunkDescriptor,
    OutputLayoutOptions,
    OutputFileCapacity,
    OutputFileSlotKind,
    OutputFileRecord,
    calculateOutputFileListCapacity,
    OutputFileListBuilder,
} from "cap:bundler";

test("bundler output_file_list: 3-segment layout & O(1) chunk index invariant", fn() {
    const chunks = [
        ChunkDescriptor{ content_kind: .javascript, has_external_sourcemap: true, is_js_like_loader: true },
        ChunkDescriptor{ content_kind: .css, has_external_sourcemap: true, is_js_like_loader: false },
        ChunkDescriptor{ content_kind: .javascript, has_external_sourcemap: false, is_js_like_loader: true },
    ];
    const opts = OutputLayoutOptions{
        external_source_maps: true,
        generate_bytecode_cache: true,
        is_esm_output: false,
        is_compile_mode: false,
        compile_to_standalone_html: false,
        additional_output_files_count: 2,
    };

    const cap = calculateOutputFileListCapacity(opts, chunks[0..]);
    expect_eq(cap.chunk_count, 3);
    expect_eq(cap.sourcemap_count, 2);
    expect_eq(cap.bytecode_count, 2);
    expect_eq(cap.module_info_count, 0);
    expect_eq(cap.supplementary_count, 4);
    expect_eq(cap.additional_count, 2);
    expect_eq(cap.total_len, 9);

    let builder = OutputFileListBuilder.init(cap) catch unreachable;
    // Interleave chunk & supplementary insertions; chunk indices MUST remain 0, 1, 2
    const c0 = builder.insertForChunk(OutputFileRecord{ path: "entry.js", slot_kind: .chunk, byte_size: 1024 }) catch unreachable;
    const s0 = builder.insertForSourcemapOrBytecode(OutputFileRecord{ path: "entry.js.map", slot_kind: .sourcemap, byte_size: 512, source_chunk_index: 0 }) catch unreachable;
    const c1 = builder.insertForChunk(OutputFileRecord{ path: "style.css", slot_kind: .chunk, byte_size: 300 }) catch unreachable;
    const s1 = builder.insertForSourcemapOrBytecode(OutputFileRecord{ path: "style.css.map", slot_kind: .sourcemap, byte_size: 200, source_chunk_index: 1 }) catch unreachable;
    const b0 = builder.insertForSourcemapOrBytecode(OutputFileRecord{ path: "entry.jsc", slot_kind: .bytecode, byte_size: 900, source_chunk_index: 0 }) catch unreachable;
    const c2 = builder.insertForChunk(OutputFileRecord{ path: "chunk.js", slot_kind: .chunk, byte_size: 700 }) catch unreachable;
    const b2 = builder.insertForSourcemapOrBytecode(OutputFileRecord{ path: "chunk.jsc", slot_kind: .bytecode, byte_size: 400, source_chunk_index: 2 }) catch unreachable;

    expect_eq(c0, 0);
    expect_eq(c1, 1);
    expect_eq(c2, 2);
    expect_eq(s0, 3);
    expect_eq(s1, 4);
    expect_eq(b0, 5);
    expect_eq(b2, 6);

    const extras = [
        OutputFileRecord{ path: "logo.png", slot_kind: .additional, byte_size: 4096 },
        OutputFileRecord{ path: "font.woff2", slot_kind: .additional, byte_size: 8192 },
    ];
    builder.insertAdditionalOutputFiles(extras[0..]) catch unreachable;

    expect_eq(builder.isComplete(), true);
    const all = builder.finish() catch unreachable;
    expect_eq(all.len, 9);
    expect_eq(builder.chunksSlice().len, 3);
    expect_eq(builder.supplementarySlice().len, 4);
    expect_eq(builder.additionalSlice().len, 2);

    // O(1) lookup by chunk index
    if (f1 := builder.getForChunk(1)) {
        expect_eq(f1.path, "style.css");
    } else {
        unreachable;
    }
    expect_eq(builder.getForChunk(3) == null, true);
});

test("bundler output_file_list: ESM --compile bytecode reserves companion module_info slots", fn() {
    const chunks = [
        ChunkDescriptor{ content_kind: .javascript, has_external_sourcemap: false, is_js_like_loader: true },
        ChunkDescriptor{ content_kind: .javascript, has_external_sourcemap: false, is_js_like_loader: false },
    ];
    const opts = OutputLayoutOptions{
        external_source_maps: false,
        generate_bytecode_cache: true,
        is_esm_output: true,
        is_compile_mode: true,
        compile_to_standalone_html: false,
        additional_output_files_count: 0,
    };

    const cap = calculateOutputFileListCapacity(opts, chunks[0..]);
    expect_eq(cap.chunk_count, 2);
    expect_eq(cap.bytecode_count, 1);
    expect_eq(cap.module_info_count, 1);
    expect_eq(cap.supplementary_count, 2);
    expect_eq(cap.total_len, 4);
});

test("bundler output_file_list: compile_to_standalone_html zeroes additional_output_files", fn() {
    const chunks = [
        ChunkDescriptor{ content_kind: .html, has_external_sourcemap: false, is_js_like_loader: false },
    ];
    const opts = OutputLayoutOptions{
        external_source_maps: false,
        generate_bytecode_cache: false,
        compile_to_standalone_html: true,
        additional_output_files_count: 5,
    };

    const cap = calculateOutputFileListCapacity(opts, chunks[0..]);
    expect_eq(cap.additional_count, 0);
    expect_eq(cap.total_len, 1);
});

test("bundler output_file_list: rejects supplementary insert when none reserved & region overflow", fn() {
    const chunks = [
        ChunkDescriptor{ content_kind: .javascript, has_external_sourcemap: false, is_js_like_loader: true },
    ];
    const opts = OutputLayoutOptions{};
    let builder = OutputFileListBuilder.initFromLayout(opts, chunks[0..]) catch unreachable;

    let no_supp = false;
    * = builder.insertForSourcemapOrBytecode(OutputFileRecord{ path: "a.map", slot_kind: .sourcemap }) catch {
        no_supp = true;
    };
    expect_eq(no_supp, true);

    * = builder.insertForChunk(OutputFileRecord{ path: "a.js", slot_kind: .chunk }) catch unreachable;

    // Second chunk insert must return ChunkRegionFull
    let chunk_full = false;
    * = builder.insertForChunk(OutputFileRecord{ path: "b.js", slot_kind: .chunk }) catch {
        chunk_full = true;
    };
    expect_eq(chunk_full, true);
});

test("bundler output_file_list: rejects incomplete finish() & mismatched additional count", fn() {
    const chunks = [
        ChunkDescriptor{ content_kind: .javascript, has_external_sourcemap: true, is_js_like_loader: true },
    ];
    const opts = OutputLayoutOptions{
        external_source_maps: true,
        additional_output_files_count: 2,
    };
    let builder = OutputFileListBuilder.initFromLayout(opts, chunks[0..]) catch unreachable;
    * = builder.insertForChunk(OutputFileRecord{ path: "app.js", slot_kind: .chunk }) catch unreachable;

    // Incomplete list must fail finish()
    expect_eq(builder.isComplete(), false);
    let incomplete_err = false;
    * = builder.finish() catch { incomplete_err = true; };
    expect_eq(incomplete_err, true);

    // Passing 1 additional file when 2 were reserved must fail
    const wrong_extras = [OutputFileRecord{ path: "only-one.png", slot_kind: .additional }];
    let mismatch_err = false;
    * = builder.insertAdditionalOutputFiles(wrong_extras[0..]) catch { mismatch_err = true; };
    expect_eq(mismatch_err, true);
});

test("bundler output_file_list: rejects capacity exceeding MAX_OUTPUT_FILES", fn() {
    expect_eq(MAX_OUTPUT_FILES, 128);
    const huge_cap = OutputFileCapacity{
        total_len: 200,
        chunk_count: 200,
        sourcemap_count: 0,
        bytecode_count: 0,
        module_info_count: 0,
        supplementary_count: 0,
        additional_count: 0,
    };
    let cap_err = false;
    * = OutputFileListBuilder.init(huge_cap) catch { cap_err = true; };
    expect_eq(cap_err, true);
});
