import { test, expect, expect_eq } from "cap:test";
import {
    PKG_SCRIPT_SLOT_COUNT,
    PkgScriptListError,
    PkgScriptSlotKind,
    PkgScriptFormatMode,
    PkgScriptTrustSource,
    isScriptExecutionAllowed,
    formatNodeModulesRelativeCwd,
    PkgScriptList,
} from "cap:pkg";

fn strEql(a: []const u8, b: []const u8): bool {
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

test("pkg script_list: PkgScriptSlotKind 6-phase roundtrip and unknown name guard", fn()! {
    expect_eq(PKG_SCRIPT_SLOT_COUNT, 6);

    const names: [6][]const u8 = ["preinstall", "install", "postinstall", "preprepare", "prepare", "postprepare"];
    let i: usize = 0;
    while (i < 6) {
        const slot = PkgScriptSlotKind.fromIndex(i) ?? return err.TestFailed;
        expect_eq(slot.slotIndex(), i);
        expect(strEql(slot.asName(), names[i]));
        const parsed = PkgScriptSlotKind.fromName(names[i]) ?? return err.TestFailed;
        expect_eq(parsed.slotIndex(), i);
        i += 1;
    }
    expect(PkgScriptSlotKind.fromIndex(6) == null);
    expect(PkgScriptSlotKind.fromName("test") == null);
});

test("pkg script_list: PkgScriptList set/clear/first ordering and eql", fn()! {
    let list = PkgScriptList.init("esbuild", "0.24.0", "/app/node_modules/esbuild");
    expect(list.isEmpty());
    expect_eq(list.count(), 0);
    expect(list.first() == null);

    list.set(.Postinstall, "node install.js");
    list.set(.Prepare, "npm run build");
    expect_eq(list.count(), 2);
    expect(strEql(list.first() ?? "", "node install.js"));

    // Adding an earlier phase updates first()
    list.set(.Preinstall, "node check.js");
    expect_eq(list.count(), 3);
    expect(strEql(list.first() ?? "", "node check.js"));

    // Clearing preinstall restores postinstall as first()
    list.set(.Preinstall, "");
    expect_eq(list.count(), 2);
    expect(strEql(list.first() ?? "", "node install.js"));

    let other = PkgScriptList.init("esbuild", "0.24.0", "/other");
    other.set(.Postinstall, "node install.js");
    other.set(.Prepare, "npm run build");
    expect(list.eql(other));

    other.set(.Prepare, "different");
    expect(!list.eql(other));
});

test("pkg script_list: formatNodeModulesRelativeCwd trims node_modules and trailing slashes", fn()! {
    let buf: [128]u8 = [0; 128];

    const n1 = formatNodeModulesRelativeCwd("/data/repo/app/node_modules/esbuild///", buf[0..])?;
    expect(strEql(buf[0..n1], "./node_modules/esbuild"));

    const n2 = formatNodeModulesRelativeCwd("/data/repo/packages/core/", buf[0..])?;
    expect(strEql(buf[0..n2], "/data/repo/packages/core"));

    const n3 = formatNodeModulesRelativeCwd("/", buf[0..])?;
    expect(strEql(buf[0..n3], "/"));
});

test("pkg script_list: PkgScriptList.format renders Completed, Untrusted, and Info modes", fn()! {
    let list = PkgScriptList.init("esbuild", "0.24.0", "/repo/node_modules/esbuild/");
    list.set(.Postinstall, "node install.js");
    list.set(.Prepare, "make build");

    let buf: [256]u8 = [0; 256];

    const n_done = list.format(.Completed, buf[0..])?;
    expect(strEql(
        buf[0..n_done],
        "./node_modules/esbuild @0.24.0\n ✓ [postinstall]: node install.js\n ✓ [prepare]: make build\n",
    ));

    const n_untrusted = list.format(.Untrusted, buf[0..])?;
    expect(strEql(
        buf[0..n_untrusted],
        "./node_modules/esbuild @0.24.0\n » [postinstall]: node install.js\n » [prepare]: make build\n",
    ));

    const n_info = list.format(.Info, buf[0..])?;
    expect(strEql(
        buf[0..n_info],
        "./node_modules/esbuild @0.24.0\n [postinstall]: node install.js\n [prepare]: make build\n",
    ));
});

test("pkg script_list: isScriptExecutionAllowed enforces source isolation and --ignore-scripts", fn()! {
    const explicit: [1][]const u8 = ["my-native-addon"];
    const defaults: [2][]const u8 = ["esbuild", "sharp"];

    // Default allowlist applies ONLY to NpmRegistry
    expect(isScriptExecutionAllowed("esbuild", .NpmRegistry, false, explicit[0..], defaults[0..]));
    expect(!isScriptExecutionAllowed("esbuild", .LocalFile, false, explicit[0..], defaults[0..]));
    expect(!isScriptExecutionAllowed("esbuild", .Symlink, false, explicit[0..], defaults[0..]));
    expect(!isScriptExecutionAllowed("esbuild", .Git, false, explicit[0..], defaults[0..]));
    expect(!isScriptExecutionAllowed("esbuild", .Tarball, false, explicit[0..], defaults[0..]));

    // Explicit trustedDependencies allows any source
    expect(isScriptExecutionAllowed("my-native-addon", .Git, false, explicit[0..], defaults[0..]));
    expect(isScriptExecutionAllowed("my-native-addon", .LocalFile, false, explicit[0..], defaults[0..]));

    // --ignore-scripts blocks everything, even explicit trustedDependencies
    expect(!isScriptExecutionAllowed("my-native-addon", .NpmRegistry, true, explicit[0..], defaults[0..]));
    expect(!isScriptExecutionAllowed("esbuild", .NpmRegistry, true, explicit[0..], defaults[0..]));

    // Unlisted and empty names blocked
    expect(!isScriptExecutionAllowed("untrusted-pkg", .NpmRegistry, false, explicit[0..], defaults[0..]));
    expect(!isScriptExecutionAllowed("", .NpmRegistry, false, explicit[0..], defaults[0..]));
});

test("pkg script_list: BufferTooSmall guards on formatting", fn()! {
    let list = PkgScriptList.init("pkg", "1.0.0", "/app/node_modules/pkg");
    list.set(.Install, "node-gyp rebuild");

    let tiny: [6]u8 = [0; 6];
    let errs: usize = 0;
    if (_ := formatNodeModulesRelativeCwd("/app/node_modules/pkg", tiny[0..])) {} else { errs += 1; }
    if (_ := list.format(.Info, tiny[0..])) {} else { errs += 1; }
    expect_eq(errs, 2);
});
