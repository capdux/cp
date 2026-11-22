// cap/test/test_bundler_chunk_output_production_deep.cp
// Production-deep regression tests for cap:bundler chunk_output module
// Source: /data/dev/bun/src/bundler/Chunk.cp & /data/dev/bun/docs/bundler/standalone-html.mdx

import { test, expect } from "cap:test";
import {
    CHUNK_OUTPUT_MAX_PIECES,
    CHUNK_OUTPUT_MAX_REFS,
    CHUNK_DEBUG_ID_COMMENT_LEN,
    ChunkOutputError,
    ChunkPieceQueryKind,
    ChunkPieceQuery,
    ChunkInlineKind,
    ChunkRefTarget,
    ChunkOutputAssembleOptions,
    chunkClosingTagForKind,
    chunkExtensionForKind,
    countClosingTagOccurrences,
    escapeClosingTagsInto,
    normalizedChunkImportPathLen,
    normalizeChunkImportPath,
    formatChunkDebugIdComment,
    ChunkPieceAssembler,
} from "cap:bundler";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("bundler chunk_output: closing tag count & case-insensitive escaping preserves non-matching tags") {
    const js_in = "const a = '<div>hi</div></script></SCRIPT></ScRiPt></sp></';";
    expect(countClosingTagOccurrences(js_in, "</script") == 3);

    let buf: [256]u8 = [0; 256];
    const n = escapeClosingTagsInto(js_in, "</script", buf[0..]) catch {
        expect(false);
        return;
    };
    const expected = "const a = '<div>hi</div><\\/script><\\/SCRIPT><\\/ScRiPt></sp></';";
    expect(n == js_in.len + 3);
    expect(strEq(buf[0..n], expected));

    // CSS </style escaping
    const css_in = "body{content:'</style></STYLE></span>';}";
    expect(countClosingTagOccurrences(css_in, "</style") == 2);
    const cn = escapeClosingTagsInto(css_in, "</style", buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..cn], "body{content:'<\\/style><\\/STYLE></span>';}"));

    // No matches -> exact passthrough
    const plain = "console.log('hello');";
    expect(countClosingTagOccurrences(plain, "</script") == 0);
    const pn = escapeClosingTagsInto(plain, "</script", buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..pn], plain));
}

test("bundler chunk_output: cross-platform import prefix normalization") {
    let buf: [128]u8 = [0; 128];

    // Windows backslashes + duplicate ./
    const n1 = normalizeChunkImportPath("./", ".\\chunks\\app-1234.js", buf[0..]) catch {
        expect(false);
        return;
    };
    expect(n1 == normalizedChunkImportPathLen("./", ".\\chunks\\app-1234.js"));
    expect(strEq(buf[0..n1], "./chunks/app-1234.js"));

    // Leading slash deduplication with CDN prefix
    const n2 = normalizeChunkImportPath("https://cdn.example.com/", "/assets\\logo.png", buf[0..]) catch {
        expect(false);
        return;
    };
    expect(n2 == normalizedChunkImportPathLen("https://cdn.example.com/", "/assets\\logo.png"));
    expect(strEq(buf[0..n2], "https://cdn.example.com/assets/logo.png"));

    // Prefix without trailing slash inserts '/'
    const n3 = normalizeChunkImportPath("/static", "chunks/main.js", buf[0..]) catch {
        expect(false);
        return;
    };
    expect(n3 == normalizedChunkImportPathLen("/static", "chunks/main.js"));
    expect(strEq(buf[0..n3], "/static/chunks/main.js"));

    // Empty prefix preserves relative path with POSIX slashes
    const n4 = normalizeChunkImportPath("", "sub\\dir\\file.css", buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..n4], "sub/dir/file.css"));
}

