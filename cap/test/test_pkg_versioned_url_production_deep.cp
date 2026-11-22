import { test, expect, expect_eq } from "cap:test";
import {
    VERSIONED_URL_MAX_ENTRIES,
    PkgSemverTriple,
    LegacyV2VersionedUrl,
    VersionedUrl,
    VersionedUrlSet,
} from "cap:pkg";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("pkg versioned_url: parse Semver 2.0 triples, prerelease precedence & build metadata", fn() {
    const v1 = PkgSemverTriple.parse("v1.2.3") catch {
        expect(false);
        return;
    };
    expect_eq(v1.major, 1);
    expect_eq(v1.minor, 2);
    expect_eq(v1.patch, 3);
    expect(!v1.isPrerelease());

    const v1_build = PkgSemverTriple.parse("1.2.3+20261122.abc") catch {
        expect(false);
        return;
    };
    // Semver 2.0: 忽略 +build 元数据
    expect(v1.eql(v1_build));
    expect_eq(v1.compare(v1_build), 0);

    const v_alpha2 = PkgSemverTriple.parse("1.2.3-alpha.2") catch {
        expect(false);
        return;
    };
    const v_alpha10 = PkgSemverTriple.parse("1.2.3-alpha.10") catch {
        expect(false);
        return;
    };
    const v_beta = PkgSemverTriple.parse("1.2.3-beta.1") catch {
        expect(false);
        return;
    };
    expect(v_alpha2.isPrerelease());
    expect_eq(v_alpha2.compare(v_alpha10), -1);
    expect_eq(v_alpha10.compare(v_beta), -1);
    expect_eq(v_beta.compare(v1), -1);

    let out: [64]u8 = [0; 64];
    const n = v1_build.formatInto(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n], "1.2.3+20261122.abc"));
});

test("pkg versioned_url: VersionedUrl creation, tarball/https checks & lock formatting", fn() {
    const u1 = VersionedUrl.parse(
        "https://registry.npmjs.org/zod/-/zod-3.24.1.tgz",
        "3.24.1",
    ) catch {
        expect(false);
        return;
    };
    expect(u1.isHttps());
    expect(u1.isTarball());

    const u2 = VersionedUrl.parse(
        "https://mirror.example.com/zod-3.24.1.tgz",
        "3.24.1+mirror",
    ) catch {
        expect(false);
        return;
    };
    // 对齐 Bun VersionedURL.eql：仅比对版本等价
    expect(u1.eql(u2));
    // eqlStrict 同时比对 URL
    expect(!u1.eqlStrict(u2));

    let out: [128]u8 = [0; 128];
    const n = u1.formatLockEntry("zod", out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n], "zod@3.24.1 -> https://registry.npmjs.org/zod/-/zod-3.24.1.tgz"));
});

test("pkg versioned_url: LegacyV2VersionedUrl u32 -> u64 migration", fn() {
    const legacy = LegacyV2VersionedUrl{
        url: "https://registry.npmjs.org/react/-/react-19.1.0.tgz",
        major: 19,
        minor: 1,
        patch: 0,
        prerelease: "",
        build: "",
    };
    const migrated = legacy.migrate() catch {
        expect(false);
        return;
    };
    expect_eq(migrated.version.major, 19);
    expect_eq(migrated.version.minor, 1);
    expect_eq(migrated.version.patch, 0);
    expect(migrated.isTarball());
    expect(migrated.isHttps());

    const bad_legacy = LegacyV2VersionedUrl{
        url: "ftp://invalid.example.com/pkg.tgz",
        major: 1,
        minor: 0,
        patch: 0,
    };
    let mig_ok = true;
    * = bad_legacy.migrate() catch {
        mig_ok = false;
    };
    expect(!mig_ok);
});

