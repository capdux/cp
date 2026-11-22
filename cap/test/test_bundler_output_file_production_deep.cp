import { test, expect, expect_eq } from "cap:test";
import {
    OUTPUT_FILE_MAX_COLLECTION,
    OutputFileKind,
    OutputValueKind,
    OutputLoaderKind,
    OutputBakeExtra,
    BuildOutputFile,
    BuildOutputCollection,
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

test("bundler output_file: fromBuffer, loader/MIME inference, content hash & summary", fn() {
    let f = BuildOutputFile.fromBuffer(
        "chunks/app.js",
        OutputFileKind.EntryPoint,
        "export const x = 42;",
    ) catch {
        expect(false);
        return;
    };
    expect(f.kind == OutputFileKind.EntryPoint);
    expect(strEq(f.kind.asStr(), "entry-point"));
    expect(f.value_kind.isInMemory());
    expect(!f.value_kind.isPersisted());
    expect(f.loader == OutputLoaderKind.Js);
    expect(strEq(f.mimeType(), "text/javascript;charset=utf-8"));
    expect(strEq(f.asSlice(), "export const x = 42;"));
    expect_eq(f.size_bytes, 20);
    expect(f.content_hash != 0);

    let line: [128]u8 = [0; 128];
    const ln = f.formatSummaryLine(line[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(line[0..ln], "chunks/app.js [entry-point, 20 B]"));

    f.markSaved();
    expect(f.value_kind.isPersisted());
    expect(!f.value_kind.isInMemory());
    expect(strEq(f.asSlice(), ""));
});

test("bundler output_file: move, copy, noop modes & BakeExtra metadata", fn() {
    let mv = BuildOutputFile.fromMove(
        "/tmp/build-123.css",
        "styles/main.css",
        OutputFileKind.Chunk,
        1024,
    ) catch {
        expect(false);
        return;
    };
    expect(mv.value_kind == OutputValueKind.Move);
    expect(mv.loader == OutputLoaderKind.Css);
    expect(strEq(mv.mimeType(), "text/css;charset=utf-8"));
    expect(strEq(mv.asSlice(), ""));
    mv.bake = OutputBakeExtra{ is_route: true, fully_static: true, bake_is_runtime: false };
    expect(mv.bake.is_route && mv.bake.fully_static);

    const cp = BuildOutputFile.fromCopy(
        "public/app.wasm",
        "assets/app.wasm",
        OutputFileKind.Asset,
        4096,
    ) catch {
        expect(false);
        return;
    };
    expect(cp.value_kind == OutputValueKind.Copy);
    expect(cp.loader == OutputLoaderKind.Wasm);
    expect(strEq(cp.mimeType(), "application/wasm"));

    const np = BuildOutputFile.noop("ignored.txt") catch {
        expect(false);
        return;
    };
    expect(np.value_kind == OutputValueKind.Noop);
    expect_eq(np.size_bytes, 0);
});

test("bundler output_file: resolveFullPath normalization & traversal guards", fn() {
    const f = BuildOutputFile.fromBuffer("js/index.js", OutputFileKind.EntryPoint, "1") catch {
        expect(false);
        return;
    };
    let out: [128]u8 = [0; 128];
    const n1 = f.resolveFullPath("./dist///", out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n1], "./dist/js/index.js"));

    const n2 = f.resolveFullPath("", out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n2], "js/index.js"));

    // 绝对 dest_path 禁止在 resolveFullPath 中逃逸 outdir
    const abs_f = BuildOutputFile.fromBuffer("/etc/passwd", OutputFileKind.Asset, "x") catch {
        expect(false);
        return;
    };
    let ok_abs = true;
    * = abs_f.resolveFullPath("./dist", out[0..]) catch {
        ok_abs = false;
    };
    expect(!ok_abs);
});

test("bundler output_file: BuildOutputCollection aggregation & lookup", fn() {
    let col = BuildOutputCollection.init();
    expect_eq(col.count(), 0);
    expect_eq(col.totalBytes(), 0);

    * = col.push(BuildOutputFile.fromBuffer("index.js", OutputFileKind.EntryPoint, "console.log(1)") catch {
        expect(false);
        return;
    }) catch {
        expect(false);
        return;
    };
    * = col.push(BuildOutputFile.fromBuffer("index.js.map", OutputFileKind.Sourcemap, "{}") catch {
        expect(false);
        return;
    }) catch {
        expect(false);
        return;
    };
    * = col.push(BuildOutputFile.fromBuffer("index.html", OutputFileKind.EntryPoint, "<html></html>") catch {
        expect(false);
        return;
    }) catch {
        expect(false);
        return;
    };

    expect_eq(col.count(), 3);
    expect_eq(col.totalBytes(), 14 + 2 + 13);
    expect_eq(col.countByKind(OutputFileKind.EntryPoint), 2);
    expect_eq(col.countByKind(OutputFileKind.Sourcemap), 1);
    expect_eq(col.countByKind(OutputFileKind.Bytecode), 0);

    if (found := col.findByPath("index.js.map")) {
        expect(found.loader == OutputLoaderKind.Json);
        expect(strEq(found.mimeType(), "application/json;charset=utf-8"));
    } else {
        expect(false);
    }
    expect(col.findByPath("missing.js") == null);
});

test("bundler output_file: malformed empty path & directory traversal rejection", fn() {
    let ok1 = true;
    * = BuildOutputFile.fromBuffer("", OutputFileKind.Chunk, "x") catch {
        ok1 = false;
    };
    expect(!ok1);

    let ok2 = true;
    * = BuildOutputFile.fromBuffer("../escape.js", OutputFileKind.Chunk, "x") catch {
        ok2 = false;
    };
    expect(!ok2);

    let ok3 = true;
    * = BuildOutputFile.fromBuffer("a/../../b.js", OutputFileKind.Chunk, "x") catch {
        ok3 = false;
    };
    expect(!ok3);

    let ok4 = true;
    * = BuildOutputFile.fromMove("", "a.js", OutputFileKind.Chunk, 1) catch {
        ok4 = false;
    };
    expect(!ok4);
});

test("bundler output_file: collection capacity & buffer-too-small guards", fn() {
    expect_eq(OUTPUT_FILE_MAX_COLLECTION, 64);

    let col = BuildOutputCollection.init();
    const sample = BuildOutputFile.fromBuffer("a.js", OutputFileKind.Chunk, "1") catch {
        expect(false);
        return;
    };
    let i: usize = 0;
    while (i < OUTPUT_FILE_MAX_COLLECTION) {
        * = col.push(sample) catch {
            expect(false);
            return;
        };
        i += 1;
    }
    expect_eq(col.count(), OUTPUT_FILE_MAX_COLLECTION);

    let ok_cap = true;
    * = col.push(sample) catch {
        ok_cap = false;
    };
    expect(!ok_cap);

    let tiny: [4]u8 = [0; 4];
    let ok_buf = true;
    * = sample.formatSummaryLine(tiny[0..]) catch {
        ok_buf = false;
    };
    expect(!ok_buf);
});
