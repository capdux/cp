import { test, expect_eq } from "cap:test";
import { Sort, SortOf } from "cap:sort";

fn cmpDesc(a: i32, b: i32): bool {
    return a > b;
}

fn cmpAsc(a: i32, b: i32): bool {
    return a < b;
}

fn main() {
    test("Sort.asc and Sort.desc with PDQSort", () => {
        let arr: [6]i32 = [42, -5, 18, 0, 99, 7];
        Sort.asc(arr[0..]);
        expect_eq(arr[0], -5);
        expect_eq(arr[1], 0);
        expect_eq(arr[2], 7);
        expect_eq(arr[3], 18);
        expect_eq(arr[4], 42);
        expect_eq(arr[5], 99);
        expect_eq(Sort.isSortedAsc(arr[0..]), true);

        Sort.desc(arr[0..]);
        expect_eq(arr[0], 99);
        expect_eq(arr[1], 42);
        expect_eq(arr[5], -5);
        expect_eq(Sort.isSortedAsc(arr[0..]), false);
    });

    test("Sort.sort with custom comparator", () => {
        let arr: [5]i32 = [3, 1, 4, 1, 5];
        Sort.sort(arr[0..], cmpDesc);
        expect_eq(arr[0], 5);
        expect_eq(arr[1], 4);
        expect_eq(arr[2], 3);
        expect_eq(arr[3], 1);
        expect_eq(arr[4], 1);
        expect_eq(Sort.isSorted(arr[0..], cmpDesc), true);
    });

    test("Sort.stable with BlockSort", () => {
        let arr: [6]i32 = [8, 3, 2, 7, 4, 6];
        Sort.stable(arr[0..], cmpAsc);
        expect_eq(arr[0], 2);
        expect_eq(arr[1], 3);
        expect_eq(arr[2], 4);
        expect_eq(arr[3], 6);
        expect_eq(arr[4], 7);
        expect_eq(arr[5], 8);
    });

    test("Sort.heap with HeapSort", () => {
        let arr: [5]i32 = [100, 20, 50, 10, 80];
        Sort.heap(arr[0..], cmpAsc);
        expect_eq(arr[0], 10);
        expect_eq(arr[1], 20);
        expect_eq(arr[2], 50);
        expect_eq(arr[3], 80);
        expect_eq(arr[4], 100);
    });

    test("Binary search & bounds", () => {
        let arr: [7]i32 = [10, 20, 30, 30, 30, 40, 50];
        let found: usize? = Sort.binarySearch(arr[0..], 40);
        expect_eq(found == 5, true);

        let missing: usize? = Sort.binarySearch(arr[0..], 25);
        expect_eq(missing == null, true);

        let lb: usize = Sort.lowerBound(arr[0..], 30);
        expect_eq(lb, 2);

        let ub: usize = Sort.upperBound(arr[0..], 30);
        expect_eq(ub, 5);
    });

    test("Extrema and argExtrema", () => {
        let arr: [5]i32 = [15, 3, 89, -12, 44];
        let mn: i32? = Sort.minI32(arr[0..]);
        let mx: i32? = Sort.maxI32(arr[0..]);
        expect_eq(mn == -12, true);
        expect_eq(mx == 89, true);

        let arg_min: usize? = Sort.argMin(arr[0..]);
        let arg_max: usize? = Sort.argMax(arr[0..]);
        expect_eq(arg_min == 3, true);
        expect_eq(arg_max == 2, true);
    });
}
