import { test, expect_eq } from "cap:test";
import {
    packageDirectoryInList, capPackageDirectoryInList, packageDirectoryBounded
} from "../../packages/pkg-dir/src/index.cp";

fn main() {
    test("1. @cap/pkg-dir — finds nearest ancestor package.json directory", () => {
        let files: [2]string = ["/workspace/package.json", "/workspace/apps/web/package.json"];
        let out: [128]u8 = [0; 128];
        let n = packageDirectoryInList("/workspace/apps/web/src/components", files[0..2], out[0..128]) ?? 0;
        expect_eq(out[0..n] as string, "/workspace/apps/web");
    });

    test("2. @cap/pkg-dir — finds package.json in current directory", () => {
        let files: [1]string = ["/repo/package.json"];
        let out: [128]u8 = [0; 128];
        let n = packageDirectoryInList("/repo", files[0..1], out[0..128]) ?? 0;
        expect_eq(out[0..n] as string, "/repo");
    });

    test("3. @cap/pkg-dir — returns null when no package.json exists", () => {
        let files: [1]string = ["/other/package.json"];
        let out: [128]u8 = [0; 128];
        expect_eq(packageDirectoryInList("/repo/src", files[0..1], out[0..128]) == null, true);
    });

    test("4. @cap/pkg-dir — capPackageDirectoryInList prefers cap.json or package.json", () => {
        let files: [2]string = ["/mono/package.json", "/mono/libs/math/cap.json"];
        let out: [128]u8 = [0; 128];
        let n = capPackageDirectoryInList("/mono/libs/math/src", files[0..2], out[0..128]) ?? 0;
        expect_eq(out[0..n] as string, "/mono/libs/math");
    });

    test("5. @cap/pkg-dir — packageDirectoryBounded respects stop_at boundary", () => {
        let files: [1]string = ["/root/package.json"];
        let out: [128]u8 = [0; 128];
        expect_eq(packageDirectoryBounded("/root/sub/deep", "/root/sub", files[0..1], out[0..128]) == null, true);
        let n = packageDirectoryBounded("/root/sub/deep", "/root", files[0..1], out[0..128]) ?? 0;
        expect_eq(out[0..n] as string, "/root");
    });

    test("6. @cap/pkg-dir — handles root-level /package.json cleanly", () => {
        let files: [1]string = ["/package.json"];
        let out: [128]u8 = [0; 128];
        let n = packageDirectoryInList("/a/b", files[0..1], out[0..128]) ?? 0;
        expect_eq(out[0..n] as string, "/");
    });
}
