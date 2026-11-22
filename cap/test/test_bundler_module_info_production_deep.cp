import { test, expect } from "cap:test";
import {
    MODULE_FETCH_JAVASCRIPT,
    MODULE_FETCH_JSON,
    MODULE_STRING_STAR_NAMESPACE,
    ModuleInfoFlags,
    ModuleRecordKind,
    TranspiledModuleInfo,
} from "cap:bundler";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("bundler module_info: string interning dedupe, export dedupe, and first-import fetch attrs", () => {
    let mi = TranspiledModuleInfo.init(false);

    const s0 = mi.internString("./utils.js") catch 99;
    const s1 = mi.internString("foo") catch 99;
    const s0_dup = mi.internString("./utils.js") catch 99;
    expect(s0).toBe(0);
    expect(s1).toBe(1);
    expect(s0_dup).toBe(0);
    expect(mi.string_count).toBe(2);

    if (txt := mi.getString(s0)) {
        expect(sliceEq(txt, "./utils.js")).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    expect(mi.getString(999) == null).toBe(true);

    // First requestModule wins for fetch parameters (JSC semantics)
    * = mi.requestModule(s0, MODULE_FETCH_JSON) catch {};
    * = mi.requestModule(s0, MODULE_FETCH_JAVASCRIPT) catch {};
    expect(mi.requested_count).toBe(1);
    expect(mi.requested_values[0]).toBe(MODULE_FETCH_JSON);

    // Duplicate exported name is ignored on second add
    * = mi.addExportLocal(s1, s1) catch {};
    * = mi.addExportLocal(s1, s1) catch {};
    expect(mi.record_count).toBe(1);
});

test("bundler module_info: finalize promotes local re-exports to indirect and marks TS single import", () => {
    // TypeScript: `import { Foo } from "./types.ts"; export { Foo };`
    let mi = TranspiledModuleInfo.init(true);
    const mod_id = mi.internString("./types.ts") catch 99;
    const foo_id = mi.internString("Foo") catch 99;
    const local_bar = mi.internString("bar") catch 99;

    * = mi.addImportSingle(mod_id, foo_id, foo_id, false) catch {};
    * = mi.addExportLocal(foo_id, foo_id) catch {};
    * = mi.addLexicalVariable(local_bar) catch {};
    * = mi.addExportLocal(local_bar, local_bar) catch {};

    let fin_ok = true;
    mi.finalize() catch {
        fin_ok = false;
    };
    expect(fin_ok).toBe(true);

    // Record 0 (import Foo) upgraded to ImportInfoSingleTypeScript
    expect(mi.record_kinds[0] == ModuleRecordKind.ImportInfoSingleTypeScript).toBe(true);
    // Record 1 (export Foo) promoted to ExportInfoIndirect: [export=Foo, import=Foo, module=./types.ts]
    expect(mi.record_kinds[1] == ModuleRecordKind.ExportInfoIndirect).toBe(true);
    expect(mi.buffer[3]).toBe(foo_id);
    expect(mi.buffer[4]).toBe(foo_id);
    expect(mi.buffer[5]).toBe(mod_id);
    // Record 3 (export bar) stays ExportInfoLocal
    expect(mi.record_kinds[3] == ModuleRecordKind.ExportInfoLocal).toBe(true);
});

test("bundler module_info: namespace import re-export promotes to indirect with STAR_NAMESPACE", () => {
    // `import * as ns from "./ns.js"; export { ns as renamed };`
    let mi = TranspiledModuleInfo.init(true);
    const mod_id = mi.internString("./ns.js") catch 99;
    const ns_id = mi.internString("ns") catch 99;
    const renamed_id = mi.internString("renamed") catch 99;

    * = mi.addImportNamespace(mod_id, ns_id) catch {};
    * = mi.addExportLocal(renamed_id, ns_id) catch {};
    * = mi.finalize() catch {};

    // Namespace import record kind stays ImportInfoNamespace (not converted to SingleTypeScript)
    expect(mi.record_kinds[0] == ModuleRecordKind.ImportInfoNamespace).toBe(true);
    // Export promoted to ExportInfoIndirect: [export=renamed, import=STAR_NAMESPACE, module=./ns.js]
    expect(mi.record_kinds[1] == ModuleRecordKind.ExportInfoIndirect).toBe(true);
    expect(mi.buffer[3]).toBe(renamed_id);
    expect(mi.buffer[4]).toBe(MODULE_STRING_STAR_NAMESPACE);
    expect(mi.buffer[5]).toBe(mod_id);
});

test("bundler module_info: replaceStringId remaps cross-chunk specifiers in records and requested_modules", () => {
    let mi = TranspiledModuleInfo.init(false);
    const old_path = mi.internString("./raw-dep.js") catch 99;
    const new_path = mi.internString("./chunk-A1B2.js") catch 99;
    const sym = mi.internString("run") catch 99;

    * = mi.requestModule(old_path, MODULE_FETCH_JAVASCRIPT) catch {};
    * = mi.addImportSingle(old_path, sym, sym, false) catch {};
    * = mi.addExportStar(old_path) catch {};

    let rep_ok = true;
    mi.replaceStringId(old_path, new_path) catch {
        rep_ok = false;
    };
    expect(rep_ok).toBe(true);
    expect(mi.requested_keys[0]).toBe(new_path);
    expect(mi.buffer[0]).toBe(new_path);
    expect(mi.buffer[3]).toBe(new_path);
});

test("bundler module_info: 4-byte aligned binary serialize and deserialize lossless round-trip", () => {
    let mi = TranspiledModuleInfo.init(true);
    mi.flags.contains_import_meta = true;
    mi.flags.has_tla = true;

    const m1 = mi.internString("./alpha.ts") catch 99;
    const m2 = mi.internString("./beta.json") catch 99;
    const x = mi.internString("x") catch 99;
    const cfg = mi.internString("cfg") catch 99;

    * = mi.requestModule(m1, MODULE_FETCH_JAVASCRIPT) catch {};
    * = mi.requestModule(m2, MODULE_FETCH_JSON) catch {};
    * = mi.addDeclaredVariable(x) catch {};
    * = mi.addImportSingle(m1, x, x, false) catch {};
    * = mi.addExportNamespace(cfg, m2) catch {}; // 3 records -> triggers 1-byte alignment padding
    * = mi.finalize() catch {};

    let wire: [1024]u8 = [0; 1024];
    const written = mi.serialize(wire[0..]) catch 0;
    expect(written > 0).toBe(true);

    const decoded = TranspiledModuleInfo.deserialize(wire[0..written]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(decoded.finalized).toBe(true);
    expect(decoded.flags.is_typescript).toBe(true);
    expect(decoded.flags.contains_import_meta).toBe(true);
    expect(decoded.flags.has_tla).toBe(true);
    expect(decoded.record_count).toBe(3);
    expect(decoded.record_kinds[0] == ModuleRecordKind.DeclaredVariable).toBe(true);
    expect(decoded.record_kinds[1] == ModuleRecordKind.ImportInfoSingle).toBe(true);
    expect(decoded.record_kinds[2] == ModuleRecordKind.ExportInfoNamespace).toBe(true);
    expect(decoded.requested_count).toBe(2);
    expect(decoded.requested_values[1]).toBe(MODULE_FETCH_JSON);
    expect(decoded.string_count).toBe(4);
    if (s := decoded.getString(m2)) {
        expect(sliceEq(s, "./beta.json")).toBe(true);
    } else {
        expect(false).toBe(true);
    }

    // Flags bit-packing check
    const fb = mi.flags.toByte();
    const fp = ModuleInfoFlags.fromByte(fb);
    expect(fp.contains_import_meta && fp.is_typescript && fp.has_tla).toBe(true);
});

test("bundler module_info: lifecycle and corrupt/truncated binary error guards", () => {
    let mi = TranspiledModuleInfo.init(false);
    let wire: [64]u8 = [0; 64];

    // Serialize before finalize -> NotFinalized
    let not_fin_err = false;
    * = mi.serialize(wire[0..]) catch {
        not_fin_err = true;
    };
    expect(not_fin_err).toBe(true);

    // Finalize once succeeds, second finalize -> AlreadyFinalized
    * = mi.finalize() catch {};
    let dup_fin_err = false;
    mi.finalize() catch {
        dup_fin_err = true;
    };
    expect(dup_fin_err).toBe(true);

    // Mutate after finalize -> AlreadyFinalized
    let mut_err = false;
    mi.addDeclaredVariable(0) catch {
        mut_err = true;
    };
    expect(mut_err).toBe(true);

    // Truncated binary -> CorruptBinary
    const truncated: [3]u8 = [1, 0, 0];
    let trunc_err = false;
    * = TranspiledModuleInfo.deserialize(truncated[0..]) catch {
        trunc_err = true;
    };
    expect(trunc_err).toBe(true);

    // Invalid record kind byte (99) -> InvalidRecordKind
    const bad_kind: [8]u8 = [1, 0, 0, 0, 99, 0, 0, 0];
    let bad_kind_err = false;
    * = TranspiledModuleInfo.deserialize(bad_kind[0..]) catch {
        bad_kind_err = true;
    };
    expect(bad_kind_err).toBe(true);
});
