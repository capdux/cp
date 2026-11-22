// Source: cp/cap/test/test_vcf_production_deep.cp (Production deep tests for cap:vcf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    VcfHeader,
    VcfRecord,
    VcfStats,
    isVcf,
    parseVcfHeader,
    VcfRecordIter,
    summarizeVcf,
} from "cap:science";

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff < 0.0001;
}

const SAMPLE_VCF_43: []const u8 =
    "##fileformat=VCFv4.3\r\n" ++
    "##fileDate=20261121\r\n" ++
    "##reference=GRCh38.p14\r\n" ++
    "##INFO=<ID=DP,Number=1,Type=Integer,Description=\"Total Depth\">\r\n" ++
    "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tNA00001\tNA00002\tNA00003\r\n" ++
    "chr1\t14370\trs6054257\tG\tA\t29.5\tPASS\tNS=3;DP=14;AF=0.5;DB;H2\tGT:GQ:DP\t0|0:48:1\t1|0:48:8\t1/1:43:5\r\n" ++
    "chr1\t17330\t.\tT\tA\t3\tq10\tNS=3;DP=11;AF=0.017\tGT:GQ\t0|0:49\t0|1:3\t0/0:41\r\n" ++
    "chr1\t1110696\trs6040355\tC\tT\t67\tPASS\tDP=20;SOMATIC;AF=0.333\tGT\t1|2\t2|1\t2/2\r\n" ++
    "chr1\t1230237\t.\tATG\tA\t.\tPASS\tDP=13;INDEL\tGT\t0|0\t0|1\t0/0\r\n";

test("1. isVcf & parseVcfHeader decode VCF v4.3 version, reference, meta count, and sample count", () => {
    expect_eq(isVcf(SAMPLE_VCF_43), true);
    const hdr = parseVcfHeader(SAMPLE_VCF_43) ?? unreachable;
    expect_eq(std.mem.eql(u8, hdr.version, "4.3"), true);
    expect_eq(std.mem.eql(u8, hdr.reference, "GRCh38.p14"), true);
    expect_eq(hdr.meta_line_count, 4);
    expect_eq(hdr.sample_count, 3);
    expect_eq(hdr.records_offset > 0, true);
});

test("2. VcfRecordIter extracts 8 core fields, missing QUAL '.', and multi-sample FORMAT slice", () => {
    let it = VcfRecordIter.init(SAMPLE_VCF_43);

    const r1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r1.chrom, "chr1"), true);
    expect_eq(r1.pos, 14370);
    expect_eq(std.mem.eql(u8, r1.id, "rs6054257"), true);
    expect_eq(std.mem.eql(u8, r1.ref_allele, "G"), true);
    expect_eq(std.mem.eql(u8, r1.alt_allele, "A"), true);
    expect_eq(r1.has_qual, true);
    expect_eq(approxEq(r1.qual, 29.5), true);
    expect_eq(r1.isPass(), true);
    expect_eq(r1.isSnp(), true);
    expect_eq(r1.isTransition(), true);
    expect_eq(r1.isTransversion(), false);
    expect_eq(std.mem.eql(u8, r1.rest_samples, "GT:GQ:DP\t0|0:48:1\t1|0:48:8\t1/1:43:5"), true);

    const r2 = it.next() ?? unreachable;
    expect_eq(r2.isPass(), false); // FILTER = q10
    expect_eq(r2.isSnp(), true);
    expect_eq(r2.isTransition(), false);
    expect_eq(r2.isTransversion(), true); // T->A

    const r3 = it.next() ?? unreachable;
    expect_eq(r3.isTransition(), true); // C->T

    const r4 = it.next() ?? unreachable;
    expect_eq(r4.has_qual, false); // QUAL = '.'
    expect_eq(r4.isPass(), true);
    expect_eq(r4.isSnp(), false);
    expect_eq(r4.isIndel(), true); // ATG -> A

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("3. findInfo extracts key=value pairs and boolean flags (DB/SOMATIC) without false prefix matches", () => {
    let it = VcfRecordIter.init(SAMPLE_VCF_43);
    const r1 = it.next() ?? unreachable;

    expect_eq(std.mem.eql(u8, r1.findInfo("DP") ?? "", "14"), true);
    expect_eq(std.mem.eql(u8, r1.findInfo("AF") ?? "", "0.5"), true);
    // Boolean flags return empty slice "" (non-null!)
    const db = r1.findInfo("DB") ?? unreachable;
    expect_eq(db.len, 0);
    const h2 = r1.findInfo("H2") ?? unreachable;
    expect_eq(h2.len, 0);
    expect_eq(r1.findInfo("D") == null, true);
    expect_eq(r1.findInfo("MISSING") == null, true);

    * = it.next();
    const r3 = it.next() ?? unreachable;
    const som = r3.findInfo("SOMATIC") ?? unreachable;
    expect_eq(som.len, 0);
});

test("4. summarizeVcf computes record/PASS/SNP/Indel counts and exact Ti/Tv ratio", () => {
    const st = summarizeVcf(SAMPLE_VCF_43);
    expect_eq(st.record_count, 4);
    expect_eq(st.pass_count, 3);
    expect_eq(st.snp_count, 3);
    expect_eq(st.indel_count, 1);
    expect_eq(st.transitions, 2); // G->A, C->T
    expect_eq(st.transversions, 1); // T->A
    expect_eq(approxEq(st.tiTvRatio(), 2.0), true);
});

test("5. Malformed VCF inputs (non-VCF, missing #CHROM, < 8 columns, invalid POS/QUAL) fail safely", () => {
    expect_eq(isVcf(""), false);
    expect_eq(isVcf("chr1\t100\t.\tA\tG\t50\tPASS\t.\n"), false);
    expect_eq(parseVcfHeader("##fileformat=VCFv4.2\n##only_meta=1\n") == null, true);

    // Fewer than 8 columns
    const bad_cols = "##fileformat=VCFv4.2\n#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\nchr1\t100\t.\tA\tG\n";
    let it1 = VcfRecordIter.init(bad_cols);
    expect_eq(it1.next() == null, true);
    expect_eq(it1.had_error, true);

    // Invalid POS (0 or non-numeric)
    const bad_pos = "##fileformat=VCFv4.2\n#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\nchr1\t0\t.\tA\tG\t50\tPASS\t.\n";
    let it2 = VcfRecordIter.init(bad_pos);
    expect_eq(it2.next() == null, true);
    expect_eq(it2.had_error, true);

    // Invalid QUAL
    const bad_qual = "##fileformat=VCFv4.2\n#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\nchr1\t10\t.\tA\tG\tNaN\tPASS\t.\n";
    let it3 = VcfRecordIter.init(bad_qual);
    expect_eq(it3.next() == null, true);
    expect_eq(it3.had_error, true);
});
