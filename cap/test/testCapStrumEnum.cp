import { test, expect_eq } from "cap:test";
import { EnumRegistry, EnumVariant } from "../../packages/strum/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/strum-enum — register, variantCount & exact fromStr", () => {
        let reg = EnumRegistry.new();
        reg.register(0, "Red", "r", "Primary red");
        reg.register(1, "Green", "g", "Primary green");
        reg.register(2, "Blue", "b", "Primary blue");
        expect_eq(reg.variantCount(), 3);
        expect_eq(reg.fromStr("Red") ?? -1, 0);
        expect_eq(reg.fromStr("g") ?? -1, 1);
        expect_eq(reg.fromStr("red") == null, true);
    });

    test("2. @cap/strum-enum — Case-insensitive fromStrIgnoreCase via @cap/unicase", () => {
        let reg = EnumRegistry.new();
        reg.register(10, "Debug", "dbg", "Debug level");
        reg.register(20, "Error", "err", "Error level");
        expect_eq(reg.fromStrIgnoreCase("DEBUG") ?? -1, 10);
        expect_eq(reg.fromStrIgnoreCase("DBG") ?? -1, 10);
        expect_eq(reg.fromStrIgnoreCase("error") ?? -1, 20);
        expect_eq(reg.fromStrIgnoreCase("unknown") == null, true);
    });

    test("3. @cap/strum-enum — asStr & getMessage discriminant lookup", () => {
        let reg = EnumRegistry.new();
        reg.register(200, "Ok", "", "Request succeeded");
        reg.register(404, "NotFound", "", "Resource missing");
        expect_eq(reg.asStr(200) ?? "", "Ok");
        expect_eq(reg.getMessage(404) ?? "", "Resource missing");
        expect_eq(reg.asStr(500) == null, true);
    });

    test("4. @cap/strum-enum — variantAt indexed iteration", () => {
        let reg = EnumRegistry.new();
        reg.register(1, "Alpha", "", "");
        reg.register(2, "Beta", "", "");
        let z = EnumVariant { id: -1, name: "", alias: "", message: "" };
        expect_eq((reg.variantAt(0) ?? z).name, "Alpha");
        expect_eq((reg.variantAt(1) ?? z).id, 2);
        expect_eq(reg.variantAt(2) == null, true);
    });

    test("5. @cap/strum-enum — formatVariantList formats comma-separated names", () => {
        let reg = EnumRegistry.new();
        reg.register(0, "json", "", "");
        reg.register(1, "yaml", "", "");
        reg.register(2, "toml", "", "");
        let out: [64]u8 = undefined;
        let n = reg.formatVariantList(out[0..64]);
        expect_eq(eqBuf(out[0..n], n, "json, yaml, toml"), true);
    });

    test("6. @cap/strum-enum — Empty registry returns null cleanly", () => {
        let reg = EnumRegistry.new();
        expect_eq(reg.variantCount(), 0);
        expect_eq(reg.fromStr("anything") == null, true);
        let out: [16]u8 = undefined;
        expect_eq(reg.formatVariantList(out[0..16]), 0);
    });
}
