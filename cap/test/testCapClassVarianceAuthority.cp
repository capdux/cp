import { test, expect_eq } from "cap:test";
import {
    VariantRule,
    VariantSelection,
    CompoundRule,
    cvaCompose,
    cvaComposeWithCompounds,
} from "../../packages/class-variance-authority/src/index.cp";

fn strEq(a: []const u8, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn main() {
    test("1. @cap/class-variance-authority — Default variants applied when no selection", () => {
        const rules: [4]VariantRule = [
            VariantRule{ group: "intent", option: "primary", classes: "bg-blue-500 text-white" },
            VariantRule{ group: "intent", option: "secondary", classes: "bg-white text-gray-800" },
            VariantRule{ group: "size", option: "small", classes: "text-sm py-1 px-2" },
            VariantRule{ group: "size", option: "medium", classes: "text-base py-2 px-4" },
        ];
        const defaults: [2]VariantSelection = [
            VariantSelection{ group: "intent", option: "primary" },
            VariantSelection{ group: "size", option: "medium" },
        ];
        const dummy: [1]VariantSelection = [VariantSelection{ group: "", option: "" }];
        const empty_sel = dummy[0..0];

        let buf: [256]u8 = undefined;
        const n = cvaCompose("font-semibold border rounded", rules[0..], defaults[0..], empty_sel, "", buf[0..]);
        expect_eq(strEq(buf[0..n], "font-semibold border rounded bg-blue-500 text-white text-base py-2 px-4"), true);
    });

    test("2. @cap/class-variance-authority — Selected variants override defaults + extra className", () => {
        const rules: [4]VariantRule = [
            VariantRule{ group: "intent", option: "primary", classes: "bg-blue-500" },
            VariantRule{ group: "intent", option: "danger", classes: "bg-red-600" },
            VariantRule{ group: "size", option: "sm", classes: "px-2" },
            VariantRule{ group: "size", option: "lg", classes: "px-6" },
        ];
        const defaults: [2]VariantSelection = [
            VariantSelection{ group: "intent", option: "primary" },
            VariantSelection{ group: "size", option: "sm" },
        ];
        const selected: [1]VariantSelection = [
            VariantSelection{ group: "intent", option: "danger" },
        ];

        let buf: [256]u8 = undefined;
        const n = cvaCompose("btn", rules[0..], defaults[0..], selected[0..], "mt-4 shadow", buf[0..]);
        expect_eq(strEq(buf[0..n], "btn bg-red-600 px-2 mt-4 shadow"), true);
    });

    test("3. @cap/class-variance-authority — Compound variants (pair & triple conditions)", () => {
        const rules: [4]VariantRule = [
            VariantRule{ group: "intent", option: "primary", classes: "bg-blue-500" },
            VariantRule{ group: "size", option: "medium", classes: "py-2" },
            VariantRule{ group: "disabled", option: "true", classes: "opacity-50" },
            VariantRule{ group: "disabled", option: "false", classes: "cursor-pointer" },
        ];
        const compounds: [2]CompoundRule = [
            CompoundRule.pair("intent", "primary", "size", "medium", "uppercase tracking-wide"),
            CompoundRule.triple("intent", "primary", "size", "medium", "disabled", "false", "hover:bg-blue-600"),
        ];
        const defaults: [3]VariantSelection = [
            VariantSelection{ group: "intent", option: "primary" },
            VariantSelection{ group: "size", option: "medium" },
            VariantSelection{ group: "disabled", option: "false" },
        ];
        const dummy: [1]VariantSelection = [VariantSelection{ group: "", option: "" }];
        const empty_sel = dummy[0..0];

        let buf: [256]u8 = undefined;
        const n1 = cvaComposeWithCompounds("btn", rules[0..], compounds[0..], defaults[0..], empty_sel, "", buf[0..]);
        expect_eq(strEq(buf[0..n1], "btn bg-blue-500 py-2 cursor-pointer uppercase tracking-wide hover:bg-blue-600"), true);

        // When disabled=true, pair still matches, triple does NOT match
        const sel_dis: [1]VariantSelection = [VariantSelection{ group: "disabled", option: "true" }];
        const n2 = cvaComposeWithCompounds("btn", rules[0..], compounds[0..], defaults[0..], sel_dis[0..], "", buf[0..]);
        expect_eq(strEq(buf[0..n2], "btn bg-blue-500 py-2 opacity-50 uppercase tracking-wide"), true);
    });

    test("4. @cap/class-variance-authority — Whitespace normalization across base, rules & extra", () => {
        const rules: [1]VariantRule = [
            VariantRule{ group: "tone", option: "dark", classes: "  bg-black\t\ntext-white  " },
        ];
        const defaults: [1]VariantSelection = [
            VariantSelection{ group: "tone", option: "dark" },
        ];
        const dummy: [1]VariantSelection = [VariantSelection{ group: "", option: "" }];
        const empty_sel = dummy[0..0];

        let buf: [128]u8 = undefined;
        const n = cvaCompose("  flex   items-center \n ", rules[0..], defaults[0..], empty_sel, "  gap-2   p-4 ", buf[0..]);
        expect_eq(strEq(buf[0..n], "flex items-center bg-black text-white gap-2 p-4"), true);
    });

    test("5. @cap/class-variance-authority — Last selection wins & unmatched option ignored", () => {
        const rules: [2]VariantRule = [
            VariantRule{ group: "size", option: "sm", classes: "h-8" },
            VariantRule{ group: "size", option: "lg", classes: "h-12" },
        ];
        const dummy: [1]VariantSelection = [VariantSelection{ group: "", option: "" }];
        const empty_def = dummy[0..0];
        const sel: [2]VariantSelection = [
            VariantSelection{ group: "size", option: "sm" },
            VariantSelection{ group: "size", option: "lg" },
        ];

        let buf: [128]u8 = undefined;
        const n1 = cvaCompose("input", rules[0..], empty_def, sel[0..], "", buf[0..]);
        expect_eq(strEq(buf[0..n1], "input h-12"), true);

        const unknown: [1]VariantSelection = [VariantSelection{ group: "size", option: "xl" }];
        const n2 = cvaCompose("input", rules[0..], empty_def, unknown[0..], "", buf[0..]);
        expect_eq(strEq(buf[0..n2], "input"), true);
    });

    test("6. @cap/class-variance-authority — Empty base / empty rules / tight buffer truncation safety", () => {
        const dummy_r: [1]VariantRule = [VariantRule{ group: "", option: "", classes: "" }];
        const empty_r = dummy_r[0..0];
        const dummy_s: [1]VariantSelection = [VariantSelection{ group: "", option: "" }];
        const empty_s = dummy_s[0..0];

        let buf: [32]u8 = undefined;
        const n1 = cvaCompose("", empty_r, empty_s, empty_s, "only-extra", buf[0..]);
        expect_eq(strEq(buf[0..n1], "only-extra"), true);

        const n2 = cvaCompose("   ", empty_r, empty_s, empty_s, "  ", buf[0..]);
        expect_eq(n2, 0);

        let tiny: [5]u8 = undefined;
        const n3 = cvaCompose("hello world", empty_r, empty_s, empty_s, "", tiny[0..]);
        expect_eq(n3, 5);
        expect_eq(strEq(tiny[0..5], "hello"), true);
    });
}
