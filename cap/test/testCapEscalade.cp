import { test, expect_eq } from "cap:test";
import { AncestorWalker, joinCandidate } from "../../packages/escalade/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/escalade — Walks POSIX path all the way to root", () => {
        let w = AncestorWalker.new("/data/dev/cp/packages");
        expect_eq(w.remainingDepth(), 5);
        let buf: [128]u8 = undefined;

        let n1 = w.next(buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n1, "/data/dev/cp/packages"), true);
        let n2 = w.next(buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n2, "/data/dev/cp"), true);
        let n3 = w.next(buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n3, "/data/dev"), true);
        let n4 = w.next(buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n4, "/data"), true);
        let n5 = w.next(buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n5, "/"), true);
        expect_eq(w.isDone(), true);
        expect_eq(w.next(buf[0..128]), 0);
    });

    test("2. @cap/escalade — Normalizes Windows backslashes and stops at drive root", () => {
        let w = AncestorWalker.new("C:\\Users\\alice\\proj\\");
        let buf: [128]u8 = undefined;

        let n1 = w.next(buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n1, "C:/Users/alice/proj"), true);
        let n2 = w.next(buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n2, "C:/Users/alice"), true);
        let n3 = w.next(buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n3, "C:/Users"), true);
        let n4 = w.next(buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n4, "C:/"), true);
        expect_eq(w.isDone(), true);
    });

    test("3. @cap/escalade — Walks relative paths cleanly", () => {
        let w = AncestorWalker.new("a/b/c");
        expect_eq(w.remainingDepth(), 3);
        let buf: [64]u8 = undefined;
        expect_eq(eqBytes(buf[0..64], w.next(buf[0..64]), "a/b/c"), true);
        expect_eq(eqBytes(buf[0..64], w.next(buf[0..64]), "a/b"), true);
        expect_eq(eqBytes(buf[0..64], w.next(buf[0..64]), "a"), true);
        expect_eq(w.isDone(), true);
    });

    test("4. @cap/escalade — Root-only path yields once", () => {
        let w = AncestorWalker.new("/");
        expect_eq(w.remainingDepth(), 1);
        let buf: [16]u8 = undefined;
        expect_eq(eqBytes(buf[0..16], w.next(buf[0..16]), "/"), true);
        expect_eq(w.isDone(), true);
    });

    test("5. @cap/escalade — joinCandidate handles normal dirs and root slash", () => {
        let buf: [64]u8 = undefined;
        let n1 = joinCandidate("/data/dev/cp", "cap.json", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "/data/dev/cp/cap.json"), true);

        let n2 = joinCandidate("/", "cap.json", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "/cap.json"), true);

        let n3 = joinCandidate("C:/", "cap.json", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n3, "C:/cap.json"), true);
    });

    test("6. @cap/escalade — Empty path is immediately done", () => {
        let w = AncestorWalker.new("");
        expect_eq(w.isDone(), true);
        expect_eq(w.remainingDepth(), 0);
        let buf: [16]u8 = undefined;
        expect_eq(w.next(buf[0..16]), 0);
    });
}
