import { test, expect_eq } from "cap:test";
import {
    SPEC_WILDCARD,
    SPEC_PARTIAL,
    SPEC_EXACT,
    NegSpec,
    parseAcceptList,
    bestEncoding,
    bestMediaType,
    bestLanguage,
} from "../../packages/@cap/negotiator/src/index.cp";

fn main() {
    test("1. @cap/negotiator — parseAcceptList weights, default q=1000 & Q= tolerance", () => {
        const hdr: string = "text/html, application/json; Q=0.85, text/*;q=0.5, */*;q=0.1";
        let specs: [8]NegSpec = undefined;
        const n = parseAcceptList(hdr, &specs);
        expect_eq(n, 4);

        expect_eq(specs[0].q_milli, 1000);
        expect_eq(specs[0].specificity, SPEC_EXACT);
        expect_eq(specs[0].order, 0);

        expect_eq(specs[1].q_milli, 850);
        expect_eq(specs[1].specificity, SPEC_EXACT);

        expect_eq(specs[2].q_milli, 500);
        expect_eq(specs[2].specificity, SPEC_PARTIAL);

        expect_eq(specs[3].q_milli, 100);
        expect_eq(specs[3].specificity, SPEC_WILDCARD);
    });

    test("2. @cap/negotiator — bestEncoding selects highest q & excludes q=0", () => {
        const avail: [3]string = ["br", "gzip", "identity"];
        expect_eq(bestEncoding("gzip, deflate, br;q=0.9, identity;q=0.1", &avail), 1);
        expect_eq(bestEncoding("br;q=1.0, gzip;q=0.8", &avail), 0);

        // Explicit q=0 overrides wildcard '*'
        const avail2: [2]string = ["gzip", "br"];
        expect_eq(bestEncoding("gzip;q=0, *", &avail2), 1);
        expect_eq(bestEncoding("gzip;q=0, br;q=0", &avail2), -1);
    });

    test("3. @cap/negotiator — bestMediaType exact > type/* > */* specificity", () => {
        const avail: [3]string = ["image/png", "text/plain", "application/json"];
        // Equal q=0.9: exact application/json (4) beats text/* (2) beats */* (1)
        expect_eq(
            bestMediaType("*/*;q=0.9, text/*;q=0.9, application/json;q=0.9", &avail),
            2
        );
        // Higher q wins over higher specificity
        expect_eq(
            bestMediaType("text/*;q=0.95, application/json;q=0.5", &avail),
            1
        );
        // Wildcard fallback
        expect_eq(bestMediaType("*/*", &avail), 0);
    });

    test("4. @cap/negotiator — bestLanguage BCP-47 prefix fallback & case insensitivity", () => {
        const avail: [3]string = ["de", "en", "ja"];
        expect_eq(bestLanguage("fr-CH, fr;q=0.9, en-US;q=0.8", &avail), 1);

        const avail_reg: [2]string = ["en-GB", "zh-CN"];
        expect_eq(bestLanguage("ZH-cn;q=0.9, en;q=0.7", &avail_reg), 1);
        expect_eq(bestLanguage("en;q=0.8", &avail_reg), 0);
    });

    test("5. @cap/negotiator — No match returns -1 across all negotiators", () => {
        const encs: [2]string = ["zstd", "lz4"];
        expect_eq(bestEncoding("gzip, br", &encs), -1);

        const types: [2]string = ["application/xml", "text/csv"];
        expect_eq(bestMediaType("application/json, image/*", &types), -1);

        const langs: [2]string = ["ko", "ja"];
        expect_eq(bestLanguage("es-ES, pt-BR;q=0.8", &langs), -1);
    });

    test("6. @cap/negotiator — Empty header defaults, empty available & small buffer safety", () => {
        const encs: [2]string = ["gzip", "identity"];
        expect_eq(bestEncoding("", &encs), 1); // prefers identity on empty header

        const types: [2]string = ["application/json", "text/html"];
        expect_eq(bestMediaType("", &types), 0);
        expect_eq(bestLanguage("", &types), 0);

        const empty_avail: []const string = encs[0..0];
        expect_eq(bestEncoding("gzip", empty_avail), -1);
        expect_eq(bestMediaType("*/*", empty_avail), -1);
        expect_eq(bestLanguage("*", empty_avail), -1);

        // Small output slice truncation in parseAcceptList
        let tiny: [2]NegSpec = undefined;
        expect_eq(parseAcceptList("a, b, c, d", &tiny), 2);
        expect_eq(parseAcceptList("", &tiny), 0);
    });
}
