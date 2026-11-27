// @cap/luhn — Zero-heap Check Digit Algorithms (Luhn ISO/IEC 7812, Damm, Verhoeff)
// Clean-room MIT implementation in pure CP (no heap allocations, no FFI).

// ISO/IEC 7812 Luhn Mod-10 validation (ignores spaces and hyphens; rejects other non-digits)
pub fn luhnValidate(input: []const u8): bool {
    let sum: u32 = 0;
    let digits: usize = 0;
    let double_next: bool = false;
    let i: usize = input.len;
    while (i > 0) {
        i -= 1;
        let ch: u8 = input[i];
        if (ch == ' ' || ch == '-') {
            continue;
        }
        if (ch < '0' || ch > '9') {
            return false;
        }
        let d: u32 = (ch - '0') as u32;
        if (double_next) {
            d = d * 2;
            if (d > 9) {
                d = d - 9;
            }
        }
        sum = sum + d;
        double_next = !double_next;
        digits += 1;
    }
    return digits >= 2 && (sum % 10) == 0;
}

// Compute Luhn Mod-10 check digit (0..9) to append to `payload`; returns -1 if invalid or empty
pub fn luhnCheckDigit(payload: []const u8): i32 {
    let sum: u32 = 0;
    let digits: usize = 0;
    let double_next: bool = true;
    let i: usize = payload.len;
    while (i > 0) {
        i -= 1;
        let ch: u8 = payload[i];
        if (ch == ' ' || ch == '-') {
            continue;
        }
        if (ch < '0' || ch > '9') {
            return -1;
        }
        let d: u32 = (ch - '0') as u32;
        if (double_next) {
            d = d * 2;
            if (d > 9) {
                d = d - 9;
            }
        }
        sum = sum + d;
        double_next = !double_next;
        digits += 1;
    }
    if (digits == 0) {
        return -1;
    }
    return ((10 - (sum % 10)) % 10) as i32;
}

// Damm (2004) totally anti-symmetric quasigroup of order 10: detects ALL single-digit & adjacent transposition errors
const DAMM_TABLE: [100]u8 = [
    0, 3, 1, 7, 5, 9, 8, 6, 4, 2,
    7, 0, 9, 2, 1, 5, 4, 8, 6, 3,
    4, 2, 0, 6, 8, 7, 1, 3, 5, 9,
    1, 7, 5, 0, 9, 8, 3, 4, 2, 6,
    6, 1, 2, 3, 0, 4, 5, 9, 7, 8,
    3, 6, 7, 4, 2, 0, 9, 5, 8, 1,
    5, 8, 6, 9, 7, 2, 0, 1, 3, 4,
    8, 9, 4, 5, 3, 6, 2, 0, 1, 7,
    9, 4, 3, 8, 6, 1, 7, 2, 0, 5,
    2, 5, 8, 1, 4, 3, 6, 7, 9, 0,
];

pub fn dammCheckDigit(payload: []const u8): i32 {
    let interim: usize = 0;
    let digits: usize = 0;
    let i: usize = 0;
    while (i < payload.len) {
        let ch: u8 = payload[i];
        if (ch == ' ' || ch == '-') {
            i += 1;
            continue;
        }
        if (ch < '0' || ch > '9') {
            return -1;
        }
        let col: usize = (ch - '0') as usize;
        interim = DAMM_TABLE[interim * 10 + col] as usize;
        digits += 1;
        i += 1;
    }
    if (digits == 0) {
        return -1;
    }
    return interim as i32;
}

pub fn dammValidate(input: []const u8): bool {
    let interim: usize = 0;
    let digits: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        let ch: u8 = input[i];
        if (ch == ' ' || ch == '-') {
            i += 1;
            continue;
        }
        if (ch < '0' || ch > '9') {
            return false;
        }
        let col: usize = (ch - '0') as usize;
        interim = DAMM_TABLE[interim * 10 + col] as usize;
        digits += 1;
        i += 1;
    }
    return digits >= 2 && interim == 0;
}

// Verhoeff (1969) dihedral group D5 check digit algorithm
const VERHOEFF_D: [100]u8 = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    1, 2, 3, 4, 0, 6, 7, 8, 9, 5,
    2, 3, 4, 0, 1, 7, 8, 9, 5, 6,
    3, 4, 0, 1, 2, 8, 9, 5, 6, 7,
    4, 0, 1, 2, 3, 9, 5, 6, 7, 8,
    5, 9, 8, 7, 6, 0, 4, 3, 2, 1,
    6, 5, 9, 8, 7, 1, 0, 4, 3, 2,
    7, 6, 5, 9, 8, 2, 1, 0, 4, 3,
    8, 7, 6, 5, 9, 3, 2, 1, 0, 4,
    9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
];

const VERHOEFF_P: [80]u8 = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    1, 5, 7, 6, 2, 8, 3, 0, 9, 4,
    5, 8, 0, 3, 7, 9, 6, 1, 4, 2,
    8, 9, 1, 6, 0, 4, 3, 5, 2, 7,
    9, 4, 5, 3, 1, 2, 6, 8, 7, 0,
    4, 2, 8, 6, 5, 7, 3, 9, 0, 1,
    2, 7, 9, 3, 8, 0, 6, 4, 1, 5,
    7, 0, 4, 6, 9, 1, 3, 2, 5, 8,
];

const VERHOEFF_INV: [10]u8 = [0, 4, 3, 2, 1, 5, 6, 7, 8, 9];

pub fn verhoeffCheckDigit(payload: []const u8): i32 {
    let c: usize = 0;
    let digits: usize = 0;
    let i: usize = payload.len;
    while (i > 0) {
        i -= 1;
        let ch: u8 = payload[i];
        if (ch == ' ' || ch == '-') {
            continue;
        }
        if (ch < '0' || ch > '9') {
            return -1;
        }
        let d: usize = (ch - '0') as usize;
        let perm_row: usize = (digits + 1) % 8;
        let p_val: usize = VERHOEFF_P[perm_row * 10 + d] as usize;
        c = VERHOEFF_D[c * 10 + p_val] as usize;
        digits += 1;
    }
    if (digits == 0) {
        return -1;
    }
    return VERHOEFF_INV[c] as i32;
}

pub fn verhoeffValidate(input: []const u8): bool {
    let c: usize = 0;
    let digits: usize = 0;
    let i: usize = input.len;
    while (i > 0) {
        i -= 1;
        let ch: u8 = input[i];
        if (ch == ' ' || ch == '-') {
            continue;
        }
        if (ch < '0' || ch > '9') {
            return false;
        }
        let d: usize = (ch - '0') as usize;
        let perm_row: usize = digits % 8;
        let p_val: usize = VERHOEFF_P[perm_row * 10 + d] as usize;
        c = VERHOEFF_D[c * 10 + p_val] as usize;
        digits += 1;
    }
    return digits >= 2 && c == 0;
}
