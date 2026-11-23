import { test, expect } from "cap:test";
import { SourceMapIndex } from "cap:bundler";

test("sourcemap_index: decode v3 VLQ mappings and binary-search lookup", () => {
    // Line 0:
    //   AAAA -> gen(0,0) -> src=0, orig(0,0)
    //   IAAI -> gen(0,4) -> src=0, orig(0,4)
    //   KAAKC -> gen(0,9) -> src=0, orig(0,9), name=1 (C=1)
    // Line 1:
    //   AACA -> gen(1,0) -> src=0, orig(1,9)
    //   EADJ -> gen(1,2) -> src=0, orig(0,5) (D=-1, J=-4)
    const idx = SourceMapIndex.parseMappings("AAAA,IAAI,KAAKC;AACA,EADJ").?;
    expect(idx.len == 5);

    // Exact match at (0, 0)
    const m0 = idx.find(0, 0).?;
    expect(m0.orig_line == 0 && m0.orig_col == 0 && m0.name_idx == -1);

    // Greatest column <= 6 on line 0 -> (0, 4)
    const m1 = idx.find(0, 6).?;
    expect(m1.gen_col == 4 && m1.orig_line == 0 && m1.orig_col == 4);

    // 5-field segment with name index
    const m2 = idx.find(0, 12).?;
    expect(m2.gen_col == 9 && m2.orig_col == 9 && m2.name_idx == 1);

    // Line 1 negative delta segment
    const m3 = idx.find(1, 2).?;
    expect(m3.orig_line == 0 && m3.orig_col == 5);

    // Unmapped line returns null
    expect(idx.find(2, 0) == null);
    // Invalid VLQ rejected
    expect(SourceMapIndex.parseMappings("@@@") == null);
});