test("bundler chunk_output: kind helpers & deterministic debugId comment") {
    expect(strEq(chunkClosingTagForKind(ChunkInlineKind.JavaScript), "</script"));
    expect(strEq(chunkClosingTagForKind(ChunkInlineKind.Css), "</style"));
    expect(strEq(chunkClosingTagForKind(ChunkInlineKind.Html), ""));

    expect(strEq(chunkExtensionForKind(ChunkInlineKind.JavaScript), "js"));
    expect(strEq(chunkExtensionForKind(ChunkInlineKind.Css), "css"));
    expect(strEq(chunkExtensionForKind(ChunkInlineKind.Html), "html"));

    let buf: [64]u8 = [0; 64];
    const n0 = formatChunkDebugIdComment(0, buf[0..]) catch {
        expect(false);
        return;
    };
    expect(n0 == CHUNK_DEBUG_ID_COMMENT_LEN);
    expect(strEq(buf[0..n0], "\n//# debugId=00000000-0000-0000-0000-000000000000\n"));

    const n1 = formatChunkDebugIdComment(0x1234_5678_9abc_def0, buf[0..]) catch {
        expect(false);
        return;
    };
    expect(n1 == CHUNK_DEBUG_ID_COMMENT_LEN);
    expect(strEq(buf[0..n1], "\n//# debugId=00000000-0000-0000-1234-56789abcdef0\n"));

    const n2 = formatChunkDebugIdComment(0xffff_ffff_ffff_ffff, buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..n2], "\n//# debugId=00000000-0000-0000-ffff-ffffffffffff\n"));
}

test("bundler chunk_output: normal mode assembly with Chunk, Asset, SCB, HtmlImport & debugId") {
    let asm = ChunkPieceAssembler.init();
    asm.addPiece("import x from \"", ChunkPieceQuery.init(ChunkPieceQueryKind.Chunk, 0)) catch { expect(false); return; };
    asm.addPiece("\";\nconst img = \"", ChunkPieceQuery.init(ChunkPieceQueryKind.Asset, 0)) catch { expect(false); return; };
    asm.addPiece("\";\nconst scb = \"", ChunkPieceQuery.init(ChunkPieceQueryKind.Scb, 0)) catch { expect(false); return; };
    asm.addPiece("\";\nconst m = ", ChunkPieceQuery.init(ChunkPieceQueryKind.HtmlImport, 0)) catch { expect(false); return; };
    asm.addLiteral(";") catch { expect(false); return; };

    const chunks: [1]ChunkRefTarget = [
        ChunkRefTarget{ rel_path: "chunks\\shared-ab12.js", kind: ChunkInlineKind.JavaScript },
    ];
    const assets: [1]ChunkRefTarget = [
        ChunkRefTarget{ rel_path: "./assets/hero.png", kind: ChunkInlineKind.Html },
    ];
    const scbs: [1]ChunkRefTarget = [
        ChunkRefTarget{ rel_path: "/client/button.js", kind: ChunkInlineKind.JavaScript },
    ];
    const html_imports: [1]ChunkRefTarget = [
        ChunkRefTarget{ inline_content: "{\"index\":\"/index.html\"}", kind: ChunkInlineKind.Html },
    ];

    const opts = ChunkOutputAssembleOptions{
        import_prefix: "./",
        standalone_mode: false,
        emit_debug_id: true,
        debug_id_hash: 0x00ab_cdef_0123_4567,
    };

    const est = asm.estimateOutputSize(chunks[0..], assets[0..], scbs[0..], html_imports[0..], opts) catch {
        expect(false);
        return;
    };
    let out: [512]u8 = [0; 512];
    const written = asm.assemble(chunks[0..], assets[0..], scbs[0..], html_imports[0..], opts, out[0..]) catch {
        expect(false);
        return;
    };
    expect(written == est);

    const expected =
        "import x from \"./chunks/shared-ab12.js\";\n" ++
        "const img = \"./assets/hero.png\";\n" ++
        "const scb = \"./client/button.js\";\n" ++
        "const m = {\"index\":\"/index.html\"};" ++
        "\n//# debugId=00000000-0000-0000-00ab-cdef01234567\n";
    expect(strEq(out[0..written], expected));
}

