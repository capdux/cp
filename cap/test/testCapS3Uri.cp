import { test, expect_eq } from "cap:test";
import {
    STYLE_S3,
    STYLE_VIRTUAL_HOSTED,
    STYLE_PATH,
    S3Uri,
    isValidBucketName,
    parseS3Uri,
    formatS3Uri,
} from "../../packages/@cap/s3-uri/src/index.cp";

fn eqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/s3-uri — s3:// scheme parsing", () => {
        let u: S3Uri = parseS3Uri("s3://my-data-bucket/2026/report.parquet");
        expect_eq(u.valid, true);
        expect_eq(u.style, STYLE_S3);
        expect_eq(eqStr(u.bucket, "my-data-bucket"), true);
        expect_eq(eqStr(u.key, "2026/report.parquet"), true);
        expect_eq(u.region.len, 0);
    });

    test("2. @cap/s3-uri — virtual-hosted-style with region", () => {
        let u: S3Uri = parseS3Uri("https://assets.example.s3.us-west-2.amazonaws.com/img/logo.png?versionId=1");
        expect_eq(u.valid, true);
        expect_eq(u.style, STYLE_VIRTUAL_HOSTED);
        expect_eq(eqStr(u.bucket, "assets.example"), true);
        expect_eq(eqStr(u.region, "us-west-2"), true);
        expect_eq(eqStr(u.key, "img/logo.png"), true);
    });

    test("3. @cap/s3-uri — path-style with & without region", () => {
        let u1: S3Uri = parseS3Uri("https://s3.eu-central-1.amazonaws.com/backup-bucket/db.sql.zst");
        expect_eq(u1.valid, true);
        expect_eq(u1.style, STYLE_PATH);
        expect_eq(eqStr(u1.bucket, "backup-bucket"), true);
        expect_eq(eqStr(u1.region, "eu-central-1"), true);
        expect_eq(eqStr(u1.key, "db.sql.zst"), true);

        let u2: S3Uri = parseS3Uri("https://s3.amazonaws.com/global-bucket");
        expect_eq(u2.valid, true);
        expect_eq(eqStr(u2.bucket, "global-bucket"), true);
        expect_eq(u2.key.len, 0);
    });

    test("4. @cap/s3-uri — formatS3Uri roundtrip", () => {
        let buf: [128]u8 = undefined;
        let n: usize = formatS3Uri("cap-packages", "v1/pkg.tar.gz", buf[0..]);
        expect_eq(eqStr(buf[0..n], "s3://cap-packages/v1/pkg.tar.gz"), true);
    });

    test("5. @cap/s3-uri — AWS bucket naming rules validation", () => {
        expect_eq(isValidBucketName("my-valid-bucket-123"), true);
        expect_eq(isValidBucketName("ab"), false);
        expect_eq(isValidBucketName("UpperCase"), false);
        expect_eq(isValidBucketName("bad..dots"), false);
        expect_eq(isValidBucketName("192.168.1.1"), false);
        expect_eq(isValidBucketName("-leading-dash"), false);
    });

    test("6. @cap/s3-uri — invalid URIs & tiny buffer protection", () => {
        expect_eq(parseS3Uri("https://example.com/bucket/key").valid, false);
        expect_eq(parseS3Uri("s3://").valid, false);
        expect_eq(parseS3Uri("s3://BAD_BUCKET/key").valid, false);
        let tiny: [5]u8 = undefined;
        expect_eq(formatS3Uri("valid-bucket", "key", tiny[0..]), 0);
    });
}
