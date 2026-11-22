// cap/test/test_pkg_workspace_pkg_cache_production_deep.cp
// 深度生产级回归测试：cap:pkg Monorepo Workspace package.json 缓存与缩进/JSONC 分析器 (lib/pkg/workspace_pkg_cache.cp)

import { test } from "cap:test";
import {
    WORKSPACE_PKG_CACHE_MAX_ENTRIES,
    WorkspacePkgJsonOptions,
    isAbsoluteWorkspacePath,
    normalizeWorkspacePkgJsonPath,
    guessJsonIndentation,
    analyzeWorkspacePackageJson,
    WorkspacePkgJsonCache,
} from "cap:pkg";

test("workspace_pkg_cache: absolute path validation & cross-platform POSIX normalization") {
    if (!isAbsoluteWorkspacePath("/repo/packages/a/package.json")) return err.PosixShouldBeAbs;
    if (!isAbsoluteWorkspacePath("C:\\repo\\packages\\a\\package.json")) return err.WinBackslashShouldBeAbs;
    if (!isAbsoluteWorkspacePath("D:/work/package.json")) return err.WinSlashShouldBeAbs;
    if (isAbsoluteWorkspacePath("packages/a/package.json")) return err.RelShouldNotBeAbs;
    if (isAbsoluteWorkspacePath("")) return err.EmptyShouldNotBeAbs;

    let buf: [128]u8 = [0; 128];
    const n1 = normalizeWorkspacePkgJsonPath("C:\\repo\\\\packages\\a\\package.json", buf[0..])?;
    if (n1 != 30) return err.NormWinLenMismatch;
    if (buf[0] != 'C' || buf[1] != ':' || buf[2] != '/' || buf[7] != '/') return err.NormWinSepMismatch;

    const n2 = normalizeWorkspacePkgJsonPath("//repo//pkg//", buf[0..])?;
    // "//repo//pkg//" -> "/repo/pkg" (9 bytes)
    if (n2 != 9 || buf[0] != '/' || buf[8] != 'g') return err.NormPosixCollapseMismatch;

    // 相对路径必须报错
    let rel_err = false;
    * = normalizeWorkspacePkgJsonPath("./package.json", buf[0..]) catch {
        rel_err = true;
        0;
    };
    if (!rel_err) return err.RelativePathMustFail;
}

test("workspace_pkg_cache: JSON indentation auto-detection (spaces, tabs & comment skip)") {
    const src_2sp = "{\n  \"name\": \"a\"\n}";
    const i2 = guessJsonIndentation(src_2sp);
    if (!i2.detected || i2.is_tabs || i2.width != 2) return err.Expected2Spaces;

    const src_4sp = "{\n    // leading comment\n    \"name\": \"b\"\n}";
    const i4 = guessJsonIndentation(src_4sp);
    if (!i4.detected || i4.is_tabs || i4.width != 4) return err.Expected4Spaces;

    const src_tab = "{\n\t\"name\": \"c\"\n}";
    const itab = guessJsonIndentation(src_tab);
    if (!itab.detected || !itab.is_tabs || itab.width != 1) return err.ExpectedTabs;

    const src_min = "{\"name\":\"minified\"}";
    const imin = guessJsonIndentation(src_min);
    if (imin.detected || imin.width != 2) return err.ExpectedDefaultMinified;
}

test("workspace_pkg_cache: JSONC comments & trailing comma detection + strict rejection") {
    const jsonc = "{\n  /* block */\n  \"name\": \"pkg-a\",\n  // line\n  \"version\": \"1.0.0\",\n}";
    const sum = analyzeWorkspacePackageJson(jsonc, WorkspacePkgJsonOptions{})?;
    if (!sum.has_comments || !sum.has_trailing_comma || sum.content_hash == 0 || sum.indentation.width != 2) {
        return err.JsoncSummaryMismatch;
    }

    // 禁止注释时应拦截
    let no_comment_err = false;
    * = analyzeWorkspacePackageJson(jsonc, WorkspacePkgJsonOptions{ allow_comments: false, allow_trailing_commas: true, guess_indentation: false }) catch {
        no_comment_err = true;
    };
    if (!no_comment_err) return err.CommentsRejectionFailed;

    // 禁止尾逗号时应拦截
    const only_trailing = "{\"name\": \"a\",}";
    let no_trailing_err = false;
    * = analyzeWorkspacePackageJson(only_trailing, WorkspacePkgJsonOptions{ allow_comments: true, allow_trailing_commas: false, guess_indentation: false }) catch {
        no_trailing_err = true;
    };
    if (!no_trailing_err) return err.TrailingCommaRejectionFailed;
}

