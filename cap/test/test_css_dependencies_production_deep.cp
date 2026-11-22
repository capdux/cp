import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CssDependencyOptions,
    CssDependencyLocation,
    CssSourceRange,
    CssDependencyKind,
    CssDependency,
    formatDependencyPlaceholder,
    CssDependencyIter,
} from "cap:css";

fn main() {
    test("css_dependencies: @import string & url() with supports and media", () => {
        const css_src =
            "@import \"./reset.css\";\n" ++
            "@IMPORT url('./grid.css') supports(display: grid) screen and (min-width: 768px);\n";
        let it = CssDependencyIter.init(css_src, "src/app.css", CssDependencyOptions{});

        const d1 = it.next().?;
        expect_eq(d1.isImport(), true);
        expect_eq(d1.isUrl(), false);
        expect_eq(std.mem.eql(u8, d1.url, "./reset.css"), true);
        expect_eq(d1.supports == null, true);
        expect_eq(d1.media == null, true);
        expect_eq(d1.loc.start.line, 1 as u32);
        expect_eq(d1.placeholder().len, 16 as usize);

        const d2 = it.next().?;
        expect_eq(d2.kind == CssDependencyKind.import_rule, true);
        expect_eq(std.mem.eql(u8, d2.url, "./grid.css"), true);
        expect_eq(d2.supports != null, true);
        expect_eq(std.mem.eql(u8, d2.supports.?, "display: grid"), true);
        expect_eq(d2.media != null, true);
        expect_eq(std.mem.eql(u8, d2.media.?, "screen and (min-width: 768px)"), true);
        expect_eq(d2.loc.start.line, 2 as u32);

        expect_eq(it.next() == null, true);
    });

    test("css_dependencies: url() quoted, unquoted & mixed case extraction", () => {
        const css_src =
            ".hero { background: url(\"./hero.png\"); }\n" ++
            "@font-face { src: URL( '../fonts/inter.woff2' ), url(./fallback.ttf); }\n";
        let it = CssDependencyIter.init(css_src, "styles/main.css", CssDependencyOptions{});

        const u1 = it.next().?;
        expect_eq(u1.isUrl(), true);
        expect_eq(std.mem.eql(u8, u1.url, "./hero.png"), true);
        expect_eq(u1.loc.start.line, 1 as u32);

        const u2 = it.next().?;
        expect_eq(std.mem.eql(u8, u2.url, "../fonts/inter.woff2"), true);
        expect_eq(u2.loc.start.line, 2 as u32);

        const u3 = it.next().?;
        expect_eq(std.mem.eql(u8, u3.url, "./fallback.ttf"), true);
        expect_eq(u3.loc.start.line, 2 as u32);

        expect_eq(it.next() == null, true);
    });

    test("css_dependencies: ignores comments, string literals, fragment #ids & data: URIs", () => {
        const css_src =
            "/* @import \"commented.css\"; url(commented.png) */\n" ++
            ".a { content: \"url(inside-string.png)\"; clip-path: url(#mask-1); }\n" ++
            ".b { background: url(data:image/png;base64,AAAA), url(./real.svg); }\n" ++
            ".c { my-url(not-a-url.png); }\n";
        let it = CssDependencyIter.init(css_src, "a.css", CssDependencyOptions{});

        const only = it.next().?;
        expect_eq(std.mem.eql(u8, only.url, "./real.svg"), true);
        expect_eq(only.loc.start.line, 3 as u32);
        expect_eq(it.next() == null, true);

        // When ignore_data_urls = false, data: URI is yielded
        let it2 = CssDependencyIter.init(css_src, "a.css", CssDependencyOptions{ ignore_data_urls: false });
        const d_data = it2.next().?;
        expect_eq(std.mem.eql(u8, d_data.url, "data:image/png;base64,AAAA"), true);
        const d_real = it2.next().?;
        expect_eq(std.mem.eql(u8, d_real.url, "./real.svg"), true);
        expect_eq(it2.next() == null, true);
    });

    test("css_dependencies: ignore_external_urls filters http/https/protocol-relative", () => {
        const css_src =
            "@import \"https://cdn.example.com/a.css\";\n" ++
            ".x { background: url(http://cdn.example.com/b.png), url(//cdn.example.com/c.png), url(/local.png); }\n";
        let it = CssDependencyIter.init(css_src, "ext.css", CssDependencyOptions{ ignore_external_urls: true });

        const local = it.next().?;
        expect_eq(std.mem.eql(u8, local.url, "/local.png"), true);
        expect_eq(it.next() == null, true);
    });

    test("css_dependencies: deterministic placeholder hash & collision isolation", () => {
        let h1: [16]u8 = [0; 16];
        let h2: [16]u8 = [0; 16];
        let h3: [16]u8 = [0; 16];
        const s1 = formatDependencyPlaceholder("src/a.css", "./img.png", &h1);
        const s2 = formatDependencyPlaceholder("src/a.css", "./img.png", &h2);
        const s3 = formatDependencyPlaceholder("src/b.css", "./img.png", &h3);

        expect_eq(s1.len, 16 as usize);
        expect_eq(std.mem.eql(u8, s1, s2), true);
        expect_eq(std.mem.eql(u8, s1, s3), false);

        const loc = CssDependencyLocation.fromZeroBased(4, 2);
        expect_eq(loc.eql(CssDependencyLocation{ line: 5, column: 3 }), true);
        const rng = CssSourceRange.init("src/a.css", loc, 4, 0);
        expect_eq(rng.start.column, 7 as u32);
        expect_eq(rng.end.column, 7 as u32);
    });

    test("css_dependencies: malformed / unterminated inputs fail gracefully without panic", () => {
        const bad = "/* unterminated comment url(a.png)\n";
        let it1 = CssDependencyIter.init(bad, "bad.css", CssDependencyOptions{});
        expect_eq(it1.next() == null, true);

        const bad2 = "@import \"unterminated\n.a { background: url(missing-close; }";
        let it2 = CssDependencyIter.init(bad2, "bad2.css", CssDependencyOptions{});
        expect_eq(it2.next() == null, true);
    });
}
