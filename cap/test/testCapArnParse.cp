// cap/test/testCapArnParse.cp
// Comprehensive unit tests for @cap/arn-parse (AWS ARN parser & IAM matcher)

import { test, expect_eq } from "cap:test";
import {
    Arn,
    parseArn,
    isValidPartition,
    formatArn,
    matchResourcePattern,
    resourceType,
} from "../../packages/@aws-sdk/util-arn-parser/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/arn-parse — S3 ARN with empty region & account", () => {
        const a: Arn = parseArn("arn:aws:s3:::my-bucket/folder/key.txt");
        expect_eq(a.valid, true);
        expect_eq(sliceEq(a.partition, "aws"), true);
        expect_eq(isValidPartition(a.partition), true);
        expect_eq(sliceEq(a.service, "s3"), true);
        expect_eq(a.region.len, 0 as usize);
        expect_eq(a.account_id.len, 0 as usize);
        expect_eq(sliceEq(a.resource, "my-bucket/folder/key.txt"), true);
    });

    test("2. @cap/arn-parse — Lambda ARN with multiple colons in resource", () => {
        const a: Arn = parseArn("arn:aws-us-gov:lambda:us-gov-west-1:123456789012:function:my-fn:v1");
        expect_eq(a.valid, true);
        expect_eq(sliceEq(a.partition, "aws-us-gov"), true);
        expect_eq(isValidPartition(a.partition), true);
        expect_eq(sliceEq(a.service, "lambda"), true);
        expect_eq(sliceEq(a.region, "us-gov-west-1"), true);
        expect_eq(sliceEq(a.account_id, "123456789012"), true);
        expect_eq(sliceEq(a.resource, "function:my-fn:v1"), true);
        expect_eq(sliceEq(resourceType(a.resource), "function"), true);
    });

    test("3. @cap/arn-parse — IAM role/admin resourceType extraction", () => {
        const a: Arn = parseArn("arn:aws:iam::123456789012:role/admin");
        expect_eq(a.valid, true);
        expect_eq(sliceEq(resourceType(a.resource), "role"), true);
        expect_eq(resourceType("plain-bucket-only").len, 0 as usize);
    });

    test("4. @cap/arn-parse — formatArn round-trip", () => {
        const raw = "arn:aws-cn:dynamodb:cn-north-1:111122223333:table/Users";
        const a: Arn = parseArn(raw);
        expect_eq(a.valid, true);
        expect_eq(isValidPartition(a.partition), true);

        let buf: [128]u8 = undefined;
        const n: usize = formatArn(a, buf[0..128]);
        expect_eq(n, raw.len);
        expect_eq(sliceEq(buf[0..n], raw), true);
    });

    test("5. @cap/arn-parse — matchResourcePattern '*' and '?' wildcards", () => {
        expect_eq(matchResourcePattern("my-bucket/logs/2026.txt", "my-bucket/logs/*.txt"), true);
        expect_eq(matchResourcePattern("my-bucket/logs/2026.txt", "my-bucket/*/20??.txt"), true);
        expect_eq(matchResourcePattern("my-bucket/logs/2026.csv", "my-bucket/logs/*.txt"), false);
        expect_eq(matchResourcePattern("role/admin", "*"), true);
        expect_eq(matchResourcePattern("role/dev", "role/???"), true);
        expect_eq(matchResourcePattern("role/devs", "role/???"), false);
    });

    test("6. @cap/arn-parse — rejects invalid prefix, missing segments & tiny buffers", () => {
        expect_eq(parseArn("notarn:aws:s3:::bucket").valid, false);
        expect_eq(parseArn("arn:aws:s3::only-five").valid, false);
        expect_eq(parseArn("arn::s3:::bucket").valid, false);
        expect_eq(parseArn("arn:aws::::bucket").valid, false);
        expect_eq(parseArn("arn:aws:s3:::").valid, false);
        expect_eq(isValidPartition("aws-unknown"), false);

        const a: Arn = parseArn("arn:aws:s3:::bucket");
        let tiny: [8]u8 = undefined;
        expect_eq(formatArn(a, tiny[0..8]), 0 as usize);
    });
}
