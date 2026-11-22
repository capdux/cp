// Source: cp/cap/test/test_fastq_production_deep.cp (Production deep tests for cap:fastq)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PHRED_OFFSET_33,
    PHRED_OFFSET_64,
    FastqRecord,
    FastqStats,
    FastqIter,
    isFastq,
    summarizeFastq,
} from "cap:science";

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff < 0.0001;
}

// Phred+33 ASCII reference:
// 'I' (73) -> Q40, '?' (63) -> Q30, '5' (53) -> Q20, '#' (35) -> Q2, '!' (33) -> Q0
const SAMPLE_FASTQ_33: []const u8 =
    "@SEQ_ID_1 lane=1 tile=1101\r\n" ++
    "GATTTGGGGTTCAAAGCAGT\r\n" ++
    "+SEQ_ID_1\r\n" ++
    "IIIIIIIIII??????????\r\n" ++
    "\r\n" ++
    "@SEQ_ID_2\n" ++
    "ACGTN\n" ++
    "+\n" ++
    "I?5#!\n";

test("1. isFastq & FastqIter parse Illumina Phred+33 reads with CRLF, descriptions, and +id header", () => {
    expect_eq(isFastq(SAMPLE_FASTQ_33), true);

    let it = FastqIter.init(SAMPLE_FASTQ_33);
    const r1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r1.id, "SEQ_ID_1"), true);
    expect_eq(std.mem.eql(u8, r1.description, "lane=1 tile=1101"), true);
    expect_eq(r1.seq.len, 20);
    expect_eq(r1.qual.len, 20);
    // 10 * Q40 + 10 * Q30 => mean = 35.0, min = 30
    expect_eq(r1.phredAt(0, PHRED_OFFSET_33) ?? 0, 40);
    expect_eq(r1.phredAt(19, PHRED_OFFSET_33) ?? 0, 30);
    expect_eq(r1.phredAt(20, PHRED_OFFSET_33) == null, true);
    expect_eq(approxEq(r1.meanQuality(PHRED_OFFSET_33), 35.0), true);
    expect_eq(r1.minQuality(PHRED_OFFSET_33), 30);
    // GATTTGGGGTTCAAAGCAGT => G:7, C:2 => 9 / 20 = 0.45
    expect_eq(approxEq(r1.gcContent(), 0.45), true);

    const r2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r2.id, "SEQ_ID_2"), true);
    expect_eq(std.mem.eql(u8, r2.description, ""), true);
    expect_eq(r2.seq.len, 5);
    // ACGTN => G:1, C:1, ATGC:4 (N excluded from ATUGC denominator) => 0.5
    expect_eq(approxEq(r2.gcContent(), 0.5), true);
    expect_eq(r2.minQuality(PHRED_OFFSET_33), 0);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("2. Phred+64 legacy Illumina quality decoding and 3-prime trimTailEnd zero-copy trimming", () => {
    // Phred+64: 'h' (104) -> Q40, '^' (94) -> Q30, 'T' (84) -> Q20, 'B' (66) -> Q2
    const fq64 =
        "@OLD_ILLUMINA\n" ++
        "ACGTACGT\n" ++
        "+\n" ++
        "hh^TTBBB\n";
    let it = FastqIter.init(fq64);
    const rec = it.next() ?? unreachable;

    expect_eq(rec.phredAt(0, PHRED_OFFSET_64) ?? 0, 40);
    expect_eq(rec.phredAt(2, PHRED_OFFSET_64) ?? 0, 30);
    expect_eq(rec.phredAt(4, PHRED_OFFSET_64) ?? 0, 20);
    expect_eq(rec.phredAt(7, PHRED_OFFSET_64) ?? 0, 2);

    // Trim tail < Q20 -> drops trailing 3 'B's, keeps first 5 bases ("ACGTA")
    const end20 = rec.trimTailEnd(20, PHRED_OFFSET_64);
    expect_eq(end20, 5);
    expect_eq(std.mem.eql(u8, rec.seq[0..end20], "ACGTA"), true);

    // Trim tail < Q30 -> keeps first 3 bases ("ACG")
    expect_eq(rec.trimTailEnd(30, PHRED_OFFSET_64), 3);

    // Underflow safety when byte < offset
    const low_rec = FastqRecord { id: "x", description: "", seq: "A", qual: "!" };
    expect_eq(low_rec.phredAt(0, PHRED_OFFSET_64) == null, true);
});

test("3. summarizeFastq computes Q20/Q30 rates, GC/N counts, and read length bounds", () => {
    const st = summarizeFastq(SAMPLE_FASTQ_33, PHRED_OFFSET_33);
    expect_eq(st.read_count, 2);
    expect_eq(st.total_bases, 25);
    expect_eq(st.min_len, 5);
    expect_eq(st.max_len, 20);
    // Read 1: all 20 >= Q30; Read 2: 'I'(40), '?'(30), '5'(20), '#'(2), '!'(0)
    // => q30_bases = 22, q20_bases = 23
    expect_eq(st.q30_bases, 22);
    expect_eq(st.q20_bases, 23);
    expect_eq(st.gc_bases, 11); // 9 + 2
    expect_eq(st.n_bases, 1);
    expect_eq(approxEq(st.q20Rate(), 23.0 / 25.0), true);
    expect_eq(approxEq(st.q30Rate(), 22.0 / 25.0), true);
    expect_eq(approxEq(st.gcRate(), 11.0 / 25.0), true);
});

test("4. Empty input and all-low-quality read trimming edge cases", () => {
    const empty_st = summarizeFastq("", PHRED_OFFSET_33);
    expect_eq(empty_st.read_count, 0);
    expect_eq(approxEq(empty_st.q20Rate(), 0.0), true);
    expect_eq(approxEq(empty_st.q30Rate(), 0.0), true);
    expect_eq(approxEq(empty_st.gcRate(), 0.0), true);

    const all_low = "@LOW\nACGT\n+\n!!!!\n";
    let it = FastqIter.init(all_low);
    const rec = it.next() ?? unreachable;
    expect_eq(rec.trimTailEnd(10, PHRED_OFFSET_33), 0);
});

test("5. Malformed FASTQ (length mismatch, missing '+', incomplete 4 lines, non-'@' header) fail safely", () => {
    expect_eq(isFastq(""), false);
    expect_eq(isFastq(">FASTA_HEADER\nACGT\n"), false);

    // seq.len != qual.len
    const len_mismatch = "@R1\nACGT\n+\nIII\n";
    expect_eq(isFastq(len_mismatch), false);
    let it1 = FastqIter.init(len_mismatch);
    expect_eq(it1.next() == null, true);
    expect_eq(it1.had_error, true);

    // Missing '+' separator line
    const no_plus = "@R1\nACGT\n-BAD\nIIII\n";
    expect_eq(isFastq(no_plus), false);
    let it2 = FastqIter.init(no_plus);
    expect_eq(it2.next() == null, true);
    expect_eq(it2.had_error, true);

    // Incomplete 3-line truncated record
    const truncated = "@R1\nACGT\n+\n";
    expect_eq(isFastq(truncated), false);
    let it3 = FastqIter.init(truncated);
    expect_eq(it3.next() == null, true);
    expect_eq(it3.had_error, true);
});
