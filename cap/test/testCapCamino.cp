import { test, expect_eq } from "cap:test";
import { Utf8PathBuf } from "../../packages/camino/src/index.cp";

fn eqBytes(buf: []u8, n: usize, expected: string): bool {
    if (n != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/camino — fromStr & fromBytes strict UTF-8 validation", () => {
        const p = Utf8PathBuf.fromStr("/usr/local/bin");
        expect_eq(p.isAbsolute(), true);
        expect_eq(p.isRelative(), false);
        expect_eq(p.len(), 14);
        expect_eq(p.isEmpty(), false);
        expect_eq(p.equals("/usr/local/bin"), true);
        expect_eq(p.equals("/usr/local"), false);

        // Valid UTF-8 bytes: "src/中文.cp"
        let valid_bytes: [13]u8 = [
            0x73, 0x72, 0x63, 0x2F,
            0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87,
            0x2E, 0x63, 0x70
        ];
        const ok_opt = Utf8PathBuf.fromBytes(valid_bytes[0..13]);
        expect_eq(ok_opt != null, true);
        const ok_p = ok_opt ?? Utf8PathBuf.fromStr("");
        expect_eq(ok_p.isRelative(), true);
        expect_eq(ok_p.len(), 13);

        // Invalid UTF-8 bytes (lone continuation / 0xFF)
        let bad_bytes: [4]u8 = [0x2F, 0x61, 0xFF, 0x62];
        expect_eq(Utf8PathBuf.fromBytes(bad_bytes[0..4]) == null, true);
    });

    test("2. @cap/camino — push (relative join & absolute replace) and pop", () => {
        let p = Utf8PathBuf.fromStr("/tmp");
        p.push("logs");
        expect_eq(p.equals("/tmp/logs"), true);
        p.push("app.log");
        expect_eq(p.equals("/tmp/logs/app.log"), true);

        // Pushing an absolute path replaces the entire path
        p.push("/etc/hosts");
        expect_eq(p.equals("/etc/hosts"), true);

        expect_eq(p.pop(), true);
        expect_eq(p.equals("/etc"), true);
        expect_eq(p.pop(), true);
        expect_eq(p.equals("/"), true);
        // Root cannot be popped further
        expect_eq(p.pop(), false);
        expect_eq(p.equals("/"), true);
    });

    test("3. @cap/camino — parent on absolute, relative, single-segment and root paths", () => {
        const p1 = Utf8PathBuf.fromStr("/a/b/c.txt");
        const par1 = p1.parent() ?? Utf8PathBuf.fromStr("ERR");
        expect_eq(par1.equals("/a/b"), true);

        const p2 = Utf8PathBuf.fromStr("file.cp");
        const par2 = p2.parent() ?? Utf8PathBuf.fromStr("ERR");
        expect_eq(par2.isEmpty(), true);

        const root = Utf8PathBuf.fromStr("/");
        expect_eq(root.parent() == null, true);

        const empty = Utf8PathBuf.fromStr("");
        expect_eq(empty.parent() == null, true);
    });

    test("4. @cap/camino — fileNameInto, fileStemInto, extensionInto", () => {
        const p = Utf8PathBuf.fromStr("/var/archive.tar.gz");
        let out: [64]u8 = [0; 64];

        const fn_len = p.fileNameInto(out[0..64]) ?? 999;
        expect_eq(eqBytes(out[0..64], fn_len, "archive.tar.gz"), true);

        const stem_len = p.fileStemInto(out[0..64]) ?? 999;
        expect_eq(eqBytes(out[0..64], stem_len, "archive.tar"), true);

        const ext_len = p.extensionInto(out[0..64]) ?? 999;
        expect_eq(eqBytes(out[0..64], ext_len, "gz"), true);

        // Dotfile has no extension and stem equals full name
        const dot = Utf8PathBuf.fromStr("/home/user/.bashrc");
        const dot_stem = dot.fileStemInto(out[0..64]) ?? 999;
        expect_eq(eqBytes(out[0..64], dot_stem, ".bashrc"), true);
        expect_eq(dot.extensionInto(out[0..64]) == null, true);
    });

    test("5. @cap/camino — setFileName & setExtension", () => {
        let p = Utf8PathBuf.fromStr("/src/main.rs");
        expect_eq(p.setExtension("cp"), true);
        expect_eq(p.equals("/src/main.cp"), true);

        // Empty extension strips extension
        expect_eq(p.setExtension(""), true);
        expect_eq(p.equals("/src/main"), true);

        p.setFileName("index.cp");
        expect_eq(p.equals("/src/index.cp"), true);

        // Root has no fileName -> setExtension returns false
        let root = Utf8PathBuf.fromStr("/");
        expect_eq(root.setExtension("txt"), false);
    });

    test("6. @cap/camino — hasPrefixPath, stripPrefix & componentCount", () => {
        const p = Utf8PathBuf.fromStr("/usr/local/bin/cap");
        expect_eq(p.componentCount(), 5);
        expect_eq(p.hasPrefixPath("/usr/local"), true);
        expect_eq(p.hasPrefixPath("/usr/loc"), false);
        expect_eq(p.hasPrefixPath("/"), true);

        const stripped = p.stripPrefix("/usr/local") ?? Utf8PathBuf.fromStr("ERR");
        expect_eq(stripped.equals("bin/cap"), true);
        expect_eq(stripped.componentCount(), 2);

        expect_eq(p.stripPrefix("/etc") == null, true);

        const exact = p.stripPrefix("/usr/local/bin/cap") ?? Utf8PathBuf.fromStr("ERR");
        expect_eq(exact.isEmpty(), true);
        expect_eq(exact.componentCount(), 0);
    });
}
