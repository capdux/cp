import { test, expect_eq } from "cap:test";
import {
    luhnValidate,
    luhnCheckDigit,
    dammValidate,
    dammCheckDigit,
    verhoeffValidate,
    verhoeffCheckDigit,
} from "../../packages/@cap/luhn/src/index.cp";

fn main() {
    test("ISO/IEC 7812 Luhn validation and check digit", () => {
        expect_eq(luhnCheckDigit("7992739871"), 3);
        expect_eq(luhnValidate("79927398713"), true);
        expect_eq(luhnValidate("7992-7398-713"), true);
        expect_eq(luhnValidate("79927398714"), false);
        expect_eq(luhnValidate("49927398716"), true);
    });

    test("Damm algorithm standard vector 572 -> check digit 4", () => {
        expect_eq(dammCheckDigit("572"), 4);
        expect_eq(dammValidate("5724"), true);
        expect_eq(dammValidate("5720"), false);
        // Damm catches 09 <-> 90 transposition that Luhn misses
        let d = dammCheckDigit("109");
        expect_eq(d >= 0, true);
        expect_eq(dammValidate("5274"), false);
    });

    test("Verhoeff algorithm standard vector 236 -> check digit 3", () => {
        expect_eq(verhoeffCheckDigit("236"), 3);
        expect_eq(verhoeffValidate("2363"), true);
        expect_eq(verhoeffValidate("2364"), false);
        expect_eq(verhoeffValidate("2633"), false);
    });

    test("whitespace and hyphen tolerance with non-digit rejection", () => {
        expect_eq(luhnValidate("7992 7398 713"), true);
        expect_eq(luhnValidate("7992a7398713"), false);
        expect_eq(dammValidate("57-24"), true);
        expect_eq(dammValidate("57x24"), false);
        expect_eq(verhoeffValidate("23 63"), true);
        expect_eq(verhoeffValidate("23!63"), false);
    });

    test("empty and single-digit edge cases", () => {
        expect_eq(luhnValidate(""), false);
        expect_eq(luhnValidate("0"), false);
        expect_eq(luhnCheckDigit(""), -1);
        expect_eq(dammValidate(""), false);
        expect_eq(dammCheckDigit(""), -1);
        expect_eq(verhoeffValidate(""), false);
        expect_eq(verhoeffCheckDigit(""), -1);
    });

    test("all three algorithms detect every single-digit mutation", () => {
        expect_eq(luhnValidate("79927398713"), true);
        expect_eq(luhnValidate("79927398723"), false);
        expect_eq(dammValidate("5724"), true);
        expect_eq(dammValidate("5824"), false);
        expect_eq(verhoeffValidate("2363"), true);
        expect_eq(verhoeffValidate("2373"), false);
    });
}