test("workspace_pkg_cache: cross-slash cache hit, lookup, invalidate & parse-error rollback") {
    let cache = WorkspacePkgJsonCache.init();
    const pkg_a = "{\n  \"name\": \"pkg-a\",\n  \"version\": \"1.0.0\"\n}";

    const e1 = cache.getOrInsert("C:\\mono\\packages\\a\\package.json", pkg_a, WorkspacePkgJsonOptions{})?;
    if (e1.hit_count != 0 || cache.len() != 1) return err.FirstInsertMismatch;

    // 不同斜杠写法命中同一缓存键
    const e2 = cache.getOrInsert("C:/mono/packages/a/package.json", pkg_a, WorkspacePkgJsonOptions{})?;
    if (e2.hit_count != 1 || e2.content_hash != e1.content_hash) return err.CrossSlashHitMismatch;

    // lookup 命中再次递增
    if (found := cache.lookup("C:\\mono\\packages\\a\\package.json")?) {
        if (found.hit_count != 2) return err.LookupHitCountMismatch;
    } else {
        return err.ExpectedLookupHit;
    }

    // 解析失败不污染缓存
    let bad_err = false;
    * = cache.getOrInsert("/mono/packages/broken/package.json", "{ \"unterminated", WorkspacePkgJsonOptions{}) catch {
        bad_err = true;
    };
    if (!bad_err || cache.len() != 1 || cache.stats().parse_errors != 1) {
        return err.ParseErrorMustNotPolluteCache;
    }

    // 失效移除
    const removed = cache.invalidate("C:/mono/packages/a/package.json")?;
    if (!removed || cache.len() != 0 || cache.stats().invalidations != 1) {
        return err.InvalidateMismatch;
    }
    if ((cache.lookup("C:/mono/packages/a/package.json")?) != null) {
        return err.LookupAfterInvalidateMustBeNull;
    }
}

test("workspace_pkg_cache: malformed JSON root & trailing garbage rejection") {
    const cases_bad: [4]([]const u8) = [
        "[1, 2, 3]",
        "{\"a\": 1} extra",
        "{\"a\": \"unclosed}",
        "{\"a\": /* unclosed comment }",
    ];
    let i: usize = 0;
    while (i < 4) {
        let rejected = false;
        * = analyzeWorkspacePackageJson(cases_bad[i], WorkspacePkgJsonOptions{}) catch {
            rejected = true;
        };
        if (!rejected) return err.MalformedJsonMustFail;
        i += 1;
    }
}

test("workspace_pkg_cache: capacity saturation (32 slots), CacheFull & clear reuse") {
    let cache = WorkspacePkgJsonCache.init();
    const valid = "{\n  \"name\": \"w\"\n}";
    let path_buf: [16]u8 = ['/', 'w', '/', '0', '0', '/', 'p', 'k', 'g', '.', 'j', 's', 'o', 'n', 0, 0];

    let i: usize = 0;
    while (i < WORKSPACE_PKG_CACHE_MAX_ENTRIES) {
        path_buf[3] = ('0' as u8) + ((i / 10) as u8);
        path_buf[4] = ('0' as u8) + ((i % 10) as u8);
        * = cache.getOrInsert(path_buf[0..14], valid, WorkspacePkgJsonOptions{})?;
        i += 1;
    }
    if (cache.len() != WORKSPACE_PKG_CACHE_MAX_ENTRIES) return err.ExpectedFull32;

    let full_err = false;
    * = cache.getOrInsert("/w/99/pkg.json", valid, WorkspacePkgJsonOptions{}) catch {
        full_err = true;
    };
    if (!full_err) return err.ExpectedCacheFull;

    cache.clear();
    if (cache.len() != 0) return err.ClearFailed;
    * = cache.getOrInsert("/w/99/pkg.json", valid, WorkspacePkgJsonOptions{})?;
    if (cache.len() != 1) return err.ReuseAfterClearFailed;
}
