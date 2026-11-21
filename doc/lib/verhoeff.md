# `cap:verhoeff` — Zero-Allocation Verhoeff ($D_5$), Damm Quasigroup & ISO/IEC 7064 Check Digit Suite

Pure CP, zero-heap-allocation check-digit library implementing the **Verhoeff** dihedral group $D_5$ algorithm, the **Damm** order-10 totally anti-symmetric quasigroup algorithm, and **ISO/IEC 7064** Mod-11-2 (ORCID / ISNI) & Mod-37-2 alphanumeric check characters.

## Core Functions

| Symbol | Description |
| :--- | :--- |
| `verhoeffCheckDigit(digits: []const u8): u8?` | Computes Verhoeff $D_5$ check digit (`'0'..'9'`); detects 100% single-digit & adjacent transposition errors. |
| `isValidVerhoeff(digits_with_check: []const u8): bool` | Validates a digit string ending with its Verhoeff check digit. |
| `verhoeffAppend(digits: []const u8, out: *[]u8): usize?` | Writes `digits` + Verhoeff check digit into `out`, returning `digits.len + 1`. |
| `dammCheckDigit(digits: []const u8): u8?` | Computes Damm 10×10 quasigroup check digit (`'0'..'9'`). |
| `isValidDamm(digits_with_check: []const u8): bool` | Validates a digit string ending with its Damm check digit. |
| `dammAppend(digits: []const u8, out: *[]u8): usize?` | Writes `digits` + Damm check digit into `out`, returning `digits.len + 1`. |
| `iso7064Mod11_2CheckDigit(digits: []const u8): u8?` | Computes ISO/IEC 7064 Mod-11-2 check char (`'0'..'9'` or `'X'`), ignoring `'-'` and `' '`. |
| `isValidIso7064Mod11_2(code: []const u8): bool` | Validates an ORCID / ISNI / Mod-11-2 code, ignoring `'-'` and `' '`. |
| `iso7064Mod37_2CheckDigit(alnum: []const u8): u8?` | Computes ISO/IEC 7064 Mod-37-2 check char (`'0'..'9'`, `'A'..'Z'`, or `'*'`). |
| `isValidIso7064Mod37_2(code: []const u8): bool` | Validates a Mod-37-2 alphanumeric code, ignoring `'-'` and `' '`. |

## Example

```cp
import {
    verhoeffAppend,
    isValidVerhoeff,
    isValidIso7064Mod11_2,
} from "cap:verhoeff";

fn validate(buf: *[]u8): bool {
    if (n := verhoeffAppend("236", buf)) {
        return isValidVerhoeff(buf.*[0..n]) && isValidIso7064Mod11_2("0000-0002-1694-233X");
    }
    return false;
}
```
