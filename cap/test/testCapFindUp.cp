import { test, expect_eq } from "cap:test";
import {
    FindUpOptions, FindUpWalker, joinDirAndName,
    findUpInList, findUpMultipleInList
} from "../../packages/@cap/find-up/src/index.cp";

fn main() {
    test("1. @cap/find-up — FindUpWalker traverses from cwd up to root", () => {
        let w = FindUpWalker.new(FindUpOptions.defaults("/a/b/c"));
        expect_eq(w.nextDir() ?? "", "/a/b/c");
        expect_eq(w.nextDir() ?? "", "/a/b");
        expect_eq(w.nextDir() ?? "", "/a");
        expect_eq(w.nextDir() ?? "", "/");
        expect_eq(w.nextDir() == null, true);
    });

    test("2. @cap/find-up — stop_at halts upward traversal inclusively", () => {
        let opts = FindUpOptions.defaults("/home/dev/repo/pkg/src");
        opts.stop_at = "/home/dev/repo";
        let w = FindUpWalker.new(opts);
        expect_eq(w.nextDir() ?? "", "/home/dev/repo/pkg/src");
        expect_eq(w.nextDir() ?? "", "/home/dev/repo/pkg");
        expect_eq(w.nextDir() ?? "", "/home/dev/repo");
        expect_eq(w.nextDir() == null, true);
    });

    test("3. @cap/find-up — joinDirAndName handles root and nested paths", () => {
        let buf: [128]u8 = [0; 128];
        let n1 = joinDirAndName("/app/src/", "cap.json", buf[0..128]);
        expect_eq(buf[0..n1] as string, "/app/src/cap.json");
        let n2 = joinDirAndName("/", "cap.json", buf[0..128]);
        expect_eq(buf[0..n2] as string, "/cap.json");
    });

    test("4. @cap/find-up — findUpInList returns closest ancestor match", () => {
        let existing: [2]string = ["/repo/cap.json", "/cap.json"];
        let out: [128]u8 = [0; 128];
        let n = findUpInList(FindUpOptions.defaults("/repo/packages/a"), "cap.json", existing[0..2], out[0..128]) ?? 0;
        expect_eq(out[0..n] as string, "/repo/cap.json");
    });

    test("5. @cap/find-up — findUpMultipleInList respects per-directory name priority", () => {
        let names: [2]string = ["cap.json", "package.json"];
        let existing: [2]string = ["/repo/package.json", "/repo/sub/package.json"];
        let out: [128]u8 = [0; 128];
        let n = findUpMultipleInList(FindUpOptions.defaults("/repo/sub/src"), names[0..2], existing[0..2], out[0..128]) ?? 0;
        expect_eq(out[0..n] as string, "/repo/sub/package.json");
    });

    test("6. @cap/find-up — returns null when not found within stop_at", () => {
        let opts = FindUpOptions.defaults("/a/b/c");
        opts.stop_at = "/a/b";
        let existing: [1]string = ["/a/target.txt"];
        let out: [128]u8 = [0; 128];
        expect_eq(findUpInList(opts, "target.txt", existing[0..1], out[0..128]) == null, true);
    });
}