test("bundler chunk_output: standalone HTML inlining with closing tag escaping & data URI") {
    let asm = ChunkPieceAssembler.init();
    asm.addPiece("<!doctype html><html><head><style>", ChunkPieceQuery.init(ChunkPieceQueryKind.Chunk, 0)) catch { expect(false); return; };
    asm.addPiece("</style></head><body><img src=\"", ChunkPieceQuery.init(ChunkPieceQueryKind.Asset, 0)) catch { expect(false); return; };
    asm.addPiece("\"><script type=\"module\">", ChunkPieceQuery.init(ChunkPieceQueryKind.Chunk, 1)) catch { expect(false); return; };
    asm.addPiece("</script><a href=\"", ChunkPieceQuery.init(ChunkPieceQueryKind.Asset, 1)) catch { expect(false); return; };
    asm.addLiteral("\">ext</a></body></html>") catch { expect(false); return; };

    const chunks: [2]ChunkRefTarget = [
        ChunkRefTarget{
            rel_path: "styles.css",
            inline_content: "body::after{content:\"</STYLE>\"}",
            kind: ChunkInlineKind.Css,
        },
        ChunkRefTarget{
            rel_path: "app.js",
            inline_content: "console.log('</script>');",
            kind: ChunkInlineKind.JavaScript,
        },
    ];
    const assets: [2]ChunkRefTarget = [
        ChunkRefTarget{
            rel_path: "logo.png",
            inline_content: "data:image/png;base64,AAAA",
            kind: ChunkInlineKind.Html,
        },
        // Fallback when inline_content == null (e.g. external/non-inlined asset)
        ChunkRefTarget{
            rel_path: "docs/readme.txt",
            inline_content: null,
            kind: ChunkInlineKind.Html,
        },
    ];
    const empty: [0]ChunkRefTarget = [];
    const opts = ChunkOutputAssembleOptions{
        import_prefix: "./",
        standalone_mode: true,
        emit_debug_id: false,
    };

    const est = asm.estimateOutputSize(chunks[0..], assets[0..], empty[0..], empty[0..], opts) catch {
        expect(false);
        return;
    };
    let out: [512]u8 = [0; 512];
    const wn = asm.assemble(chunks[0..], assets[0..], empty[0..], empty[0..], opts, out[0..]) catch {
        expect(false);
        return;
    };
    expect(wn == est);

    const expected =
        "<!doctype html><html><head><style>body::after{content:\"<\\/STYLE>\"}</style></head>" ++
        "<body><img src=\"data:image/png;base64,AAAA\">" ++
        "<script type=\"module\">console.log('<\\/script>');</script>" ++
        "<a href=\"./docs/readme.txt\">ext</a></body></html>";
    expect(strEq(out[0..wn], expected));
}

test("bundler chunk_output: malformed inputs, invalid refs & buffer/capacity boundary guards") {
    let small: [4]u8 = [0; 4];

    // Malformed close tags
    expect(countClosingTagOccurrences("</script>", "") == 0);
    expect(countClosingTagOccurrences("</script>", "</") == 0);
    expect(countClosingTagOccurrences("</script>", "script") == 0);

    let bad_tag_err = false;
    * = escapeClosingTagsInto("abc", "</", small[0..]) catch { bad_tag_err = true; };
    expect(bad_tag_err);

    let bad_tag_err2 = false;
    * = escapeClosingTagsInto("abc", "script", small[0..]) catch { bad_tag_err2 = true; };
    expect(bad_tag_err2);

    // BufferTooSmall in escapeClosingTagsInto & formatChunkDebugIdComment & normalizeChunkImportPath
    let buf_err1 = false;
    * = escapeClosingTagsInto("</script>", "</script", small[0..]) catch { buf_err1 = true; };
    expect(buf_err1);

    let buf_err2 = false;
    * = formatChunkDebugIdComment(1, small[0..]) catch { buf_err2 = true; };
    expect(buf_err2);

    let buf_err3 = false;
    * = normalizeChunkImportPath("./", "long_chunk_name.js", small[0..]) catch { buf_err3 = true; };
    expect(buf_err3);

    // InvalidReferenceIndex for Chunk / Asset / SCB / HtmlImport
    const empty: [0]ChunkRefTarget = [];
    const opts = ChunkOutputAssembleOptions{};

    let a_chunk = ChunkPieceAssembler.init();
    a_chunk.addPiece("", ChunkPieceQuery.init(ChunkPieceQueryKind.Chunk, 0)) catch { expect(false); return; };
    let ref_err1 = false;
    * = a_chunk.estimateOutputSize(empty[0..], empty[0..], empty[0..], empty[0..], opts) catch { ref_err1 = true; };
    expect(ref_err1);

    let a_asset = ChunkPieceAssembler.init();
    a_asset.addPiece("", ChunkPieceQuery.init(ChunkPieceQueryKind.Asset, 1)) catch { expect(false); return; };
    let ref_err2 = false;
    * = a_asset.estimateOutputSize(empty[0..], empty[0..], empty[0..], empty[0..], opts) catch { ref_err2 = true; };
    expect(ref_err2);

    // MissingHtmlImportManifest
    let a_html = ChunkPieceAssembler.init();
    a_html.addPiece("", ChunkPieceQuery.init(ChunkPieceQueryKind.HtmlImport, 0)) catch { expect(false); return; };
    const bad_html: [1]ChunkRefTarget = [ChunkRefTarget{ rel_path: "x.html", inline_content: null }];
    let manifest_err = false;
    * = a_html.estimateOutputSize(empty[0..], empty[0..], empty[0..], bad_html[0..], opts) catch { manifest_err = true; };
    expect(manifest_err);

    // TooManyPieces capacity guard (64)
    let a_full = ChunkPieceAssembler.init();
    let i: usize = 0;
    while (i < CHUNK_OUTPUT_MAX_PIECES) {
        a_full.addLiteral("x") catch { expect(false); return; };
        i += 1;
    }
    expect(a_full.literalByteSize() == CHUNK_OUTPUT_MAX_PIECES);
    let cap_err = false;
    a_full.addLiteral("overflow") catch { cap_err = true; };
    expect(cap_err);
    expect(CHUNK_OUTPUT_MAX_REFS == 32);
}

