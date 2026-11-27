// cap/test/testCapNatsort.cp
// Comprehensive unit tests for @cap/natsort

import { test, expect_eq } from "cap:test";
import {
    naturalCompare,
    naturalCompareIgnoreCase,
    naturalLess,
    naturalLessIgnoreCase,
    naturalEq,
    sortNatural,
    sortNaturalDesc,
    sortNaturalIgnoreCase,
    isSortedNatural,
} from "../../packages/natural-compare/src/index.cp";

fn main() {
    test("1. @cap/natsort — filenames (file1/file2/file10/file20)", () => {
        expect_eq(naturalCompare("file2.txt", "file10.txt"), -1 as i8);
        expect_eq(naturalCompare("file10.txt", "file2.txt"), 1 as i8);
        expect_eq(naturalCompare("file10.txt", "file10.txt"), 0 as i8);
        expect_eq(naturalLess("file1.txt", "file2.txt"), true);
        expect_eq(naturalLess("file2.txt", "file10.txt"), true);
        expect_eq(naturalLess("file10.txt", "file20.txt"), true);
        expect_eq(naturalLess("file20.txt", "file2.txt"), false);
        expect_eq(naturalEq("file10.txt", "file10.txt"), true);
        expect_eq(naturalEq("file10.txt", "file20.txt"), false);

        let files: [4]string = ["file10.txt", "file20.txt", "file2.txt", "file1.txt"];
        expect_eq(isSortedNatural(files[0..4]), false);
        sortNatural(files[0..4]);
        expect_eq(isSortedNatural(files[0..4]), true);
        expect_eq(files[0], "file1.txt");
        expect_eq(files[1], "file2.txt");
        expect_eq(files[2], "file10.txt");
        expect_eq(files[3], "file20.txt");
    });

    test("2. @cap/natsort — semver-like versions (v1.2.0/v1.2.9/v1.2.10/v1.10.0)", () => {
        expect_eq(naturalCompare("v1.2.9", "v1.2.10"), -1 as i8);
        expect_eq(naturalCompare("v1.2.10", "v1.10.0"), -1 as i8);
        expect_eq(naturalCompare("v1.2.0", "v1.2.10"), -1 as i8);

        let vers: [5]string = ["v1.10.0", "v1.2.10", "v1.2.0", "v1.2.9", "v2.0.0"];
        sortNatural(vers[0..5]);
        expect_eq(isSortedNatural(vers[0..5]), true);
        expect_eq(vers[0], "v1.2.0");
        expect_eq(vers[1], "v1.2.9");
        expect_eq(vers[2], "v1.2.10");
        expect_eq(vers[3], "v1.10.0");
        expect_eq(vers[4], "v2.0.0");
    });

    test("3. @cap/natsort — huge 30+ digit numbers without u64 overflow", () => {
        // u64::MAX is 18446744073709551615 (20 digits); use 30-digit & 31-digit numbers
        const a = "item_123456789012345678901234567890";
        const b = "item_123456789012345678901234567891";
        const c = "item_999999999999999999999999999999";
        const d = "item_1000000000000000000000000000000";

        expect_eq(naturalCompare(a, b), -1 as i8);
        expect_eq(naturalCompare(b, a), 1 as i8);
        expect_eq(naturalCompare(b, c), -1 as i8);
        expect_eq(naturalCompare(c, d), -1 as i8);
        expect_eq(naturalCompare(d, c), 1 as i8);
        expect_eq(naturalEq(a, a), true);

        // Leading zeros on huge numbers
        const az = "item_0000123456789012345678901234567890";
        expect_eq(naturalCompare(az, b), -1 as i8);
        expect_eq(naturalCompare(az, a), -1 as i8);
    });

    test("4. @cap/natsort — leading zeros deterministic total ordering", () => {
        // Same numeric value -> deterministic raw-byte tie-breaker
        // '0' (48) < '1' (49), so "a001" < "a01" < "a1"
        expect_eq(naturalCompare("a001", "a1"), -1 as i8);
        expect_eq(naturalCompare("a1", "a001"), 1 as i8);
        expect_eq(naturalCompare("a001", "a01"), -1 as i8);
        expect_eq(naturalCompare("a01", "a1"), -1 as i8);
        expect_eq(naturalEq("a001", "a1"), false);

        // Zero runs: "a0" is prefix of "a00", so "a0" < "a00"
        expect_eq(naturalCompare("a0", "a00"), -1 as i8);
        expect_eq(naturalCompare("a00", "a0"), 1 as i8);
        expect_eq(naturalCompare("a00", "a01"), -1 as i8);

        let items: [4]string = ["a1", "a001", "a01", "a2"];
        sortNatural(items[0..4]);
        expect_eq(items[0], "a001");
        expect_eq(items[1], "a01");
        expect_eq(items[2], "a1");
        expect_eq(items[3], "a2");
    });

    test("5. @cap/natsort — ASCII case-insensitive comparison & sort", () => {
        // Case-insensitive natural order
        expect_eq(naturalCompareIgnoreCase("File2", "file10"), -1 as i8);
        expect_eq(naturalCompareIgnoreCase("file10", "File2"), 1 as i8);
        expect_eq(naturalLessIgnoreCase("alpha2", "BETA1"), true);
        expect_eq(naturalLessIgnoreCase("BETA1", "alpha2"), false);

        // Tie-break on raw bytes when case-folded forms are equal ('F' 70 < 'f' 102)
        expect_eq(naturalCompareIgnoreCase("File2", "file2"), -1 as i8);
        expect_eq(naturalCompareIgnoreCase("file2", "File2"), 1 as i8);
        expect_eq(naturalCompareIgnoreCase("file2", "file2"), 0 as i8);

        let names: [6]string = ["file10", "File2", "file2", "alpha1", "Alpha10", "Beta3"];
        sortNaturalIgnoreCase(names[0..6]);
        expect_eq(names[0], "alpha1");
        expect_eq(names[1], "Alpha10");
        expect_eq(names[2], "Beta3");
        expect_eq(names[3], "File2");
        expect_eq(names[4], "file2");
        expect_eq(names[5], "file10");
    });

    test("6. @cap/natsort — ascending/descending in-place sort, edge cases & heapsort (>16 items)", () => {
        // Empty & single-element slices
        let single: [1]string = ["only1"];
        expect_eq(isSortedNatural(single[0..0]), true);
        sortNatural(single[0..0]);
        sortNaturalDesc(single[0..0]);
        expect_eq(isSortedNatural(single[0..1]), true);
        sortNatural(single[0..1]);
        expect_eq(single[0], "only1");

        // Descending sort
        let desc_arr: [4]string = ["img2.png", "img10.png", "img1.png", "img20.png"];
        sortNaturalDesc(desc_arr[0..4]);
        expect_eq(isSortedNatural(desc_arr[0..4]), false);
        expect_eq(desc_arr[0], "img20.png");
        expect_eq(desc_arr[1], "img10.png");
        expect_eq(desc_arr[2], "img2.png");
        expect_eq(desc_arr[3], "img1.png");

        // 20 elements to exercise the heapsort path (n > 16)
        let big: [20]string = [
            "n19", "n2", "n10", "n1", "n20",
            "n5", "n15", "n3", "n12", "n8",
            "n18", "n4", "n11", "n7", "n16",
            "n9", "n14", "n6", "n13", "n17",
        ];
        expect_eq(isSortedNatural(big[0..20]), false);
        sortNatural(big[0..20]);
        expect_eq(isSortedNatural(big[0..20]), true);
        expect_eq(big[0], "n1");
        expect_eq(big[1], "n2");
        expect_eq(big[8], "n9");
        expect_eq(big[9], "n10");
        expect_eq(big[18], "n19");
        expect_eq(big[19], "n20");

        sortNaturalDesc(big[0..20]);
        expect_eq(big[0], "n20");
        expect_eq(big[1], "n19");
        expect_eq(big[18], "n2");
        expect_eq(big[19], "n1");
    });
}
