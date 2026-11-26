// cap/test/testCapRedact.cp
// Comprehensive unit tests for @cap/redact

import { test, expect_eq } from "cap:test";
import {
    RedactOptions,
    maskEmail,
    maskPhone,
    maskCard,
    maskSecret,
    maskIp,
    redactText,
    containsSensitive,
    redactJsonFields,
} from "../../packages/@cap/redact/src/index.cp";

fn main() {
    test("1. @cap/redact — Single-value PII maskers (email, phone, card, secret, IP)", () => {
        let buf: [128]u8 = undefined;
        expect_eq(maskEmail("alice@capdux.org", buf[0..128]), "a***e@capdux.org");
        expect_eq(maskEmail("ab@x.io", buf[0..128]), "a***@x.io");

        expect_eq(maskPhone("13800138000", buf[0..128]), "138****8000");
        expect_eq(maskPhone("+8613800138000", buf[0..128]), "+86138****8000");

        expect_eq(maskCard("4532-0151-1283-0366", buf[0..128]), "****-****-****-0366");
        expect_eq(maskSecret("sk-proj-1234567890abcdef", buf[0..128]), "sk-p***cdef");
        expect_eq(maskSecret("short", buf[0..128]), "****");
        expect_eq(maskIp("192.168.1.42", buf[0..128]), "192.168.*.*");
    });

    test("2. @cap/redact — Auto-redacting emails, API tokens & JWTs in logs", () => {
        let out: [512]u8 = undefined;
        const raw = "login alice@capdux.org key=sk-proj-1234567890abcdef jwt=eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiIxMjMifQ.sig1234567890";
        const clean = redactText(raw, RedactOptions.defaultAll(), out[0..512]);
        expect_eq(clean, "login a***e@capdux.org key=sk-p***cdef jwt=eyJh***7890");
    });

    test("3. @cap/redact — Auto-redacting Luhn cards, phones & strict IPv4", () => {
        let out: [512]u8 = undefined;
        const raw = "pay card=4532-0151-1283-0366 tel=13800138000 from 10.20.30.40";
        const clean = redactText(raw, RedactOptions.strict(), out[0..512]);
        expect_eq(clean, "pay card=****-****-****-0366 tel=138****8000 from 10.20.*.*");
    });

    test("4. @cap/redact — Structured JSON sensitive field redaction", () => {
        let out: [512]u8 = undefined;
        const raw = "{\"user\":\"alice\",\"password\":\"hunter2\",\"api_key\":\"capx_secret123\",\"role\":\"admin\"}";
        const clean = redactJsonFields(raw, out[0..512]);
        expect_eq(clean, "{\"user\":\"alice\",\"password\":\"[REDACTED]\",\"api_key\":\"[REDACTED]\",\"role\":\"admin\"}");
    });

    test("5. @cap/redact — Fast containsSensitive leak detector", () => {
        expect_eq(containsSensitive("normal log message count=42"), false);
        expect_eq(containsSensitive("leaked ghp_1234567890abcdef123456"), true);
        expect_eq(containsSensitive("contact bob@example.com"), true);
        expect_eq(containsSensitive("call +61412345678 now"), true);
    });
}
