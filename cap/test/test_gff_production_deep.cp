// Source: cp/cap/test/test_gff_production_deep.cp (Production deep tests for cap:gff)

import { test, expect_eq } from "cap:test";
import {
    GFF_PHASE_NONE,
    GffRecord,
    GffStats,
    GffIter,
    isGff3,
    parseGffLine,
    summarizeGff,
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

test("1. GFF3 header, 9-column records, 1-based closed length, and ID/Parent/Name lookup", () => {
    const gff3 =
        "##gff-version 3\r\n" +
        "##sequence-region chr1 1 10000\r\n" +
        "chr1\tEnsembl\tgene\t1000\t2000\t.\t+\t.\tID=gene001;Name=TP53\r\n" +
        "chr1\tEnsembl\tmRNA\t1000\t2000\t98.5\t+\t.\tID=mrna001;Parent=gene001\r\n" +
        "chr1\tEnsembl\tCDS\t1100\t1399\t1.5e2\t+\t0\tID=cds001;Parent=mrna001\r\n";

    expect_eq(isGff3(gff3), true);

    let it = GffIter.init(gff3);
    const g = it.next() ?? unreachable;
    expect_eq(bytesEq(g.seqid, "chr1"), true);
    expect_eq(bytesEq(g.source, "Ensembl"), true);
    expect_eq(bytesEq(g.feature_type, "gene"), true);
    expect_eq(g.start, 1000);
    expect_eq(g.end, 2000);
    expect_eq(g.length(), 1001); // 1-based closed [1000, 2000]
    expect_eq(g.has_score, false);
    expect_eq(g.strand, '+');
    expect_eq(g.phase, GFF_PHASE_NONE);
    expect_eq(bytesEq(g.findAttr("ID") ?? "", "gene001"), true);
    expect_eq(bytesEq(g.findAttr("Name") ?? "", "TP53"), true);
    expect_eq(g.findAttr("Missing") == null, true);

    const m = it.next() ?? unreachable;
    expect_eq(m.has_score, true);
    expect_eq(m.score > 98.4 && m.score < 98.6, true);
    expect_eq(bytesEq(m.findAttr("Parent") ?? "", "gene001"), true);

    const c = it.next() ?? unreachable;
    expect_eq(c.phase, 0);
    expect_eq(c.length(), 300);
    expect_eq(c.score > 149.9 && c.score < 150.1, true);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("2. GTF2 quoted attribute parsing (gene_id, transcript_id, semicolons inside quotes)", () => {
    const gtf_line =
        "chr13\tHAVANA\texon\t32315474\t32315667\t.\t+\t.\t" +
        "gene_id \"ENSG00000139618\"; transcript_id \"ENST00000380152\"; note \"a;b\"; gene_name \"BRCA2\";";

    const rec = parseGffLine(gtf_line) ?? unreachable;
    expect_eq(bytesEq(rec.feature_type, "exon"), true);
    expect_eq(rec.length(), 194);
    expect_eq(bytesEq(rec.findAttr("gene_id") ?? "", "ENSG00000139618"), true);
    expect_eq(bytesEq(rec.findAttr("transcript_id") ?? "", "ENST00000380152"), true);
    expect_eq(bytesEq(rec.findAttr("note") ?? "", "a;b"), true);
    expect_eq(bytesEq(rec.findAttr("gene_name") ?? "", "BRCA2"), true);
});

test("3. ##FASTA embedded sequence section immediate stop", () => {
    const gff_with_fasta =
        "##gff-version 3\n" +
        "chr1\tref\tgene\t1\t10\t.\t+\t.\tID=g1\n" +
        "##FASTA\n" +
        ">chr1\n" +
        "ACGTACGTAC\n";

    let it = GffIter.init(gff_with_fasta);
    const r0 = it.next() ?? unreachable;
    expect_eq(bytesEq(r0.findAttr("ID") ?? "", "g1"), true);
    expect_eq(r0.length(), 10);

    // Stops cleanly at ##FASTA without marking had_error
    expect_eq(it.next() == null, true);
    expect_eq(it.reached_fasta, true);
    expect_eq(it.had_error, false);
});

test("4. summarizeGff single-pass feature statistics and 1-bp closed interval", () => {
    const gff =
        "##gff-version 3\n" +
        "chr1\tsrc\tgene\t1\t100\t.\t+\t.\tID=g1\n" +   // 100 bp, +
        "chr1\tsrc\texon\t1\t50\t.\t+\t.\tID=e1\n" +    // 50 bp, +
        "chr1\tsrc\tCDS\t10\t10\t.\t-\t1\tID=c1\n" +    // 1 bp (single-base closed [10,10]), -
        "chr1\tsrc\tCDS\t60\t89\t.\t-\t2\tID=c2\n";     // 30 bp, -

    const stats = summarizeGff(gff);
    expect_eq(stats.record_count, 4);
    expect_eq(stats.gene_count, 1);
    expect_eq(stats.exon_count, 1);
    expect_eq(stats.cds_count, 2);
    expect_eq(stats.total_feature_bp, 181);
    expect_eq(stats.plus_count, 2);
    expect_eq(stats.minus_count, 2);
});

test("5. Fault tolerance: < 9 columns, start == 0, start > end, invalid phase/strand", () => {
    expect_eq(isGff3("# not gff3\n"), false);

    // Fewer than 9 tab-separated columns
    expect_eq(parseGffLine("chr1\tsrc\tgene\t1\t10\t.\t+\t.") == null, true);

    // 1-based start == 0 is illegal in GFF3/GTF2
    expect_eq(parseGffLine("chr1\tsrc\tgene\t0\t10\t.\t+\t.\tID=bad") == null, true);

    // start > end
    expect_eq(parseGffLine("chr1\tsrc\tgene\t20\t10\t.\t+\t.\tID=bad") == null, true);

    // Invalid phase '3'
    expect_eq(parseGffLine("chr1\tsrc\tCDS\t1\t10\t.\t+\t3\tID=bad") == null, true);

    // Invalid strand 'X'
    expect_eq(parseGffLine("chr1\tsrc\tCDS\t1\t10\t.\tX\t0\tID=bad") == null, true);

    // Invalid score
    expect_eq(parseGffLine("chr1\tsrc\tCDS\t1\t10\tNaN\t+\t0\tID=bad") == null, true);

    // GffIter sets had_error on malformed feature line
    let it = GffIter.init("chr1\tsrc\tgene\t1\t10\t.\t+\t.\tID=ok\nbad_line\n");
    * = it.next() ?? unreachable;
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, true);
});
