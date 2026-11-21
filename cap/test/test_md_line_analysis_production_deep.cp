import { test, expect } from "cap:test";
import {
    TableAlign,
    isSetextUnderline,
    isHrLine,
    isAtxHeaderLine,
    isOpeningCodeFence,
    isClosingCodeFence,
    isHtmlBlockStartCondition,
    isHtmlBlockEndCondition,
    isTableUnderline,
} from "cap:md_line_analysis";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("md_line_analysis: Setext, HR, and ATX headers", () => {
    const s1 = isSetextUnderline("====   \n", 0);
    expect(s1.is_setext && s1.level == 1);
    const s2 = isSetextUnderline("---\n", 0);
    expect(s2.is_setext && s2.level == 2);
    expect(!isSetextUnderline("=== x\n", 0).is_setext);

    expect(isHrLine("* * * *\n", 0));
    expect(isHrLine("___\n", 0));
    expect(!isHrLine("--\n", 0));
    expect(!isHrLine("---a\n", 0));

    const a1 = isAtxHeaderLine("### Hello World ###  \n", 0, false);
    expect(a1.is_atx && a1.level == 3);
    expect(strEq("### Hello World ###  \n"[a1.content_beg..a1.content_end], "Hello World"));

    // No space after '#' rejected in strict mode, accepted in permissive mode
    expect(!isAtxHeaderLine("##NoSpace\n", 0, false).is_atx);
    expect(isAtxHeaderLine("##NoSpace\n", 0, true).is_atx);
    expect(!isAtxHeaderLine("####### TooMany\n", 0, false).is_atx);
});

test("md_line_analysis: Fenced code blocks", () => {
    const f1 = isOpeningCodeFence("````cp lang=cp\n", 0);
    expect(f1.is_fence && f1.fence_char == '`' && f1.fence_count == 4);
    expect(strEq(f1.info, "cp lang=cp"));

    // Backtick info string cannot contain backtick
    expect(!isOpeningCodeFence("```foo`bar\n", 0).is_fence);
    // Tilde info string CAN contain backtick
    const f2 = isOpeningCodeFence("~~~foo`bar\n", 0);
    expect(f2.is_fence && f2.fence_char == '~' && f2.fence_count == 3);

    expect(!isClosingCodeFence("```\n", 0, '`', 4));
    expect(isClosingCodeFence("````  \n", 0, '`', 4));
    expect(!isClosingCodeFence("```` trailing\n", 0, '`', 4));
});

test("md_line_analysis: HTML block types 1-7 and GFM table underline", () => {
    expect(isHtmlBlockStartCondition("<SCRIPT type=\"text/javascript\">\n", 0) == 1);
    expect(isHtmlBlockEndCondition("  </script>\n", 0, 1));
    expect(isHtmlBlockStartCondition("<!-- comment\n", 0) == 2);
    expect(isHtmlBlockEndCondition("end -->\n", 0, 2));
    expect(isHtmlBlockStartCondition("<?xml version=\"1.0\"?>\n", 0) == 3);
    expect(isHtmlBlockStartCondition("<!DOCTYPE html>\n", 0) == 4);
    expect(isHtmlBlockStartCondition("<![CDATA[raw]]>\n", 0) == 5);
    expect(isHtmlBlockStartCondition("<div class=\"box\">\n", 0) == 6);
    expect(isHtmlBlockStartCondition("<custom-el data-x=\"1\">\n", 0) == 7);
    expect(isHtmlBlockStartCondition("<span>inline</span> text\n", 0) == 0);

    const t = isTableUnderline("| :--- | :---: | ---: | --- |\n", 0);
    expect(t.is_underline && t.col_count == 4);
    expect(t.alignments[0] == TableAlign.left);
    expect(t.alignments[1] == TableAlign.center);
    expect(t.alignments[2] == TableAlign.right);
    expect(t.alignments[3] == TableAlign.none);

    expect(!isTableUnderline("---\n", 0).is_underline);
});
