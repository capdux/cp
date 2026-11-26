import { test, expect_eq } from "cap:test";
import {
    LevnKind,
    LevnValue,
    LevnPair,
    parseLevnScalar,
    parseLevnList,
    parseLevnObject,
} from "../../packages/@cap/levn/src/index.cp";

fn main() {
    test("1. @cap/levn — parses null and boolean scalars", () => {
        const n = parseLevnScalar(" null ");
        expect_eq(n.kind == LevnKind.Null, true);

        const t = parseLevnScalar("true");
        expect_eq(t.kind == LevnKind.Bool, true);
        expect_eq(t.bool_val, true);

        const f = parseLevnScalar("  false\t");
        expect_eq(f.kind == LevnKind.Bool, true);
        expect_eq(f.bool_val, false);
    });

    test("2. @cap/levn — parses integer and floating-point scalars", () => {
        const pos = parseLevnScalar("8080");
        expect_eq(pos.kind == LevnKind.Int, true);
        expect_eq(pos.int_val, 8080 as i64);

        const neg = parseLevnScalar("-42");
        expect_eq(neg.kind == LevnKind.Int, true);
        expect_eq(neg.int_val, -42 as i64);

        const flt = parseLevnScalar("3.25");
        expect_eq(flt.kind == LevnKind.Float, true);
        expect_eq(flt.float_val, 3.25);

        const nflt = parseLevnScalar("-0.5");
        expect_eq(nflt.kind == LevnKind.Float, true);
        expect_eq(nflt.float_val, -0.5);
    });

    test("3. @cap/levn — parses quoted strings, bare words, and invalid inputs", () => {
        const dq = parseLevnScalar("\"hello, world\"");
        expect_eq(dq.kind == LevnKind.String, true);
        expect_eq(dq.str_val, "hello, world");

        const sq = parseLevnScalar("'single quoted'");
        expect_eq(sq.kind == LevnKind.String, true);
        expect_eq(sq.str_val, "single quoted");

        const bare = parseLevnScalar("localhost");
        expect_eq(bare.kind == LevnKind.String, true);
        expect_eq(bare.str_val, "localhost");

        const empty = parseLevnScalar("   ");
        expect_eq(empty.kind == LevnKind.Invalid, true);

        const bad_quote = parseLevnScalar("\"unterminated");
        expect_eq(bad_quote.kind == LevnKind.Invalid, true);
    });

    test("4. @cap/levn — parseLevnList handles bracketed and bare lists with quoted commas", () => {
        let vals: [8]LevnValue = undefined;
        const n1 = parseLevnList("[alpha, 42, true, 'a, b', null]", vals[0..8]);
        expect_eq(n1, 5 as usize);
        expect_eq(vals[0].kind == LevnKind.String, true);
        expect_eq(vals[0].str_val, "alpha");
        expect_eq(vals[1].kind == LevnKind.Int, true);
        expect_eq(vals[1].int_val, 42 as i64);
        expect_eq(vals[2].kind == LevnKind.Bool, true);
        expect_eq(vals[2].bool_val, true);
        expect_eq(vals[3].kind == LevnKind.String, true);
        expect_eq(vals[3].str_val, "a, b");
        expect_eq(vals[4].kind == LevnKind.Null, true);

        const n2 = parseLevnList("x, 1.5, false", vals[0..8]);
        expect_eq(n2, 3 as usize);
        expect_eq(vals[0].str_val, "x");
        expect_eq(vals[1].float_val, 1.5);
        expect_eq(vals[2].bool_val, false);
    });

    test("5. @cap/levn — parseLevnObject parses light object notation", () => {
        let pairs: [8]LevnPair = undefined;
        const n = parseLevnObject("{host: localhost, port: 8080, tls: true, ratio: 0.75}", pairs[0..8]);
        expect_eq(n, 4 as usize);

        expect_eq(pairs[0].key, "host");
        expect_eq(pairs[0].value.kind == LevnKind.String, true);
        expect_eq(pairs[0].value.str_val, "localhost");

        expect_eq(pairs[1].key, "port");
        expect_eq(pairs[1].value.kind == LevnKind.Int, true);
        expect_eq(pairs[1].value.int_val, 8080 as i64);

        expect_eq(pairs[2].key, "tls");
        expect_eq(pairs[2].value.kind == LevnKind.Bool, true);
        expect_eq(pairs[2].value.bool_val, true);

        expect_eq(pairs[3].key, "ratio");
        expect_eq(pairs[3].value.kind == LevnKind.Float, true);
        expect_eq(pairs[3].value.float_val, 0.75);
    });

    test("6. @cap/levn — parseLevnObject handles quoted keys/values and empty collections", () => {
        let pairs: [4]LevnPair = undefined;
        const n = parseLevnObject("{'user-agent': \"cap, v1:2\", timeout: null}", pairs[0..4]);
        expect_eq(n, 2 as usize);
        expect_eq(pairs[0].key, "user-agent");
        expect_eq(pairs[0].value.str_val, "cap, v1:2");
        expect_eq(pairs[1].key, "timeout");
        expect_eq(pairs[1].value.kind == LevnKind.Null, true);

        let vals: [4]LevnValue = undefined;
        expect_eq(parseLevnList("[]", vals[0..4]), 0 as usize);
        expect_eq(parseLevnObject("{}", pairs[0..4]), 0 as usize);
    });
}
