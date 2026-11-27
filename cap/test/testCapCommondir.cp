import { test, expect_eq } from "cap:test";
import {
    normalizePosixPath,
    resolvePosixPath,
    commonDirTwo,
    commonDir,
    commonDirTwoNormalized,
    commonDirNormalized,
    commonDirWithBase,
} from "../../packages/commondir/src/index.cp";

fn main() {
    test("1. @cap/commondir — finds deepest common parent of sibling files", () => {
        const paths: [3]string = ["/a/b/c/foo.cp", "/a/b/c/bar.cp", "/a/b/d/baz.cp"];
        expect_eq(commonDir(paths[0..3]), "/a/b");
    });

    test("2. @cap/commondir — avoids partial directory name prefix collision", () => {
        expect_eq(commonDirTwo("/srv/app-v1/a.cp", "/srv/app-v2/b.cp"), "/srv");
        expect_eq(commonDirTwo("/a/foo/x", "/a/foobar/y"), "/a");
    });

    test("3. @cap/commondir — returns root '/' when only root is shared", () => {
        expect_eq(commonDirTwo("/etc/hosts", "/var/log/syslog"), "/");
    });

    test("4. @cap/commondir — handles parent-child containment", () => {
        expect_eq(commonDirTwo("/data/dev/cp", "/data/dev/cp/src/main.cp"), "/data/dev/cp");
        expect_eq(commonDirTwo("/data/dev/cp/src/main.cp", "/data/dev/cp"), "/data/dev/cp");
    });

    test("5. @cap/commondir — handles relative paths and disjoint roots", () => {
        expect_eq(commonDirTwo("src/net/tcp.cp", "src/net/udp.cp"), "src/net");
        expect_eq(commonDirTwo("foo/a", "bar/b"), "");
    });

    test("6. @cap/commondir — handles empty and single-element lists", () => {
        const one: [1]string = ["/a/b/"];
        expect_eq(commonDir(one[0..0]), "");
        expect_eq(commonDir(one[0..1]), "/a/b");
    });

    test("7. @cap/commondir — normalizePosixPath collapses //, ., .. & trailing /", () => {
        let out: [128]u8 = undefined;
        let n1 = normalizePosixPath("/a/b/../c//./d/", out[0..128]);
        expect_eq(out[0..n1] as string, "/a/c/d");

        let n2 = normalizePosixPath("/../../..", out[0..128]);
        expect_eq(out[0..n2] as string, "/");

        let n3 = normalizePosixPath("a/b/../c", out[0..128]);
        expect_eq(out[0..n3] as string, "a/c");

        let n4 = normalizePosixPath("./a/..", out[0..128]);
        expect_eq(out[0..n4] as string, ".");
    });

    test("8. @cap/commondir — commonDirNormalized & commonDirTwoNormalized", () => {
        let out: [128]u8 = undefined;
        const paths: [3]string = ["/a/b/c/../d/x.cp", "/a/b//d/./y.cp", "/a/b/e/../d/z.cp"];
        let n1 = commonDirNormalized(paths[0..3], out[0..128]);
        expect_eq(out[0..n1] as string, "/a/b/d");

        let n2 = commonDirTwoNormalized("/x/y/../z/1", "/x/z/2", out[0..128]);
        expect_eq(out[0..n2] as string, "/x/z");
    });

    test("9. @cap/commondir — commonDirWithBase & resolvePosixPath (Node commondir(base, files))", () => {
        let out: [128]u8 = undefined;
        const rels: [3]string = ["../c/d", "../../b/c/d", "./e/f"];
        let n1 = commonDirWithBase("/a/b/x", rels[0..3], out[0..128]);
        expect_eq(out[0..n1] as string, "/a/b");

        let n2 = resolvePosixPath("/usr/local/bin", "../lib/cp", out[0..128]);
        expect_eq(out[0..n2] as string, "/usr/local/lib/cp");

        // Absolute file overrides base
        const mixed: [2]string = ["sub/a.cp", "/other/b.cp"];
        let n3 = commonDirWithBase("/data/dev", mixed[0..2], out[0..128]);
        expect_eq(out[0..n3] as string, "/");
    });
}
