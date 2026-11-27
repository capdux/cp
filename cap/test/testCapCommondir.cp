import { test, expect_eq } from "cap:test";
import {
    commonDirTwo,
    commonDir,
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
}
