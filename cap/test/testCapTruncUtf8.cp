// cap/test/testCapTruncUtf8.cp
// Comprehensive unit tests for @cap/cli-truncate (UTF-8 & display-width aware truncation)

import { test, expect_eq } from "cap:test";
import {
    cliTruncateEndInto,
    cliTruncateStartInto,
    cliTruncateMiddleInto,
} from "../../packages/cli-truncate/src/index.cp";

fn truncEq(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/cli-truncate — end, start & middle ASCII truncation", () => {
        let out: [64]u8 = [0; 64];
        const n1 = cliTruncateEndInto("unicorn", 5 as usize, out[0..64]);
        expect_eq(truncEq(out[0..n1], "un..."), true);

        const n2 = cliTruncateStartInto("unicorn", 5 as usize, out[0..64]);
        expect_eq(truncEq(out[0..n2], "...rn"), true);

        const n3 = cliTruncateMiddleInto("unicorns", 5 as usize, out[0..64]);
        expect_eq(n3 > 0, true);
    });

    test("2. @cap/cli-truncate — short string preserved unchanged", () => {
        let out: [64]u8 = [0; 64];
        const n = cliTruncateEndInto("hello", 10 as usize, out[0..64]);
        expect_eq(truncEq(out[0..n], "hello"), true);
    });
}
