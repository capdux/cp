import std from "std";
import { test, expect_eq } from "cap:test";
import { S3MetaAcl, S3MetaStorageClass, isValidBucketName, S3Uri } from "cap:s3";

test("1. S3MetaAcl round-trips all 8 canned ACL policies and rejects unknown values", () => {
    const names: [8][]const u8 = [
        "private",
        "public-read",
        "public-read-write",
        "aws-exec-read",
        "authenticated-read",
        "bucket-owner-read",
        "bucket-owner-full-control",
        "log-delivery-write",
    ];
    for (name of names) {
        const parsed = S3MetaAcl.parse(name) ?? unreachable;
        expect_eq(std.mem.eql(u8, parsed.asStr(), name), true);
    }
    expect_eq(S3MetaAcl.parse("PRIVATE") == null, true);
    expect_eq(S3MetaAcl.parse("public") == null, true);
    expect_eq(S3MetaAcl.parse("") == null, true);
});

test("2. S3MetaStorageClass round-trips all 11 tiers and identifies archive tiers", () => {
    const tiers: [11][]const u8 = [
        "STANDARD",
        "STANDARD_IA",
        "INTELLIGENT_TIERING",
        "EXPRESS_ONEZONE",
        "ONEZONE_IA",
        "GLACIER",
        "GLACIER_IR",
        "REDUCED_REDUNDANCY",
        "OUTPOSTS",
        "DEEP_ARCHIVE",
        "SNOW",
    ];
    for (tier of tiers) {
        const parsed = S3MetaStorageClass.parse(tier) ?? unreachable;
        expect_eq(std.mem.eql(u8, parsed.asStr(), tier), true);
    }

    expect_eq(S3MetaStorageClass.GLACIER.isArchiveTier(), true);
    expect_eq(S3MetaStorageClass.GLACIER_IR.isArchiveTier(), true);
    expect_eq(S3MetaStorageClass.DEEP_ARCHIVE.isArchiveTier(), true);
    expect_eq(S3MetaStorageClass.STANDARD.isArchiveTier(), false);
    expect_eq(S3MetaStorageClass.STANDARD_IA.isArchiveTier(), false);
    expect_eq(S3MetaStorageClass.EXPRESS_ONEZONE.isArchiveTier(), false);
    expect_eq(S3MetaStorageClass.SNOW.isArchiveTier(), false);

    expect_eq(S3MetaStorageClass.parse("standard") == null, true);
    expect_eq(S3MetaStorageClass.parse("UNKNOWN") == null, true);
});

test("3. isValidBucketName enforces AWS length, charset, boundary, double-dot, and IPv4 rules", () => {
    // 合法名称
    expect_eq(isValidBucketName("abc"), true);
    expect_eq(isValidBucketName("my-prod-bucket-2026"), true);
    expect_eq(isValidBucketName("logs.us-east-1.company"), true);
    expect_eq(isValidBucketName("0123456789abcdefghijklmnopqrstuvwxyz-0123456789abcdefghijklmno1"), true); // 63 chars

    // 非法名称
    expect_eq(isValidBucketName("ab"), false); // 过短 (< 3)
    expect_eq(isValidBucketName("a123456789012345678901234567890123456789012345678901234567890123"), false); // 64 chars
    expect_eq(isValidBucketName("MyBucket"), false); // 大写
    expect_eq(isValidBucketName("my_bucket"), false); // 下划线
    expect_eq(isValidBucketName("-leading-dash"), false);
    expect_eq(isValidBucketName("trailing-dash-"), false);
    expect_eq(isValidBucketName(".leading-dot"), false);
    expect_eq(isValidBucketName("trailing-dot."), false);
    expect_eq(isValidBucketName("double..dot"), false);
    expect_eq(isValidBucketName("192.168.5.4"), false); // IPv4 格式禁止
    expect_eq(isValidBucketName("10.0.0.1"), false);
});

test("4. S3Uri parses s3://, virtual-hosted-style, and path-style URLs", () => {
    // 1. s3:// scheme
    const u1 = S3Uri.parse("s3://my-data-lake/2026/11/events.parquet") ?? unreachable;
    expect_eq(std.mem.eql(u8, u1.bucket, "my-data-lake"), true);
    expect_eq(std.mem.eql(u8, u1.key, "2026/11/events.parquet"), true);
    expect_eq(u1.region == null, true);

    // 2. Virtual-hosted-style with region
    const u2 = S3Uri.parse("https://assets-cdn.s3.ap-southeast-2.amazonaws.com/static/app.js?versionId=123") ?? unreachable;
    expect_eq(std.mem.eql(u8, u2.bucket, "assets-cdn"), true);
    expect_eq(std.mem.eql(u8, u2.key, "static/app.js"), true);
    expect_eq(std.mem.eql(u8, u2.region ?? unreachable, "ap-southeast-2"), true);

    // 3. Path-style with region
    const u3 = S3Uri.parse("https://s3.us-west-2.amazonaws.com/backup-vault/db/dump.sql.zst") ?? unreachable;
    expect_eq(std.mem.eql(u8, u3.bucket, "backup-vault"), true);
    expect_eq(std.mem.eql(u8, u3.key, "db/dump.sql.zst"), true);
    expect_eq(std.mem.eql(u8, u3.region ?? unreachable, "us-west-2"), true);

    // 4. Legacy global virtual-hosted-style without region
    const u4 = S3Uri.parse("https://legacy-bucket.s3.amazonaws.com/readme.txt") ?? unreachable;
    expect_eq(std.mem.eql(u8, u4.bucket, "legacy-bucket"), true);
    expect_eq(std.mem.eql(u8, u4.key, "readme.txt"), true);
    expect_eq(u4.region == null, true);
});

test("5. S3Uri.toCanonicalS3 formatting, buffer bounds, and malformed URI rejection", () => {
    const u = S3Uri.parse("https://s3.eu-central-1.amazonaws.com/media-store/videos/intro.mp4") ?? unreachable;
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const canon = u.toCanonicalS3(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, canon, "s3://media-store/videos/intro.mp4"), true);

    // 仅 bucket 无 key
    const root_u = S3Uri.parse("s3://root-bucket") ?? unreachable;
    const canon_root = root_u.toCanonicalS3(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, canon_root, "s3://root-bucket"), true);

    // 缓冲区不足返回 null
    let tiny: [10]u8 = std.mem.zeroes([10]u8);
    expect_eq(u.toCanonicalS3(&tiny) == null, true);

    // 非法 URI 拦截
    expect_eq(S3Uri.parse("") == null, true);
    expect_eq(S3Uri.parse("s3://") == null, true);
    expect_eq(S3Uri.parse("s3://BadBucket/key") == null, true);
    expect_eq(S3Uri.parse("https://example.com/bucket/key") == null, true);
    expect_eq(S3Uri.parse("https://s3.us-east-1.amazonaws.com/") == null, true);
    expect_eq(S3Uri.parse("https://192.168.1.1.s3.us-east-1.amazonaws.com/k") == null, true);
});
