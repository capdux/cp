import {
    matchSectionPattern,
    IndentStyle,
    EndOfLine,
    Charset,
    resolveEditorConfig,
} from "cap:editorconfig";
import { test, expect } from "cap:test";

const SAMPLE_EC =
    "root = true\n" ++
    "\n" ++
    "[*]\n" ++
    "charset = utf-8\n" ++
    "end_of_line = lf\n" ++
    "indent_style = space\n" ++
    "indent_size = 4\n" ++
    "insert_final_newline = true\n" ++
    "trim_trailing_whitespace = true\n" ++
    "\n" ++
    "[*.{js,ts,json}]\n" ++
    "indent_size = 2\n" ++
    "max_line_length = 100\n" ++
    "\n" ++
    "[Makefile]\n" ++
    "indent_style = tab\n" ++
    "indent_size = tab\n" ++
    "tab_width = 8\n" ++
    "\n" ++
    "[docs/**.md]\n" ++
    "trim_trailing_whitespace = false\n" ++
    "max_line_length = off\n";

test("matchSectionPattern supports braces, wildcards, and anchored paths", () => {
    expect(matchSectionPattern("*.{js,ts,json}", "src/app.ts"));
    expect(!matchSectionPattern("*.{js,ts,json}", "src/app.cp"));
    expect(matchSectionPattern("docs/**.md", "docs/api/readme.md"));
    expect(!matchSectionPattern("docs/**.md", "src/readme.md"));
    expect(matchSectionPattern("Makefile", "sub/Makefile"));
});

test("resolveEditorConfig cascades sections and handles overrides/unset", () => {
    const cp_props = resolveEditorConfig(SAMPLE_EC, "lib/http/server.cp");
    expect(cp_props.is_root);
    expect(cp_props.charset == Charset.utf8);
    expect(cp_props.end_of_line == EndOfLine.lf);
    expect(cp_props.indent_style == IndentStyle.space);
    expect(cp_props.indent_size.? == 4);
    expect(cp_props.effectiveTabWidth().? == 4);
    expect(cp_props.trim_trailing_whitespace.? == true);

    const ts_props = resolveEditorConfig(SAMPLE_EC, "web/index.ts");
    expect(ts_props.indent_size.? == 2);
    expect(ts_props.max_line_length.? == 100);

    const mk_props = resolveEditorConfig(SAMPLE_EC, "Makefile");
    expect(mk_props.indent_style == IndentStyle.tab);
    expect(mk_props.indent_size_tab);
    expect(mk_props.effectiveTabWidth().? == 8);

    const md_props = resolveEditorConfig(SAMPLE_EC, "docs/guide/intro.md");
    expect(md_props.trim_trailing_whitespace.? == false);
    expect(md_props.max_line_length == null);
});
