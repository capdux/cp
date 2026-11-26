import { test, expect_eq } from "cap:test";
import {
    PatchOp,
    StatePatch,
    invertPatch,
    formatJsonPointer,
    parseJsonPointer,
    unescapePointerToken,
    patchOpName,
} from "../../packages/@cap/immer-patches/src/index.cp";

fn main() {
    test("1. @cap/immer-patches — patchOpName returns RFC 6902 op strings", () => {
        expect_eq(patchOpName(.Add), "add");
        expect_eq(patchOpName(.Replace), "replace");
        expect_eq(patchOpName(.Remove), "remove");
    });

    test("2. @cap/immer-patches — invertPatch swaps Add <-> Remove for undo/redo", () => {
        const added = StatePatch{
            op: .Add,
            path: "/users/0",
            value: "alice",
            prev_value: "",
        };
        const undo = invertPatch(added);
        expect_eq(patchOpName(undo.op), "remove");
        expect_eq(undo.path, "/users/0");
        expect_eq(undo.value, "");
        expect_eq(undo.prev_value, "alice");

        const redo = invertPatch(undo);
        expect_eq(patchOpName(redo.op), "add");
        expect_eq(redo.path, "/users/0");
        expect_eq(redo.value, "alice");
        expect_eq(redo.prev_value, "");
    });

    test("3. @cap/immer-patches — invertPatch swaps value and prev_value on Replace", () => {
        const p = StatePatch{
            op: .Replace,
            path: "/config/port",
            value: "9090",
            prev_value: "8080",
        };
        const inv = invertPatch(p);
        expect_eq(patchOpName(inv.op), "replace");
        expect_eq(inv.path, "/config/port");
        expect_eq(inv.value, "8080");
        expect_eq(inv.prev_value, "9090");
    });

    test("4. @cap/immer-patches — formatJsonPointer escapes ~ and / per RFC 6901", () => {
        const segs: [3]string = ["a/b", "c~d", "m~n/p"];
        let buf: [128]u8 = [0; 128];
        const n = formatJsonPointer(segs[0..3], buf[0..128]);
        expect_eq(buf[0..n] as string, "/a~1b/c~0d/m~0n~1p");

        const n0 = formatJsonPointer(segs[0..0], buf[0..128]);
        expect_eq(n0, 0 as usize);
    });

    test("5. @cap/immer-patches — parseJsonPointer splits segments including empty root token", () => {
        let out: [8]string = [""; 8];
        const n = parseJsonPointer("/users/0/profile/name", out[0..8]);
        expect_eq(n, 4 as usize);
        expect_eq(out[0], "users");
        expect_eq(out[1], "0");
        expect_eq(out[2], "profile");
        expect_eq(out[3], "name");

        expect_eq(parseJsonPointer("", out[0..8]), 0 as usize);
        expect_eq(parseJsonPointer("no-leading-slash", out[0..8]), 0 as usize);
    });

    test("6. @cap/immer-patches — format + parse + unescapePointerToken round-trip", () => {
        const segs: [2]string = ["foo/bar", "tilde~key"];
        let ptr_buf: [128]u8 = [0; 128];
        const ptr_len = formatJsonPointer(segs[0..2], ptr_buf[0..128]);
        const ptr = ptr_buf[0..ptr_len] as string;

        let tokens: [4]string = [""; 4];
        const count = parseJsonPointer(ptr, tokens[0..4]);
        expect_eq(count, 2 as usize);

        let dec0: [64]u8 = [0; 64];
        let dec1: [64]u8 = [0; 64];
        const d0 = unescapePointerToken(tokens[0], dec0[0..64]);
        const d1 = unescapePointerToken(tokens[1], dec1[0..64]);
        expect_eq(dec0[0..d0] as string, "foo/bar");
        expect_eq(dec1[0..d1] as string, "tilde~key");
    });
}
