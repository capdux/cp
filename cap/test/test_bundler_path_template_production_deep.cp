import { test, expect } from "cap:test";
import {
    PathTemplateField,
    PathSeparatorStyle,
    formatTruncatedHashHex,
    PathTemplatePlaceholder,
    PathTemplate,
    formatBundlerNaming,
} from "cap:bundler";

fn eqBytes(a: []const u8, b: []const u8): bool {
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

test("bundler path_template: 6 official presets and static needs() detection") {
    const f = PathTemplate.file();
    expect(f.needs(.dir));
    expect(f.needs(.name));
    expect(f.needs(.ext));
    expect(!f.needs(.hash));
    expect(!f.needs(.target));
    expect((f.validate()?) == 3);

    const ft = PathTemplate.fileWithTarget();
    expect(ft.needs(.target));
    expect((ft.validate()?) == 4);

    const c = PathTemplate.chunk();
    expect(!c.needs(.dir));
    expect(c.needs(.hash));
    expect(c.needs(.ext));
    expect((c.validate()?) == 2);

    const ct = PathTemplate.chunkWithTarget();
    expect(ct.needs(.dir) && ct.needs(.target) && ct.needs(.hash) && ct.needs(.ext));
    expect((ct.validate()?) == 4);

    const a = PathTemplate.asset();
    expect(a.needs(.name) && a.needs(.hash) && a.needs(.ext));
    expect((a.validate()?) == 3);

    const at = PathTemplate.assetWithTarget();
    expect((at.validate()?) == 5);
}

test("bundler path_template: empty dir fallback, hash formatting and deterministic output") {
    let hbuf: [8]u8 = [0; 8];
    const hn = formatTruncatedHashHex(0x0000_0000_dead_beef, hbuf[0..])?;
    expect(hn == 8);
    expect(eqBytes(hbuf[0..hn], "deadbeef"));

    // 高低 32 位异或折叠
    const hn2 = formatTruncatedHashHex(0x1234_5678_9abc_def0, hbuf[0..])?;
    expect(hn2 == 8);
    expect(eqBytes(hbuf[0..hn2], "88888888"));

    let out: [128]u8 = [0; 128];

    // 空 dir 自动回退 "."
    const vals_empty_dir = PathTemplatePlaceholder{
        dir: "",
        name: "index",
        ext: "js",
        hash: 0x0000_0000_dead_beef,
        target: "browser",
    };
    const n1 = PathTemplate.file().withValues(vals_empty_dir).format(out[0..])?;
    expect(eqBytes(out[0..n1], "./index.js"));

    // chunk 默认预设
    const n2 = PathTemplate.chunk().withValues(vals_empty_dir).format(out[0..])?;
    expect(eqBytes(out[0..n2], "./chunk-deadbeef.js"));

    // asset 默认预设
    const n3 = PathTemplate.asset().withValues(vals_empty_dir).format(out[0..])?;
    expect(eqBytes(out[0..n3], "./index-deadbeef.js"));
}

test("bundler path_template: POSIX vs Windows separator normalization and helper") {
    const vals = PathTemplatePlaceholder{
        dir: "pages/auth",
        name: "login",
        ext: "js",
        hash: 0x0000_0000_cafe_babe,
        target: "node",
    };
    let out: [128]u8 = [0; 128];

    const n_posix = formatBundlerNaming("files/[dir]/[target]/[name]-[hash].[ext]", vals, .posix, out[0..])?;
    expect(eqBytes(out[0..n_posix], "files/pages/auth/node/login-cafebabe.js"));

    const n_win = formatBundlerNaming("files/[dir]/[target]/[name]-[hash].[ext]", vals, .windows, out[0..])?;
    expect(eqBytes(out[0..n_win], "files\\pages\\auth\\node\\login-cafebabe.js"));
}

test("bundler path_template: lenient upstream fallback vs strict validation errors") {
    let out: [128]u8 = [0; 128];
    const vals = PathTemplatePlaceholder{
        dir: "dist",
        name: "app",
        ext: "js",
        hash: null,
        target: "browser",
    };

    // 宽松模式：未知占位符剥离括号保留字面、缺失 hash 输出空串、未闭合 '[' 保留字面
    const lenient_tpl = PathTemplate.fromString("[dir]/[unknown]/[name]-[hash].[ext]").withValues(vals);
    const nl = lenient_tpl.format(out[0..])?;
    expect(eqBytes(out[0..nl], "dist/unknown/app-.js"));

    const unclosed_lenient = PathTemplate.fromString("[name]-[broken").withValues(vals);
    const nu = unclosed_lenient.format(out[0..])?;
    expect(eqBytes(out[0..nu], "app-[broken"));

    // 严格模式拦截所有非法形态
    let rejected: usize = 0;
    * = PathTemplate.fromString("").validate() catch { rejected += 1; 0; };
    * = PathTemplate.fromString("[dir]/[name").validate() catch { rejected += 1; 0; };
    * = PathTemplate.fromString("[dir]/name].js").validate() catch { rejected += 1; 0; };
    * = PathTemplate.fromString("[dir]/[unknown].[ext]").validate() catch { rejected += 1; 0; };
    // 严格模式缺失 hash 值
    * = PathTemplate.asset().withValues(vals).formatWithStyle(.posix, true, out[0..]) catch { rejected += 1; 0; };
    expect(rejected == 5);
}

test("bundler path_template: path traversal security guards") {
    let out: [128]u8 = [0; 128];
    let rejected: usize = 0;

    // 模板自身包含 '..'
    * = PathTemplate.fromString("../secret/[name].[ext]").validate() catch { rejected += 1; 0; };
    * = PathTemplate.fromString("a/../b/[name].[ext]").validate() catch { rejected += 1; 0; };

    // 占位符注入 '..'（展开后拦截）
    const evil_vals = PathTemplatePlaceholder{
        dir: "../../etc",
        name: "passwd",
        ext: "js",
        hash: null,
        target: "",
    };
    * = PathTemplate.file().withValues(evil_vals).formatWithStyle(.posix, true, out[0..]) catch { rejected += 1; 0; };
    expect(rejected == 3);

    // 合法含双点文件名（非独立 ".." 段）应放行
    const ok_vals = PathTemplatePlaceholder{
        dir: "assets",
        name: "foo..bar",
        ext: "css",
        hash: null,
        target: "",
    };
    const n_ok = PathTemplate.file().withValues(ok_vals).formatWithStyle(.posix, true, out[0..])?;
    expect(eqBytes(out[0..n_ok], "assets/foo..bar.css"));
}

test("bundler path_template: placeholder equality, field tokens and buffer guard") {
    const p1 = PathTemplatePlaceholder{
        dir: "src",
        name: "main",
        ext: "js",
        hash: 42,
        target: "bun",
    };
    const p2 = PathTemplatePlaceholder{
        dir: "src",
        name: "main",
        ext: "js",
        hash: 42,
        target: "bun",
    };
    const p3 = PathTemplatePlaceholder{
        dir: "src",
        name: "main",
        ext: "js",
        hash: 99,
        target: "bun",
    };
    expect(p1.eql(p2));
    expect(!p1.eql(p3));
    expect(eqBytes(PathTemplateField.hash.bracketToken(), "[hash]"));

    // 缓冲区不足防护
    let tiny: [5]u8 = [0; 5];
    let buf_err: usize = 0;
    * = formatTruncatedHashHex(0x1234, tiny[0..]) catch { buf_err += 1; 0; };
    * = PathTemplate.file().withValues(p1).format(tiny[0..]) catch { buf_err += 1; 0; };
    expect(buf_err == 2);
}
