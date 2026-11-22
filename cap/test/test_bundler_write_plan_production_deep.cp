// Production Deep Test for cap:bundler (writePlan.cp)
// Covers: relative output path traversal guards, POSIX parent dir extraction,
// JS/CSS sourceMappingURL formatting, DiskWritePlanner directory deduplication,
// SourceMap/Bytecode companion emission, executable mode (0o755/0o644),
// standalone HTML placeholder alignment, and duplicate output path collision detection.

import { test, assert, assertEq } from "cap:test";
import {
    validateOutputRelPath,
    extractParentDirPosix,
    WriteChunkContentKind,
    WriteSourceMapMode,
    WriteFileRole,
    formatSourceMapUrlComment,
    DiskWritePlanner,
} from "cap:bundler";

fn strEql(a: []const u8, b: []const u8): bool {
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

test("writePlan: validateOutputRelPath traversal and control-char guards", () => {
    assert(validateOutputRelPath("index.js") catch false);
    assert(validateOutputRelPath("assets/js/chunk-1.js") catch false);
    assert(validateOutputRelPath("a/..hidden/file.js") catch false);

    assert(!(validateOutputRelPath("") catch false));
    assert(!(validateOutputRelPath("/etc/passwd") catch false));
    assert(!(validateOutputRelPath("\\windows\\sys") catch false));
    assert(!(validateOutputRelPath("C:foo/bar.js") catch false));
    assert(!(validateOutputRelPath("../escape.js") catch false));
    assert(!(validateOutputRelPath("assets/../../escape.js") catch false));
    assert(!(validateOutputRelPath("bad\nfile.js") catch false));
});

test("writePlan: extractParentDirPosix and JS/CSS sourceMappingURL formatting", () => {
    assert(extractParentDirPosix("index.js") == null);
    if (p := extractParentDirPosix("assets/js/app.js")) {
        assert(strEql(p, "assets/js"));
    } else {
        assert(false);
    }

    let buf: [128]u8 = [0; 128];
    const n1 = formatSourceMapUrlComment(WriteChunkContentKind.Js, "app.js.map", "/static/", buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n1], "//# sourceMappingURL=/static/app.js.map\n"));

    const n2 = formatSourceMapUrlComment(WriteChunkContentKind.Css, "style.css.map", "", buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n2], "/*# sourceMappingURL=style.css.map */\n"));

    let tiny: [8]u8 = [0; 8];
    const fail_n = formatSourceMapUrlComment(WriteChunkContentKind.Js, "app.js.map", "", tiny[0..]) catch 999;
    assertEq(fail_n, 999);
});

test("writePlan: DiskWritePlanner directory deduplication, companions, and executable mode", () => {
    let wp = DiskWritePlanner.init(false);

    * = wp.planChunk(
        0,
        WriteChunkContentKind.Js,
        "bin/cli.js",
        2048,
        true,
        WriteSourceMapMode.Linked,
        "bin/cli.js.map",
        true,
        "bin/cli.jsc",
    ) catch {
        assert(false);
        return;
    };
    * = wp.planChunk(
        1,
        WriteChunkContentKind.Css,
        "assets/app.css",
        512,
        false,
        WriteSourceMapMode.External,
        "assets/app.css.map",
        false,
        "",
    ) catch {
        assert(false);
        return;
    };

    const st = wp.stats();
    assertEq(st.dirs_to_create, 2); // "bin" (deduped across 3 files) + "assets"
    assertEq(st.files_to_write, 5);
    assertEq(st.sourcemaps_count, 2);
    assertEq(st.bytecode_count, 1);
    assertEq(st.executable_count, 1);
    assertEq(st.placeholders_count, 0);
    assertEq(st.total_bytes, 2560);

    if (d0 := wp.dirAt(0)) {
        assert(strEql(d0, "bin"));
    } else {
        assert(false);
    }
    if (d1 := wp.dirAt(1)) {
        assert(strEql(d1, "assets"));
    } else {
        assert(false);
    }

    if (f0 := wp.fileAt(0)) {
        assertEq(f0.file_mode, 0o755);
        assert(f0.role == WriteFileRole.PrimaryChunk);
    } else {
        assert(false);
    }
});

test("writePlan: standalone HTML mode emits index-aligned placeholders for non-HTML chunks", () => {
    let wp = DiskWritePlanner.init(true);

    const wrote_js = wp.planChunk(
        0,
        WriteChunkContentKind.Js,
        "chunk-0.js",
        1024,
        false,
        WriteSourceMapMode.None,
        "",
        false,
        "",
    ) catch {
        assert(false);
        return;
    };
    assert(!wrote_js);

    const wrote_html = wp.planChunk(
        1,
        WriteChunkContentKind.Html,
        "index.html",
        4096,
        false,
        WriteSourceMapMode.None,
        "",
        false,
        "",
    ) catch {
        assert(false);
        return;
    };
    assert(wrote_html);

    const st = wp.stats();
    assertEq(st.placeholders_count, 1);
    assertEq(st.files_to_write, 1);
    assertEq(st.dirs_to_create, 0);

    if (p0 := wp.fileAt(0)) {
        assert(p0.role == WriteFileRole.Placeholder);
        assert(p0.standalone_skipped);
    } else {
        assert(false);
    }
});

test("writePlan: duplicate output path collision and unsafe path rejection", () => {
    let wp = DiskWritePlanner.init(false);

    * = wp.planChunk(
        0,
        WriteChunkContentKind.Js,
        "dist/app.js",
        100,
        false,
        WriteSourceMapMode.None,
        "",
        false,
        "",
    ) catch {
        assert(false);
        return;
    };

    // Duplicate path must fail
    const dup = wp.planChunk(
        1,
        WriteChunkContentKind.Js,
        "dist/app.js",
        200,
        false,
        WriteSourceMapMode.None,
        "",
        false,
        "",
    ) catch false;
    assert(!dup);

    // Traversal path must fail
    const bad = wp.planChunk(
        2,
        WriteChunkContentKind.Js,
        "../outside.js",
        50,
        false,
        WriteSourceMapMode.None,
        "",
        false,
        "",
    ) catch false;
    assert(!bad);
});
