// cap/test/testCapMergeDescriptors.cp
// Comprehensive unit tests for @cap/merge-descriptors (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    MAX_DESCRIPTORS,
    PropDescriptor,
    DescriptorTable,
    mergeDescriptors,
    countEnumerable
} from "../../packages/@cap/merge-descriptors/src/index.cp";

fn main() {
    test("1. DescriptorTable — define, get, has & len basics", () => {
        let tbl = DescriptorTable.new();
        expect_eq(tbl.len(), 0 as usize);
        expect_eq(tbl.has("host"), false);
        expect_eq(tbl.get("host") == null, true);

        expect_eq(tbl.define(PropDescriptor.data("host", "localhost")), true);
        expect_eq(tbl.define(PropDescriptor.data("port", "8080")), true);
        expect_eq(tbl.len(), 2 as usize);
        expect_eq(tbl.has("host"), true);

        const d = tbl.get("port");
        expect_eq(d != null, true);
        if (d != null) {
            expect_eq(d.value, "8080");
            expect_eq(d.writable, true);
            expect_eq(d.enumerable, true);
            expect_eq(d.configurable, true);
        }

        // Empty key rejected
        expect_eq(tbl.define(PropDescriptor.data("", "bad")), false);
    });

    test("2. DescriptorTable — configurable guard on redefine & deleteKey", () => {
        let tbl = DescriptorTable.new();
        expect_eq(tbl.define(PropDescriptor.data("mutable", "v1")), true);
        expect_eq(tbl.define(PropDescriptor.readonly("locked", "const")), true);

        // Configurable key can be redefined & deleted
        expect_eq(tbl.define(PropDescriptor.data("mutable", "v2")), true);
        const m = tbl.get("mutable");
        if (m != null) { expect_eq(m.value, "v2"); }
        expect_eq(tbl.deleteKey("mutable"), true);
        expect_eq(tbl.has("mutable"), false);

        // Non-configurable key cannot be redefined or deleted
        expect_eq(tbl.define(PropDescriptor.data("locked", "hacked")), false);
        expect_eq(tbl.deleteKey("locked"), false);
        expect_eq(tbl.has("locked"), true);
        const l = tbl.get("locked");
        if (l != null) { expect_eq(l.value, "const"); }
    });

    test("3. mergeDescriptors — redefine=true overwrites configurable keys & protects non-configurable", () => {
        let dst = DescriptorTable.new();
        dst.define(PropDescriptor.data("name", "old"));
        dst.define(PropDescriptor.readonly("id", "42"));

        let src = DescriptorTable.new();
        src.define(PropDescriptor.data("name", "new"));
        src.define(PropDescriptor.data("id", "999"));
        src.define(PropDescriptor.data("env", "prod"));

        const n = mergeDescriptors(&dst, src, true);
        expect_eq(n, 2 as usize);
        expect_eq(dst.len(), 3 as usize);

        const dName = dst.get("name");
        if (dName != null) { expect_eq(dName.value, "new"); }
        const dId = dst.get("id");
        if (dId != null) { expect_eq(dId.value, "42"); }
        const dEnv = dst.get("env");
        if (dEnv != null) { expect_eq(dEnv.value, "prod"); }
    });

    test("4. mergeDescriptors — redefine=false preserves existing keys & only inserts new ones", () => {
        let dst = DescriptorTable.new();
        dst.define(PropDescriptor.data("a", "1"));
        dst.define(PropDescriptor.data("b", "2"));

        let src = DescriptorTable.new();
        src.define(PropDescriptor.data("b", "override"));
        src.define(PropDescriptor.data("c", "3"));
        src.define(PropDescriptor.data("d", "4"));

        const n = mergeDescriptors(&dst, src, false);
        expect_eq(n, 2 as usize);
        expect_eq(dst.len(), 4 as usize);

        const db = dst.get("b");
        if (db != null) { expect_eq(db.value, "2"); }
        const dc = dst.get("c");
        if (dc != null) { expect_eq(dc.value, "3"); }
        const dd = dst.get("d");
        if (dd != null) { expect_eq(dd.value, "4"); }
    });

    test("5. countEnumerable — distinguishes enumerable vs hidden descriptors", () => {
        let tbl = DescriptorTable.new();
        tbl.define(PropDescriptor.data("pub1", "x"));
        tbl.define(PropDescriptor.data("pub2", "y"));
        tbl.define(PropDescriptor {
            key: "_internal",
            value: "secret",
            writable: true,
            enumerable: false,
            configurable: true
        });
        tbl.define(PropDescriptor {
            key: "_meta",
            value: "v1",
            writable: false,
            enumerable: false,
            configurable: false
        });

        expect_eq(tbl.len(), 4 as usize);
        expect_eq(countEnumerable(tbl), 2 as usize);
    });

    test("6. DescriptorTable — fixed 32-slot pool capacity boundary enforcement", () => {
        let tbl = DescriptorTable.new();
        const keys: [33]string = [
            "k00", "k01", "k02", "k03", "k04", "k05", "k06", "k07",
            "k08", "k09", "k10", "k11", "k12", "k13", "k14", "k15",
            "k16", "k17", "k18", "k19", "k20", "k21", "k22", "k23",
            "k24", "k25", "k26", "k27", "k28", "k29", "k30", "k31",
            "k32"
        ];
        let i: usize = 0;
        while (i < MAX_DESCRIPTORS) {
            expect_eq(tbl.define(PropDescriptor.data(keys[i], "ok")), true);
            i = i + 1;
        }
        expect_eq(tbl.len(), 32 as usize);
        // 33rd unique key must fail cleanly without overflow
        expect_eq(tbl.define(PropDescriptor.data(keys[32], "overflow")), false);
        expect_eq(tbl.len(), 32 as usize);
        // Redefining an existing configurable key when full still succeeds
        expect_eq(tbl.define(PropDescriptor.data("k00", "updated")), true);
        const k0 = tbl.get("k00");
        if (k0 != null) { expect_eq(k0.value, "updated"); }
    });
}
