// Source: cp/cap/test/test_fasta_production_deep.cp (Production deep tests for cap:fasta)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    FastaRecord,
    FastaStats,
    FastaIter,
    isFasta,
    dnaComplement,
    reverseComplementDna,
    summarizeFasta,
} from "cap:fasta";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. Multi-record multi-line wrapped DNA and protein FASTA iteration & copyCleanSeq", () => {
    const fasta =
        ">NM_000518.5 Homo sapiens hemoglobin subunit beta (HBB), mRNA\n" +
        "ACATTTGCTT\n" +
        "CTGACACAAC\n" +
        ">NP_000509.1 hemoglobin subunit beta [Homo sapiens]\n" +
        "MVHLTPEEKS\n" +
        "AVTALWGKVN\n";

    expect_eq(isFasta(fasta), true);
    let it = FastaIter.init(fasta);

    const r0 = it.next() ?? unreachable;
    expect_eq(bytesEq(r0.id, "NM_000518.5"), true);
    expect_eq(bytesEq(r0.description, "Homo sapiens hemoglobin subunit beta (HBB), mRNA"), true);
    expect_eq(r0.seqLen(), 20);

    let clean: [32]u8 = std.mem.zeroes([32]u8);
    const n0 = r0.copyCleanSeq(clean[0..]) ?? unreachable;
    expect_eq(bytesEq(clean[0..n0], "ACATTTGCTTCTGACACAAC"), true);

    // Overflow check on copyCleanSeq
    let tiny: [10]u8 = std.mem.zeroes([10]u8);
    expect_eq(r0.copyCleanSeq(tiny[0..]) == null, true);

    const r1 = it.next() ?? unreachable;
    expect_eq(bytesEq(r1.id, "NP_000509.1"), true);
    expect_eq(bytesEq(r1.description, "hemoglobin subunit beta [Homo sapiens]"), true);
    expect_eq(r1.seqLen(), 20);

    const n1 = r1.copyCleanSeq(clean[0..]) ?? unreachable;
    expect_eq(bytesEq(clean[0..n1], "MVHLTPEEKSAVTALWGKVN"), true);
    expect_eq(it.next() == null, true);
});

test("2. gcContent() (case-insensitive + RNA U) and summarizeFasta statistics", () => {
    const fasta =
        ">seq1 60% GC\n" +
        "GGCCggccAAttuuNN\n" + // 8 GC, 6 AT/U, 2 N -> GC/(GC+AT) = 8/14 = 0.5714...
        ">seq2 50% GC\n" +
        "ACGT\n";              // 2 GC, 2 AT -> 0.5

    let it = FastaIter.init(fasta);
    const r0 = it.next() ?? unreachable;
    const gc0 = r0.gcContent();
    expect_eq(gc0 > 0.571 && gc0 < 0.572, true);

    const r1 = it.next() ?? unreachable;
    const gc1 = r1.gcContent();
    expect_eq(gc1 > 0.499 && gc1 < 0.501, true);

    const stats = summarizeFasta(fasta);
    expect_eq(stats.record_count, 2);
    expect_eq(stats.total_bases, 20);
    expect_eq(stats.min_len, 4);
    expect_eq(stats.max_len, 16);
    expect_eq(stats.gc_bases, 10);
    expect_eq(stats.at_bases, 8);
    expect_eq(stats.n_bases, 2);
    // overallGc = 10 / 18 = 0.5555...
    const ogc = stats.overallGc();
    expect_eq(ogc > 0.555 && ogc < 0.556, true);
});

test("3. dnaComplement and reverseComplementDna with mixed case and full IUPAC codes", () => {
    expect_eq(dnaComplement('A'), 'T');
    expect_eq(dnaComplement('t'), 'a');
    expect_eq(dnaComplement('C'), 'G');
    expect_eq(dnaComplement('g'), 'c');
    expect_eq(dnaComplement('R'), 'Y');
    expect_eq(dnaComplement('y'), 'r');
    expect_eq(dnaComplement('S'), 'S');
    expect_eq(dnaComplement('w'), 'w');
    expect_eq(dnaComplement('K'), 'M');
    expect_eq(dnaComplement('m'), 'k');
    expect_eq(dnaComplement('B'), 'V');
    expect_eq(dnaComplement('v'), 'b');
    expect_eq(dnaComplement('D'), 'H');
    expect_eq(dnaComplement('h'), 'd');
    expect_eq(dnaComplement('N'), 'N');

    // Input with mixed case, IUPAC degenerate bases, and embedded whitespace/newlines:
    // Clean forward: "A c G t R y K m B d N"
    // Reverse:       "N d B m K y R t G c A"
    // Complement:    "N h V k M r Y a C g T"
    const seq = "AcGt\r\nRyKm BdN\n";
    let out: [16]u8 = std.mem.zeroes([16]u8);
    const n = reverseComplementDna(seq, out[0..]) ?? unreachable;
    expect_eq(n, 11);
    expect_eq(bytesEq(out[0..n], "NhVkMrYaCgT"), true);

    // Overflow -> null
    let small: [5]u8 = std.mem.zeroes([5]u8);
    expect_eq(reverseComplementDna(seq, small[0..]) == null, true);
});

test("4. CRLF line endings, leading ';' comments, empty-sequence records, and header without description", () => {
    const fasta =
        "; Legacy FASTA comment line 1\r\n" +
        "; Comment line 2\r\n" +
        "\r\n" +
        ">empty_rec\r\n" +
        ">id_only\r\n" +
        "acgt";

    expect_eq(isFasta(fasta), true);
    let it = FastaIter.init(fasta);

    const r0 = it.next() ?? unreachable;
    expect_eq(bytesEq(r0.id, "empty_rec"), true);
    expect_eq(r0.description.len, 0);
    expect_eq(r0.seqLen(), 0);
    expect_eq(r0.gcContent() == 0.0, true);

    const r1 = it.next() ?? unreachable;
    expect_eq(bytesEq(r1.id, "id_only"), true);
    expect_eq(r1.description.len, 0);
    expect_eq(r1.seqLen(), 4);

    expect_eq(it.next() == null, true);
});

test("5. Fault tolerance: empty and non-FASTA inputs", () => {
    expect_eq(isFasta(""), false);
    expect_eq(isFasta("; only comments\n\n"), false);
    expect_eq(isFasta("@FASTQ_HEADER\nACGT\n+\nIIII\n"), false);

    let it = FastaIter.init("@FASTQ_HEADER\nACGT\n");
    expect_eq(it.next() == null, true);

    const stats = summarizeFasta("Not a FASTA file");
    expect_eq(stats.record_count, 0);
    expect_eq(stats.total_bases, 0);
    expect_eq(stats.overallGc() == 0.0, true);
});
