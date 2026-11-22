// cap/test/test_resolver_path_name_production_deep.cp
// 深度生产级回归测试：cap:resolver 零拷贝路径拆解与裸包名解析引擎 (lib/resolver/path_name.cp)

import { test } from "cap:test";
import {
    PathPlatform,
    ResolverPathName,
    NodeJSPathName,
    PackageSpecifier,
    isRelativeModuleSpecifier,
    isBarePackageSpecifier,
    stripNodeModulesPrefix,
} from "cap:resolver";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("resolver_path_name: POSIX decomposition, dotfiles, multi-ext & trailing slashes") {
    const p1 = ResolverPathName.init("/workspace/src/app.tsx");
    if (!bytesEq(p1.dir, "/workspace/src")) return err.DirMismatch;
    if (!bytesEq(p1.filename, "app.tsx")) return err.FilenameMismatch;
    if (!bytesEq(p1.base, "app")) return err.BaseMismatch;
    if (!bytesEq(p1.ext, ".tsx") || !bytesEq(p1.extWithoutLeadingDot(), "tsx")) return err.ExtMismatch;

    // 点文件（.gitignore）不将整个名字视为扩展名
    const p_dot = ResolverPathName.init("/repo/.gitignore");
    if (!bytesEq(p_dot.dir, "/repo") || !bytesEq(p_dot.filename, ".gitignore") ||
        !bytesEq(p_dot.base, ".gitignore") || p_dot.ext.len != 0)
    {
        return err.DotfileHandlingFailed;
    }

    // 多段扩展名（types.d.ts）剥离最后一段扩展名
    const p_multi = ResolverPathName.init("types.d.ts");
    if (!bytesEq(p_multi.dirOrDot(), ".") || !bytesEq(p_multi.base, "types.d") || !bytesEq(p_multi.ext, ".ts")) {
        return err.MultiExtFailed;
    }

    // 尾斜杠折叠与根目录处理
    const p_trail = ResolverPathName.init("/a/b/c///");
    if (!bytesEq(p_trail.dir, "/a/b") || !bytesEq(p_trail.base, "c") || p_trail.ext.len != 0) {
        return err.TrailingSlashFailed;
    }

    const p_root = ResolverPathName.init("/");
    if (!bytesEq(p_root.dir, "/") || p_root.filename.len != 0) return err.RootPathFailed;

    if (!bytesEq(ResolverPathName.findExtname("/a/b/file.mjs"), ".mjs")) return err.FindExtFailed;
    if (ResolverPathName.findExtname("/a/b/.env").len != 0) return err.FindExtDotfileFailed;
}

test("resolver_path_name: Windows drive letters & backslash separators") {
    const pw = ResolverPathName.init("C:\\Users\\dev\\project\\main.ts");
    if (!bytesEq(pw.dir, "C:\\Users\\dev\\project")) return err.WinDirMismatch;
    if (!bytesEq(pw.filename, "main.ts")) return err.WinFilenameMismatch;
    if (!bytesEq(pw.base, "main") || !bytesEq(pw.ext, ".ts")) return err.WinBaseExtMismatch;

    const pw_root = ResolverPathName.init("D:\\");
    if (!bytesEq(pw_root.dir, "D:\\") || pw_root.filename.len != 0) return err.WinRootFailed;
}

test("resolver_path_name: index-dir naming & zero-alloc JS identifier formatting") {
    // /bar/foo/index.js -> nonUniqueNameStringBase() == "foo"
    const p_idx = ResolverPathName.init("/bar/my-widget/index.js");
    if (!bytesEq(p_idx.nonUniqueNameStringBase(), "my-widget")) return err.IndexParentNameFailed;

    let id_buf: [64]u8 = [0; 64];
    const n1 = p_idx.formatValidIdentifier(id_buf[0..])?;
    if (!bytesEq(id_buf[0..n1], "my_widget")) return err.IdentSanitizeFailed;

    // 首字符为数字时前补 '_'
    const p_num = ResolverPathName.init("/pkg/123-mod.v2.ts");
    const n2 = p_num.formatValidIdentifier(id_buf[0..])?;
    if (!bytesEq(id_buf[0..n2], "_123_mod_v2")) return err.DigitIdentPrefixFailed;

    // 根目录下 /index.js 保留 "index"
    const p_root_idx = ResolverPathName.init("/index.js");
    if (!bytesEq(p_root_idx.nonUniqueNameStringBase(), "index")) return err.RootIndexFallbackFailed;
}

test("resolver_path_name: NodeJSPathName POSIX vs Windows parity") {
    const np = NodeJSPathName.init("/usr/local/lib/react.cjs", PathPlatform.Posix);
    if (!bytesEq(np.dir, "/usr/local/lib") || !bytesEq(np.base, "react.cjs") ||
        !bytesEq(np.filename, "react") || !bytesEq(np.ext, ".cjs"))
    {
        return err.NodePosixMismatch;
    }

    const nw = NodeJSPathName.init("C:\\pkg\\sub\\util.mjs\\", PathPlatform.Windows);
    if (!bytesEq(nw.dir, "C:\\pkg\\sub") || !bytesEq(nw.base, "util.mjs") ||
        !bytesEq(nw.filename, "util") || !bytesEq(nw.ext, ".mjs"))
    {
        return err.NodeWinMismatch;
    }

    // 单斜杠根文件 /app.js：dir=""、base="app.js"（对齐 Bun NodeJSPathName）
    const n_top = NodeJSPathName.init("/app.js", PathPlatform.Posix);
    if (n_top.dir.len != 0 || !bytesEq(n_top.base, "app.js") || !bytesEq(n_top.filename, "app")) {
        return err.NodeTopSlashMismatch;
    }
}

