// @cap/natural-cmp — Zero-heap arbitrary-precision human natural sort comparator & sorter
// MIT License (clean-room CP implementation)

import { isDigit, toLowerByte } from "cap:ascii";

fn compareImpl(a: string, b: string, ignore_case: bool) -> i32 {
    let ia: usize = 0;
    let ib: usize = 0;

    while (ia < a.len && ib < b.len) {
        if (isDigit(a[ia]) && isDigit(b[ib])) {
            let a_end: usize = ia;
            while (a_end < a.len && isDigit(a[a_end])) {
                a_end += 1;
            }
            let b_end: usize = ib;
            while (b_end < b.len && isDigit(b[b_end])) {
                b_end += 1;
            }

            // Skip leading zeros for arbitrary-precision numeric comparison
            let a_nz: usize = ia;
            while (a_nz < a_end && a[a_nz] == '0' as u8) {
                a_nz += 1;
            }
            let b_nz: usize = ib;
            while (b_nz < b_end && b[b_nz] == '0' as u8) {
                b_nz += 1;
            }

            const a_sig: usize = a_end - a_nz;
            const b_sig: usize = b_end - b_nz;
            if (a_sig < b_sig) {
                return -1;
            }
            if (a_sig > b_sig) {
                return 1;
            }

            let k: usize = 0;
            while (k < a_sig) {
                const da: u8 = a[a_nz + k];
                const db: u8 = b[b_nz + k];
                if (da < db) {
                    return -1;
                }
                if (da > db) {
                    return 1;
                }
                k += 1;
            }

            // Numerically equal: fewer leading zeros sorts first ("1" < "01")
            const a_run: usize = a_end - ia;
            const b_run: usize = b_end - ib;
            if (a_run < b_run) {
                return -1;
            }
            if (a_run > b_run) {
                return 1;
            }

            ia = a_end;
            ib = b_end;
            continue;
        }

        let ca: u8 = a[ia];
        let cb: u8 = b[ib];
        if (ignore_case) {
            ca = toLowerByte(ca);
            cb = toLowerByte(cb);
        }
        if (ca < cb) {
            return -1;
        }
        if (ca > cb) {
            return 1;
        }
        ia += 1;
        ib += 1;
    }

    if (ia < a.len) {
        return 1;
    }
    if (ib < b.len) {
        return -1;
    }
    return 0;
}

pub fn compareNatural(a: string, b: string) -> i32 {
    return compareImpl(a, b, false);
}

pub fn compareNaturalIgnoreCase(a: string, b: string) -> i32 {
    return compareImpl(a, b, true);
}

pub fn isSortedNatural(items: []const string) -> bool {
    if (items.len <= 1) {
        return true;
    }
    let i: usize = 1;
    while (i < items.len) {
        if (compareNatural(items[i - 1], items[i]) > 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn sortNatural(items: []string) {
    const n: usize = items.len;
    if (n <= 1) {
        return;
    }
    const gaps: [8]usize = [701, 301, 132, 57, 23, 10, 4, 1];
    let g: usize = 0;
    while (g < 8) {
        const gap: usize = gaps[g];
        let i: usize = gap;
        while (i < n) {
            const key: string = items[i];
            let j: usize = i;
            while (j >= gap && compareNatural(items[j - gap], key) > 0) {
                items[j] = items[j - gap];
                j -= gap;
            }
            items[j] = key;
            i += 1;
        }
        g += 1;
    }
}