test("pkg versioned_url: VersionedUrlSet dedupe, sortAsc/sortDesc, latestStable & latestAny", fn() {
    let set = VersionedUrlSet.init();
    * = set.push(VersionedUrl.parse("https://r.io/a-2.0.0-rc.1.tgz", "2.0.0-rc.1") catch {
        expect(false);
        return;
    }) catch {
        expect(false);
        return;
    };
    * = set.push(VersionedUrl.parse("https://r.io/a-1.9.0.tgz", "1.9.0") catch {
        expect(false);
        return;
    }) catch {
        expect(false);
        return;
    };
    * = set.push(VersionedUrl.parse("https://r.io/a-1.10.0.tgz", "1.10.0") catch {
        expect(false);
        return;
    }) catch {
        expect(false);
        return;
    };
    // 相同版本覆盖去重
    * = set.push(VersionedUrl.parse("https://cdn.io/a-1.10.0.tgz", "1.10.0") catch {
        expect(false);
        return;
    }) catch {
        expect(false);
        return;
    };
    expect_eq(set.count(), 3);

    set.sortAsc();
    if (first := set.at(0)) {
        expect_eq(first.version.minor, 9);
    } else {
        expect(false);
    }
    if (last := set.at(2)) {
        expect_eq(last.version.major, 2);
    } else {
        expect(false);
    }

    if (stable := set.latestStable()) {
        expect_eq(stable.version.major, 1);
        expect_eq(stable.version.minor, 10);
        expect(strEq(stable.url, "https://cdn.io/a-1.10.0.tgz"));
    } else {
        expect(false);
    }

    if (any_ver := set.latestAny()) {
        expect_eq(any_ver.version.major, 2);
        expect(any_ver.version.isPrerelease());
    } else {
        expect(false);
    }

    set.sortDesc();
    if (top := set.at(0)) {
        expect_eq(top.version.major, 2);
    } else {
        expect(false);
    }

    const q = PkgSemverTriple.parse("1.9.0") catch {
        expect(false);
        return;
    };
    if (found := set.findByVersion(q)) {
        expect(strEq(found.url, "https://r.io/a-1.9.0.tgz"));
    } else {
        expect(false);
    }
});

test("pkg versioned_url: malformed versions, leading zeros & invalid URL guards", fn() {
    // 前导零非法
    let ok1 = true;
    * = PkgSemverTriple.parse("01.2.3") catch {
        ok1 = false;
    };
    expect(!ok1);

    // 缺段非法
    let ok2 = true;
    * = PkgSemverTriple.parse("1.2") catch {
        ok2 = false;
    };
    expect(!ok2);

    // 空 prerelease / 空 build 非法
    let ok3 = true;
    * = PkgSemverTriple.parse("1.2.3-") catch {
        ok3 = false;
    };
    expect(!ok3);

    let ok4 = true;
    * = PkgSemverTriple.parse("1.2.3+") catch {
        ok4 = false;
    };
    expect(!ok4);

    // 非法 URL scheme 或含空格
    let ok5 = true;
    * = VersionedUrl.parse("javascript:alert(1)", "1.0.0") catch {
        ok5 = false;
    };
    expect(!ok5);

    let ok6 = true;
    * = VersionedUrl.parse("https://example.com/a b.tgz", "1.0.0") catch {
        ok6 = false;
    };
    expect(!ok6);
});

test("pkg versioned_url: capacity limits and buffer-too-small guard", fn() {
    expect_eq(VERSIONED_URL_MAX_ENTRIES, 64);

    let set = VersionedUrlSet.init();
    expect(set.latestStable() == null);
    expect(set.latestAny() == null);
    expect(set.at(0) == null);

    let i: u64 = 0;
    while (i < VERSIONED_URL_MAX_ENTRIES) {
        const entry = VersionedUrl.init(
            "https://registry.npmjs.org/x/-/x.tgz",
            PkgSemverTriple{ major: 1, minor: 0, patch: i },
        ) catch {
            expect(false);
            return;
        };
        * = set.push(entry) catch {
            expect(false);
            return;
        };
        i += 1;
    }
    expect_eq(set.count(), VERSIONED_URL_MAX_ENTRIES);

    const overflow_entry = VersionedUrl.init(
        "https://registry.npmjs.org/x/-/x.tgz",
        PkgSemverTriple{ major: 2, minor: 0, patch: 0 },
    ) catch {
        expect(false);
        return;
    };
    let cap_ok = true;
    * = set.push(overflow_entry) catch {
        cap_ok = false;
    };
    expect(!cap_ok);

    let tiny: [4]u8 = [0; 4];
    let buf_ok = true;
    * = overflow_entry.formatLockEntry("pkg", tiny[0..]) catch {
        buf_ok = false;
    };
    expect(!buf_ok);
});