test("resolver_path_name: PackageSpecifier scoped, unscoped & subpath round-trip") {
    // 1. 作用域包 + 子路径
    const s1 = PackageSpecifier.parse("@babel/core/lib/parse.js")?;
    if (!s1.is_scoped || !bytesEq(s1.scope, "@babel") || !bytesEq(s1.name, "core") ||
        !bytesEq(s1.full_name, "@babel/core") || !bytesEq(s1.subpath, "lib/parse.js"))
    {
        return err.ScopedSubpathMismatch;
    }

    // 2. 作用域包无子路径
    const s2 = PackageSpecifier.parse("@types/node")?;
    if (!s2.is_scoped || !bytesEq(s2.full_name, "@types/node") || s2.subpath.len != 0) {
        return err.ScopedRootMismatch;
    }

    // 3. 普通裸包 + 子路径
    const s3 = PackageSpecifier.parse("lodash/fp/map")?;
    if (s3.is_scoped || s3.scope.len != 0 || !bytesEq(s3.name, "lodash") ||
        !bytesEq(s3.full_name, "lodash") || !bytesEq(s3.subpath, "fp/map"))
    {
        return err.UnscopedSubpathMismatch;
    }

    // 4. 普通裸包无子路径
    const s4 = PackageSpecifier.parse("react")?;
    if (s4.is_scoped || !bytesEq(s4.full_name, "react") || s4.subpath.len != 0) {
        return err.UnscopedRootMismatch;
    }
}

test("resolver_path_name: specifier classification & nested node_modules prefix stripping") {
    if (!isRelativeModuleSpecifier(".") || !isRelativeModuleSpecifier("..") ||
        !isRelativeModuleSpecifier("./a") || !isRelativeModuleSpecifier("../b/c"))
    {
        return err.RelativeClassifyFailed;
    }
    if (isRelativeModuleSpecifier(".hidden") || isRelativeModuleSpecifier("..foo")) {
        return err.NonRelativeDotClassifiedAsRelative;
    }

    if (!isBarePackageSpecifier("react") || !isBarePackageSpecifier("@scope/pkg/x")) {
        return err.BareClassifyFailed;
    }
    // 排除相对路径、绝对路径、subpath import '#'、Windows 盘符与 scheme URI
    if (isBarePackageSpecifier("./local") || isBarePackageSpecifier("/abs/path") ||
        isBarePackageSpecifier("#internal") || isBarePackageSpecifier("C:\\win\\a.js") ||
        isBarePackageSpecifier("node:fs") || isBarePackageSpecifier("cap:io") ||
        isBarePackageSpecifier("https://esm.sh/react"))
    {
        return err.NonBareClassifiedAsBare;
    }

    // 嵌套 node_modules 精确取最深一层
    const deep = stripNodeModulesPrefix("/repo/node_modules/a/node_modules/@scope/b/dist/index.js");
    if (rel := deep) {
        if (!bytesEq(rel, "@scope/b/dist/index.js")) return err.StripDeepMismatch;
        // 与 PackageSpecifier.parse 组合形成完整闭环
        const pkg = PackageSpecifier.parse(rel)?;
        if (!bytesEq(pkg.full_name, "@scope/b") || !bytesEq(pkg.subpath, "dist/index.js")) {
            return err.StripThenParseMismatch;
        }
    } else {
        return err.StripExpectedSome;
    }

    if (stripNodeModulesPrefix("/repo/src/my_node_modules_helper/a.ts") != null) {
        return err.FalsePositiveNodeModulesMatch;
    }
}

test("resolver_path_name: malformed specifiers & BufferTooSmall boundary guards") {
    // 1. 畸形包名拒绝
    const bad_specs: [7]([]const u8) = ["", "@", "@scope", "@scope/", "@/pkg", "./rel", "node:path"];
    let i: usize = 0;
    while (i < bad_specs.len) {
        let caught = false;
        * = PackageSpecifier.parse(bad_specs[i]) catch {
            caught = true;
        };
        if (!caught) return err.ExpectedInvalidSpecifier;
        i += 1;
    }

    // 2. formatValidIdentifier 输出缓冲过小安全返回 BufferTooSmall
    const p = ResolverPathName.init("/dir/very_long_module_name.ts");
    let tiny: [4]u8 = [0; 4];
    let caught_small = false;
    * = p.formatValidIdentifier(tiny[0..]) catch {
        caught_small = true;
    };
    if (!caught_small) return err.ExpectedBufferTooSmall;
}