test("bundler chunk_output: empty assembler & pure literal exact size invariant") {
    const empty: [0]ChunkRefTarget = [];
    let out: [128]u8 = [0; 128];

    // Completely empty assembler
    const asm0 = ChunkPieceAssembler.init();
    expect(asm0.literalByteSize() == 0);
    const opts0 = ChunkOutputAssembleOptions{ emit_debug_id: false };
    const n0 = asm0.assemble(empty[0..], empty[0..], empty[0..], empty[0..], opts0, out[0..]) catch {
        expect(false);
        return;
    };
    expect(n0 == 0);

    // Pure literal pieces + debugId
    let asm1 = ChunkPieceAssembler.init();
    asm1.addLiteral("export const v = ") catch { expect(false); return; };
    asm1.addLiteral("42;") catch { expect(false); return; };
    expect(asm1.literalByteSize() == 20);

    const opts1 = ChunkOutputAssembleOptions{ emit_debug_id: true, debug_id_hash: 0x0000_0000_0000_00ff };
    const est1 = asm1.estimateOutputSize(empty[0..], empty[0..], empty[0..], empty[0..], opts1) catch {
        expect(false);
        return;
    };
    expect(est1 == 20 + CHUNK_DEBUG_ID_COMMENT_LEN);
    const n1 = asm1.assemble(empty[0..], empty[0..], empty[0..], empty[0..], opts1, out[0..]) catch {
        expect(false);
        return;
    };
    expect(n1 == est1);
    expect(strEq(out[0..20], "export const v = 42;"));
}

test("bundler chunk_output: standalone chunk fallback to path when inline_content is null") {
    let asm = ChunkPieceAssembler.init();
    asm.addPiece("import(\"", ChunkPieceQuery.init(ChunkPieceQueryKind.Chunk, 0)) catch { expect(false); return; };
    asm.addLiteral("\");") catch { expect(false); return; };

    const chunks: [1]ChunkRefTarget = [
        ChunkRefTarget{ rel_path: "lazy\\chunk.js", inline_content: null, kind: ChunkInlineKind.JavaScript },
    ];
    const empty: [0]ChunkRefTarget = [];
    const opts = ChunkOutputAssembleOptions{
        import_prefix: "/",
        standalone_mode: true,
        emit_debug_id: false,
    };

    let out: [128]u8 = [0; 128];
    const wn = asm.assemble(chunks[0..], empty[0..], empty[0..], empty[0..], opts, out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..wn], "import(\"/lazy/chunk.js\");"));
}
