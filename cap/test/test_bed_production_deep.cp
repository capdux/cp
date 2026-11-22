// Source: cp/cap/test/test_bed_production_deep.cp (Production deep tests for cap:bed)

import { test, expect_eq } from "cap:test";
import {
    BedStrand,
    BedRecord,
    BedStats,
    BedIter,
    parseBedLine,
    summarizeBed,
} from "cap:science";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. BED3, BED6, and BED12 full column parsing (Tab, space, CRLF)", () => {
    // BED3 (space-delimited + CRLF)
    const b3 = parseBedLine("chr1 100 250\r\n") ?? unreachable;
    expect_eq(bytesEq(b3.chrom, "chr1"), true);
    expect_eq(b3.start, 100);
    expect_eq(b3.end, 250);
    expect_eq(b3.length(), 150);
    expect_eq(b3.field_count, 3);
    expect_eq(b3.strand, BedStrand.unknown);

    // BED6 (Tab-delimited)
    const b6 = parseBedLine("chr2\t1000\t2000\tGENE_A\t850\t+") ?? unreachable;
    expect_eq(bytesEq(b6.chrom, "chr2"), true);
    expect_eq(bytesEq(b6.name, "GENE_A"), true);
    expect_eq(b6.score, 850);
    expect_eq(b6.strand, BedStrand.plus);
    expect_eq(b6.field_count, 6);

    // BED12 (full 12 columns)
    const b12 = parseBedLine("chr7\t127471196\t127472363\tPos1\t0\t-\t127471196\t127472363\t255,0,0\t2\t567,488,\t0,679,") ?? unreachable;
    expect_eq(b12.field_count, 12);
    expect_eq(b12.strand, BedStrand.minus);
    expect_eq(b12.thick_start, 127471196);
    expect_eq(b12.thick_end, 127472363);
    expect_eq(bytesEq(b12.item_rgb, "255,0,0"), true);
    expect_eq(b12.block_count, 2);
    expect_eq(bytesEq(b12.block_sizes, "567,488,"), true);
    expect_eq(bytesEq(b12.block_starts, "0,679,"), true);
});

test("2. BedIter skips UCSC browser/track headers, '#' comments, and blank lines", () => {
    const bed_file =
        "# UCSC BED annotation track\r\n" +
        "browser position chr7:127471196-127495720\r\n" +
        "track name=\"ItemRGBDemo\" description=\"Item RGB demonstration\" itemRgb=\"On\"\r\n" +
        "\r\n" +
        "chr7\t100\t200\tfeat1\t500\t+\r\n" +
        "# inline comment\r\n" +
        "chr7\t150\t300\tfeat2\t600\t-\r\n";

    let it = BedIter.init(bed_file);
    const r0 = it.next() ?? unreachable;
    expect_eq(bytesEq(r0.name, "feat1"), true);
    expect_eq(r0.strand, BedStrand.plus);

    const r1 = it.next() ?? unreachable;
    expect_eq(bytesEq(r1.name, "feat2"), true);
    expect_eq(r1.strand, BedStrand.minus);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("3. Half-open [start, end) overlaps, overlapLen, containsPos, and cross-chrom isolation", () => {
    const a = parseBedLine("chr1\t100\t200") ?? unreachable;
    const b = parseBedLine("chr1\t150\t250") ?? unreachable;
    const touching = parseBedLine("chr1\t200\t300") ?? unreachable;
    const other_chr = parseBedLine("chr2\t120\t180") ?? unreachable;
    const inner = parseBedLine("chr1\t120\t140") ?? unreachable;

    // Partial overlap: [100, 200) & [150, 250) -> [150, 200) = 50 bp
    expect_eq(a.overlaps(b), true);
    expect_eq(a.overlapLen(b), 50);
    expect_eq(b.overlapLen(a), 50);

    // Contained interval: [120, 140) inside [100, 200) -> 20 bp
    expect_eq(a.overlaps(inner), true);
    expect_eq(a.overlapLen(inner), 20);

    // Touching half-open intervals [100, 200) & [200, 300) do NOT overlap
    expect_eq(a.overlaps(touching), false);
    expect_eq(a.overlapLen(touching), 0);

    // Different chromosomes NEVER overlap
    expect_eq(a.overlaps(other_chr), false);
    expect_eq(a.overlapLen(other_chr), 0);

    // 0-based half-open containsPos: 100 inclusive, 200 exclusive
    expect_eq(a.containsPos(99), false);
    expect_eq(a.containsPos(100), true);
    expect_eq(a.containsPos(199), true);
    expect_eq(a.containsPos(200), false);
});

test("4. summarizeBed single-pass dataset statistics", () => {
    const bed_file =
        "track name=demo\n" +
        "chr1\t0\t100\n" +                      // len=100, fields=3, unknown
        "chr1\t200\t500\tG1\t100\t+\n" +        // len=300, fields=6, plus
        "chr2\t1000\t1050\tG2\t200\t-\n" +      // len=50,  fields=6, minus
        "chr2\t2000\t2400\tG3\t300\t+\t2000\t2400\t0\t1\t400\t0\n"; // len=400, fields=12, plus

    const stats = summarizeBed(bed_file);
    expect_eq(stats.record_count, 4);
    expect_eq(stats.total_span_bp, 850);
    expect_eq(stats.min_len, 50);
    expect_eq(stats.max_len, 400);
    expect_eq(stats.plus_count, 2);
    expect_eq(stats.minus_count, 1);
    expect_eq(stats.min_fields, 3);
    expect_eq(stats.max_fields, 12);
});

test("5. Fault tolerance: start > end, < 3 columns, bad numbers, bad strand, and BedIter.had_error", () => {
    // start > end
    expect_eq(parseBedLine("chr1\t300\t200") == null, true);

    // Fewer than 3 columns
    expect_eq(parseBedLine("chr1\t100") == null, true);
    expect_eq(parseBedLine("") == null, true);

    // Non-numeric coordinates or score
    expect_eq(parseBedLine("chr1\tabc\t200") == null, true);
    expect_eq(parseBedLine("chr1\t100\t200\tG1\tbadscore\t+") == null, true);

    // Invalid strand character
    expect_eq(parseBedLine("chr1\t100\t200\tG1\t0\t*") == null, true);

    // thick_start > thick_end
    expect_eq(parseBedLine("chr1\t100\t200\tG1\t0\t+\t190\t150") == null, true);

    // BedIter stops and sets had_error=true on malformed line
    const bad_bed = "chr1\t10\t20\nchr1\t50\t10\nchr1\t30\t40\n";
    let it = BedIter.init(bad_bed);
    * = it.next() ?? unreachable;
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, true);
});
