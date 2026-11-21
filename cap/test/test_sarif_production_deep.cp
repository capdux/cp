// Source: cp/cap/test/test_sarif_production_deep.cp (Production deep tests for cap:sarif)

import { test, expect_eq } from "cap:test";
import {
    SarifLevel,
    SarifHeader,
    SarifRun,
    SarifResult,
    SarifStats,
    SarifRunIter,
    SarifResultIter,
    isSarif,
    parseSarifHeader,
    summarizeSarif,
} from "cap:sarif";

fn bytesEq(a: []const u8, b: []const u8): bool {
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

test("1. Multi-run (CodeQL + Semgrep) full SARIF 2.1.0 header, runs, and results parsing", () => {
    const sarif =
        "{\n" +
        "  \"$schema\": \"https://json.schemastore.org/sarif-2.1.0.json\",\n" +
        "  \"version\": \"2.1.0\",\n" +
        "  \"runs\": [\n" +
        "    {\n" +
        "      \"tool\": {\n" +
        "        \"driver\": {\n" +
        "          \"name\": \"CodeQL\",\n" +
        "          \"version\": \"2.18.0\",\n" +
        "          \"informationUri\": \"https://codeql.github.com\"\n" +
        "        }\n" +
        "      },\n" +
        "      \"results\": [\n" +
        "        {\n" +
        "          \"ruleId\": \"cpp/buffer-overflow\",\n" +
        "          \"level\": \"error\",\n" +
        "          \"message\": { \"text\": \"Potential out-of-bounds write\" },\n" +
        "          \"locations\": [\n" +
        "            {\n" +
        "              \"physicalLocation\": {\n" +
        "                \"artifactLocation\": { \"uri\": \"src/parser.cp\" },\n" +
        "                \"region\": { \"startLine\": 42, \"startColumn\": 9, \"endLine\": 44 }\n" +
        "              }\n" +
        "            }\n" +
        "          ]\n" +
        "        }\n" +
        "      ]\n" +
        "    },\n" +
        "    {\n" +
        "      \"tool\": {\n" +
        "        \"driver\": {\n" +
        "          \"name\": \"Semgrep\",\n" +
        "          \"version\": \"1.90.0\"\n" +
        "        }\n" +
        "      },\n" +
        "      \"results\": [\n" +
        "        {\n" +
        "          \"ruleId\": \"cp.unused-import\",\n" +
        "          \"level\": \"note\",\n" +
        "          \"message\": { \"text\": \"Unused symbol imported\" },\n" +
        "          \"locations\": [\n" +
        "            {\n" +
        "              \"physicalLocation\": {\n" +
        "                \"artifactLocation\": { \"uri\": \"src/main.cp\" },\n" +
        "                \"region\": { \"startLine\": 3, \"startColumn\": 1 }\n" +
        "              }\n" +
        "            }\n" +
        "          ]\n" +
        "        }\n" +
        "      ]\n" +
        "    }\n" +
        "  ]\n" +
        "}";

    expect_eq(isSarif(sarif), true);

    const hdr = parseSarifHeader(sarif) ?? unreachable;
    expect_eq(bytesEq(hdr.version, "2.1.0"), true);
    expect_eq(bytesEq(hdr.schema_uri, "https://json.schemastore.org/sarif-2.1.0.json"), true);

    let rit = SarifRunIter.init(hdr.runs_slice);
    const run1 = rit.next() ?? unreachable;
    expect_eq(bytesEq(run1.tool_name, "CodeQL"), true);
    expect_eq(bytesEq(run1.tool_version, "2.18.0"), true);
    expect_eq(bytesEq(run1.info_uri, "https://codeql.github.com"), true);

    let res1_it = run1.results();
    const r1 = res1_it.next() ?? unreachable;
    expect_eq(bytesEq(r1.rule_id, "cpp/buffer-overflow"), true);
    expect_eq(r1.level == .error_level, true);
    expect_eq(bytesEq(r1.message, "Potential out-of-bounds write"), true);
    expect_eq(bytesEq(r1.artifact_uri, "src/parser.cp"), true);
    expect_eq(r1.start_line, 42);
    expect_eq(r1.start_col, 9);
    expect_eq(r1.end_line, 44);
    expect_eq(res1_it.next() == null, true);

    const run2 = rit.next() ?? unreachable;
    expect_eq(bytesEq(run2.tool_name, "Semgrep"), true);
    let res2_it = run2.results();
    const r2 = res2_it.next() ?? unreachable;
    expect_eq(bytesEq(r2.rule_id, "cp.unused-import"), true);
    expect_eq(r2.level == .note, true);
    expect_eq(bytesEq(r2.artifact_uri, "src/main.cp"), true);
    expect_eq(r2.start_line, 3);
    expect_eq(r2.start_col, 1);
    expect_eq(r2.end_line, 0);

    expect_eq(rit.next() == null, true);
    expect_eq(rit.had_error, false);
});

test("2. Omitted level defaults to SarifLevel.warning per SARIF 2.1.0 specification", () => {
    const sarif =
        "{\"version\":\"2.1.0\",\"runs\":[{\"tool\":{\"driver\":{\"name\":\"Clippy\"}}," +
        "\"results\":[{\"ruleId\":\"clippy::needless_return\",\"message\":{\"text\":\"unneeded return\"}}]}]}";

    const hdr = parseSarifHeader(sarif) ?? unreachable;
    let rit = SarifRunIter.init(hdr.runs_slice);
    const run = rit.next() ?? unreachable;
    let res_it = run.results();
    const res = res_it.next() ?? unreachable;
    expect_eq(res.level == .warning, true);
    expect_eq(bytesEq(res.rule_id, "clippy::needless_return"), true);

    const stats = summarizeSarif(sarif);
    expect_eq(stats.warning_count, 1);
    expect_eq(stats.hasErrors(), false);
});

test("3. Escaped quotes and deeply nested codeFlows / relatedLocations safe skipping", () => {
    const sarif =
        "{\"version\":\"2.1.0\",\"runs\":[{\"tool\":{\"driver\":{\"name\":\"ESLint\"}}," +
        "\"results\":[{" +
        "\"codeFlows\":[{\"threadFlows\":[{\"locations\":[{\"location\":{\"message\":{\"text\":\"step \\\"1\\\" with } and ]\"}}}]}]}]," +
        "\"relatedLocations\":[{\"id\":1,\"message\":{\"text\":\"related { [ ] }\"}}]," +
        "\"ruleId\":\"no-eval\"," +
        "\"level\":\"error\"," +
        "\"message\":{\"text\":\"Avoid \\\"eval\\\" calls\"}," +
        "\"locations\":[{\"physicalLocation\":{\"artifactLocation\":{\"uri\":\"app.js\"},\"region\":{\"startLine\":10,\"startColumn\":5}}}]" +
        "}]}]}";

    const hdr = parseSarifHeader(sarif) ?? unreachable;
    let rit = SarifRunIter.init(hdr.runs_slice);
    const run = rit.next() ?? unreachable;
    let res_it = run.results();
    const res = res_it.next() ?? unreachable;
    expect_eq(bytesEq(res.rule_id, "no-eval"), true);
    expect_eq(res.level == .error_level, true);
    expect_eq(bytesEq(res.message, "Avoid \\\"eval\\\" calls"), true);
    expect_eq(bytesEq(res.artifact_uri, "app.js"), true);
    expect_eq(res.start_line, 10);
    expect_eq(res.start_col, 5);
});

test("4. summarizeSarif counts across all levels (error, warning, note, none)", () => {
    const sarif =
        "{\"version\":\"2.1.0\",\"runs\":[" +
        "{\"tool\":{\"driver\":{\"name\":\"A\"}},\"results\":[" +
        "{\"ruleId\":\"R1\",\"level\":\"error\"}," +
        "{\"ruleId\":\"R2\",\"level\":\"warning\"}," +
        "{\"ruleId\":\"R3\"}" +
        "]}," +
        "{\"tool\":{\"driver\":{\"name\":\"B\"}},\"results\":[" +
        "{\"ruleId\":\"R4\",\"level\":\"note\"}," +
        "{\"ruleId\":\"R5\",\"level\":\"none\"}" +
        "]}" +
        "]}";

    const st = summarizeSarif(sarif);
    expect_eq(st.run_count, 2);
    expect_eq(st.total_results, 5);
    expect_eq(st.error_count, 1);
    expect_eq(st.warning_count, 2); // 1 explicit + 1 default
    expect_eq(st.note_count, 1);
    expect_eq(st.none_count, 1);
    expect_eq(st.hasErrors(), true);
});

test("5. Non-SARIF JSON and truncated / malformed JSON fault tolerance", () => {
    expect_eq(isSarif("{\"name\":\"not-sarif\"}"), false);
    expect_eq(isSarif("[1, 2, 3]"), false);
    expect_eq(isSarif("{\"version\":\"2.1.0\",\"runs\":["), false);
    expect_eq(parseSarifHeader("{\"version\":\"2.1.0\"}") == null, true);
    expect_eq(parseSarifHeader("{\"runs\":123}") == null, true);

    const empty_stats = summarizeSarif("not json at all");
    expect_eq(empty_stats.run_count, 0);
    expect_eq(empty_stats.total_results, 0);
    expect_eq(empty_stats.hasErrors(), false);
});
