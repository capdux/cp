import { test, expect_eq } from "cap:test";
import {
    QualitySpec, parseQValue, parseWeightedList,
    negotiateEncoding, negotiateLanguage, negotiateCharset
} from "../../packages/negotiator/src/index.cp";

fn main() {
    test("1. @cap/negotiator — parseQValue parses RFC 7231 quality weights", () => {
        expect_eq(parseQValue("1"), 1000);
        expect_eq(parseQValue("1.000"), 1000);
        expect_eq(parseQValue("0.8"), 800);
        expect_eq(parseQValue("0.125"), 125);
        expect_eq(parseQValue("0"), 0);
        expect_eq(parseQValue("0.000"), 0);
    });

    test("2. @cap/negotiator — parseWeightedList sorts descending by q-value", () => {
        let specs: [8]QualitySpec = undefined;
        let n = parseWeightedList("deflate;q=0.5, br;q=1.0, gzip;q=0.8", specs[0..8]);
        expect_eq(n, 3);
        expect_eq(specs[0].value, "br");
        expect_eq(specs[0].q_milli, 1000);
        expect_eq(specs[1].value, "gzip");
        expect_eq(specs[1].q_milli, 800);
        expect_eq(specs[2].value, "deflate");
        expect_eq(specs[2].q_milli, 500);
    });

    test("3. @cap/negotiator — negotiateEncoding picks highest q & respects q=0 exclusion", () => {
        let avail: [3]string = ["gzip", "br", "deflate"];
        expect_eq(negotiateEncoding("gzip;q=0.7, br;q=0.9", avail[0..3]) ?? "", "br");
        expect_eq(negotiateEncoding("br;q=0, gzip;q=0.5", avail[0..3]) ?? "", "gzip");
        expect_eq(negotiateEncoding("br;q=0, gzip;q=0, deflate;q=0", avail[0..3]) == null, true);
    });

    test("4. @cap/negotiator — negotiateLanguage exact & prefix tag matching", () => {
        let avail: [3]string = ["en-US", "zh-CN", "fr"];
        expect_eq(negotiateLanguage("fr;q=0.4, zh-CN;q=0.9, en;q=0.8", avail[0..3]) ?? "", "zh-CN");
        expect_eq(negotiateLanguage("en;q=0.9, de;q=1.0", avail[0..3]) ?? "", "en-US");
        expect_eq(negotiateLanguage("ja, ko", avail[0..3]) == null, true);
    });

    test("5. @cap/negotiator — negotiateCharset & wildcard fallback", () => {
        let avail: [2]string = ["utf-8", "iso-8859-1"];
        expect_eq(negotiateCharset("ISO-8859-1;q=0.9, UTF-8;q=0.5", avail[0..2]) ?? "", "iso-8859-1");
        expect_eq(negotiateCharset("*;q=0.5", avail[0..2]) ?? "", "utf-8");
    });

    test("6. @cap/negotiator — Empty Accept header defaults to first available option", () => {
        let avail: [2]string = ["br", "gzip"];
        expect_eq(negotiateEncoding("", avail[0..2]) ?? "", "br");
        expect_eq(negotiateCharset("   ", avail[0..2]) ?? "", "br");
    });
}
