// Production Deep Test for cap:pkg (package_json_editor.cp)
// Covers: dependency group precedence (optional > dev > prod > peer),
// version pin detection & formatting (^ / ~ / exact / --exact / npm: alias preservation),
// cross-group move & only_missing guard, alphabetized trustedDependencies deduplication,
// patchedDependencies CRUD, deterministic JSON serialization, and malformed input guards.

import { test, assert, assertEq } from "cap:test";
import {
    PkgJsonDepGroup,
    pkgJsonDepGroupFieldName,
    parsePkgJsonDepGroup,
    PkgJsonVersionPin,
    detectPkgJsonVersionPin,
    formatEditedVersionSpec,
    PkgJsonEditOptions,
    PkgJsonEditor,
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

test("package_json_editor: group field mapping and precedence", () => {
    assert(strEql(pkgJsonDepGroupFieldName(PkgJsonDepGroup.Optional), "optionalDependencies"));
    assert(strEql(pkgJsonDepGroupFieldName(PkgJsonDepGroup.Dev), "devDependencies"));
    assert(strEql(pkgJsonDepGroupFieldName(PkgJsonDepGroup.Prod), "dependencies"));
    assert(strEql(pkgJsonDepGroupFieldName(PkgJsonDepGroup.Peer), "peerDependencies"));

    if (g := parsePkgJsonDepGroup("devDependencies")) {
        assert(g == PkgJsonDepGroup.Dev);
    } else {
        assert(false);
    }
    assert(parsePkgJsonDepGroup("unknownDependencies") == null);

    // Precedence check when move_between_groups = false
    let ed = PkgJsonEditor.init();
    const keep_opts = PkgJsonEditOptions{ move_between_groups: false };
    * = ed.upsertDep(PkgJsonDepGroup.Peer, "react", ">=18", keep_opts) catch {
        assert(false);
        return;
    };
    * = ed.upsertDep(PkgJsonDepGroup.Prod, "react", "^18.2.0", keep_opts) catch {
        assert(false);
        return;
    };
    if (g1 := ed.findDepGroup("react")) {
        assert(g1 == PkgJsonDepGroup.Prod);
    } else {
        assert(false);
    }

    * = ed.upsertDep(PkgJsonDepGroup.Dev, "react", "^18.3.0", keep_opts) catch {
        assert(false);
        return;
    };
    if (g2 := ed.findDepGroup("react")) {
        assert(g2 == PkgJsonDepGroup.Dev);
    } else {
        assert(false);
    }

    * = ed.upsertDep(PkgJsonDepGroup.Optional, "react", "^18.3.1", keep_opts) catch {
        assert(false);
        return;
    };
    if (g3 := ed.findDepGroup("react")) {
        assert(g3 == PkgJsonDepGroup.Optional);
    } else {
        assert(false);
    }

    // removeDep must clean across all groups
    const removed = ed.removeDep("react");
    assertEq(removed, 4);
    assert(ed.findDepGroup("react") == null);
});

test("package_json_editor: version pin detection and formatEditedVersionSpec", () => {
    assert(detectPkgJsonVersionPin("1.2.3") == PkgJsonVersionPin.Patch);
    assert(detectPkgJsonVersionPin("~1.2.3") == PkgJsonVersionPin.Minor);
    assert(detectPkgJsonVersionPin("^1.2.3") == PkgJsonVersionPin.Major);
    assert(detectPkgJsonVersionPin("latest") == PkgJsonVersionPin.Major);
    assert(detectPkgJsonVersionPin("npm:@scope/pkg@~2.1.0") == PkgJsonVersionPin.Minor);
    assert(detectPkgJsonVersionPin("npm:lodash@4.17.21") == PkgJsonVersionPin.Patch);

    let buf: [64]u8 = [0; 64];

    const n1 = formatEditedVersionSpec("^18.2.0", "18.3.1", false, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n1], "^18.3.1"));

    const n2 = formatEditedVersionSpec("^18.2.0", "18.3.1", true, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n2], "18.3.1"));

    const n3 = formatEditedVersionSpec("~1.4.0", "1.4.9", false, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n3], "~1.4.9"));

    const n4 = formatEditedVersionSpec("npm:@scope/react@~18.2.0", "18.3.1", false, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n4], "npm:@scope/react@~18.3.1"));

    const n5 = formatEditedVersionSpec("npm:@scope/react@^18.2.0", "18.3.1", true, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n5], "npm:@scope/react@18.3.1"));

    // Small buffer must fail
    let tiny: [4]u8 = [0; 4];
    const fail_n = formatEditedVersionSpec("^1.0.0", "2.0.0", false, tiny[0..]) catch 999;
    assertEq(fail_n, 999);
});

test("package_json_editor: cross-group move and only_missing semantics", () => {
    let ed = PkgJsonEditor.init();
    const def_opts = PkgJsonEditOptions{};

    const added1 = ed.upsertDep(PkgJsonDepGroup.Prod, "zod", "^3.22.0", def_opts) catch {
        assert(false);
        return;
    };
    assert(added1);

    // only_missing must skip if package already exists in any group
    const skipped = ed.upsertDep(
        PkgJsonDepGroup.Dev,
        "zod",
        "^3.23.0",
        PkgJsonEditOptions{ only_missing: true },
    ) catch {
        assert(false);
        return;
    };
    assert(!skipped);
    if (v := ed.getDepVersion(PkgJsonDepGroup.Prod, "zod")) {
        assert(strEql(v, "^3.22.0"));
    } else {
        assert(false);
    }

    // Moving from Prod to Dev removes Prod entry automatically
    const moved = ed.upsertDep(PkgJsonDepGroup.Dev, "zod", "^3.23.0", def_opts) catch {
        assert(false);
        return;
    };
    assert(moved);
    assert(ed.getDepVersion(PkgJsonDepGroup.Prod, "zod") == null);
    if (v2 := ed.getDepVersion(PkgJsonDepGroup.Dev, "zod")) {
        assert(strEql(v2, "^3.23.0"));
    } else {
        assert(false);
    }
});

test("package_json_editor: alphabetized trustedDependencies and deduplication", () => {
    let ed = PkgJsonEditor.init();

    const t1 = ed.addTrustedDependency("sharp") catch {
        assert(false);
        return;
    };
    const t2 = ed.addTrustedDependency("esbuild") catch {
        assert(false);
        return;
    };
    const t3 = ed.addTrustedDependency("@parcel/watcher") catch {
        assert(false);
        return;
    };
    const dup = ed.addTrustedDependency("esbuild") catch {
        assert(false);
        return;
    };
    assert(t1);
    assert(t2);
    assert(t3);
    assert(!dup);
    assert(ed.isTrustedDependency("esbuild"));
    assert(!ed.isTrustedDependency("unknown-pkg"));

    // Verify alphabetical order
    if (a0 := ed.trustedAt(0)) {
        assert(strEql(a0, "@parcel/watcher"));
    } else {
        assert(false);
    }
    if (a1 := ed.trustedAt(1)) {
        assert(strEql(a1, "esbuild"));
    } else {
        assert(false);
    }
    if (a2 := ed.trustedAt(2)) {
        assert(strEql(a2, "sharp"));
    } else {
        assert(false);
    }

    let out: [128]u8 = [0; 128];
    const n = ed.formatTrustedJson(out[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(out[0..n], "[\"@parcel/watcher\", \"esbuild\", \"sharp\"]"));
});

test("package_json_editor: patchedDependencies CRUD and deterministic group JSON", () => {
    let ed = PkgJsonEditor.init();

    * = ed.upsertDep(PkgJsonDepGroup.Prod, "zod", "^3.23.0", PkgJsonEditOptions{}) catch {
        assert(false);
        return;
    };
    * = ed.upsertDep(
        PkgJsonDepGroup.Prod,
        "axios",
        "1.7.0",
        PkgJsonEditOptions{ add_trusted_dependencies: true },
    ) catch {
        assert(false);
        return;
    };
    assert(ed.isTrustedDependency("axios"));

    let grp_buf: [128]u8 = [0; 128];
    const gn = ed.formatGroupJson(PkgJsonDepGroup.Prod, grp_buf[0..]) catch {
        assert(false);
        return;
    };
    // Sorted alphabetically: axios before zod
    assert(strEql(grp_buf[0..gn], "{\"axios\": \"1.7.0\", \"zod\": \"^3.23.0\"}"));

    * = ed.setPatchedDependency("zod@3.23.0", "patches/zod@3.23.0.patch") catch {
        assert(false);
        return;
    };
    * = ed.setPatchedDependency("zod@3.23.0", "patches/zod-fixed.patch") catch {
        assert(false);
        return;
    };
    if (pp := ed.getPatchedDependency("zod@3.23.0")) {
        assert(strEql(pp, "patches/zod-fixed.patch"));
    } else {
        assert(false);
    }

    const st = ed.stats();
    assertEq(st.prod_count, 2);
    assertEq(st.trusted_count, 1);
    assertEq(st.patched_count, 1);

    assert(ed.removePatchedDependency("zod@3.23.0"));
    assert(!ed.removePatchedDependency("zod@3.23.0"));
    assert(ed.getPatchedDependency("zod@3.23.0") == null);
});

test("package_json_editor: malformed inputs and control character guards", () => {
    let ed = PkgJsonEditor.init();
    const opts = PkgJsonEditOptions{};

    const bad_name = ed.upsertDep(PkgJsonDepGroup.Prod, "", "^1.0.0", opts) catch false;
    assert(!bad_name);

    const bad_ctrl = ed.upsertDep(PkgJsonDepGroup.Prod, "foo\nbar", "^1.0.0", opts) catch false;
    assert(!bad_ctrl);

    const bad_ver = ed.upsertDep(PkgJsonDepGroup.Prod, "foo", "", opts) catch false;
    assert(!bad_ver);

    const bad_patch = ed.setPatchedDependency("", "patches/a.patch") catch false;
    assert(!bad_patch);

    let out: [16]u8 = [0; 16];
    const bad_fmt = formatEditedVersionSpec("^1.0.0", "", false, out[0..]) catch 999;
    assertEq(bad_fmt, 999);
});
