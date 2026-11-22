// cap/test/test_s3_endpoint_production_deep.cp
// 深度生产级回归测试：cap:s3 多云端点解析、Region/Bucket 推导与安全编码 (lib/s3/endpoint.cp)

import { test } from "cap:test";
import {
    S3_PRESIGN_MIN_EXPIRES,
    S3_PRESIGN_MAX_EXPIRES,
    S3_PRESIGN_DEFAULT_EXPIRES,
    S3EndpointProvider,
    containsS3Crlf,
    validateS3HeaderValue,
    validateS3PresignExpiry,
    normalizeS3ObjectKey,
    encodeS3UriComponent,
    guessS3Bucket,
    guessS3Region,
    S3ParsedEndpoint,
} from "cap:s3";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

test("s3_endpoint: multi-cloud provider detection and region/bucket guessing") {
    // 1. AWS S3 virtual-hosted
    const aws = S3ParsedEndpoint.parse("https://my-assets.s3.ap-southeast-1.amazonaws.com/path")?;
    if (aws.provider != S3EndpointProvider.AwsS3 || !aws.is_https || aws.requires_path_style) {
        return err.AwsProviderMismatch;
    }
    if (!bytesEq(aws.guessed_region, "ap-southeast-1")) { return err.AwsRegionMismatch; }
    if (b := aws.guessed_bucket) {
        if (!bytesEq(b, "my-assets")) { return err.AwsBucketMismatch; }
    } else { return err.AwsBucketMissing; }

    // 2. Cloudflare R2 (account-only vs virtual-hosted bucket)
    const r2_acct = S3ParsedEndpoint.parse("https://acc12345.r2.cloudflarestorage.com")?;
    if (r2_acct.provider != S3EndpointProvider.CloudflareR2 || !bytesEq(r2_acct.guessed_region, "auto")) {
        return err.R2ProviderMismatch;
    }
    if (r2_acct.guessed_bucket != null) { return err.R2AccountOnlyShouldHaveNullBucket; }

    const r2_vh = S3ParsedEndpoint.parse("https://media.acc12345.r2.cloudflarestorage.com")?;
    if (b := r2_vh.guessed_bucket) {
        if (!bytesEq(b, "media")) { return err.R2BucketMismatch; }
    } else { return err.R2BucketMissing; }

    // 3. DigitalOcean Spaces & Backblaze B2
    const do_ep = S3ParsedEndpoint.parse("https://nyc3.digitaloceanspaces.com")?;
    if (do_ep.provider != S3EndpointProvider.DigitalOceanSpaces || !bytesEq(do_ep.guessed_region, "nyc3")) {
        return err.DigitalOceanMismatch;
    }
    const b2_ep = S3ParsedEndpoint.parse("https://s3.us-west-004.backblazeb2.com")?;
    if (b2_ep.provider != S3EndpointProvider.BackblazeB2 || !bytesEq(b2_ep.guessed_region, "us-west-004")) {
        return err.BackblazeMismatch;
    }

    // 4. MinIO localhost with port
    const minio = S3ParsedEndpoint.parse("http://localhost:9000")?;
    if (minio.provider != S3EndpointProvider.MinioLocal || minio.is_https || minio.port != 9000 || !minio.requires_path_style) {
        return err.MinioMismatch;
    }

    // 5. 空端点 region 默认回退 us-east-1
    if (!bytesEq(guessS3Region(""), "us-east-1")) { return err.EmptyRegionDefaultMismatch; }
    if (guessS3Bucket("http://127.0.0.1:9000") != null) { return err.IpShouldNotGuessBucket; }
}

test("s3_endpoint: virtual-hosted vs path-style URL formatting and dotted-bucket fallback") {
    const aws = S3ParsedEndpoint.parse("https://s3.us-east-1.amazonaws.com")?;
    let buf: [256]u8 = [0; 256];

    // 普通 bucket 开启 virtual_hosted -> virtual-hosted URL
    const n1 = aws.formatObjectUrl("my-bucket", "/folder\\hello world.json", true, buf[0..])?;
    if (!bytesEq(buf[0..n1], "https://my-bucket.s3.us-east-1.amazonaws.com/folder/hello%20world.json")) {
        return err.VhostUrlMismatch;
    }

    // 含 `.` 的 bucket 即使开启 virtual_hosted 也自动降级为 Path-Style（避免 TLS 通配证书失效）
    const n2 = aws.formatObjectUrl("static.example.com", "img/logo.png", true, buf[0..])?;
    if (!bytesEq(buf[0..n2], "https://s3.us-east-1.amazonaws.com/static.example.com/img/logo.png")) {
        return err.DottedBucketFallbackMismatch;
    }

    // MinIO 本地端口强制 Path-Style 并保留端口
    const minio = S3ParsedEndpoint.parse("http://127.0.0.1:9000")?;
    const n3 = minio.formatObjectUrl("dev-bucket", "a+b/c.txt", true, buf[0..])?;
    if (!bytesEq(buf[0..n3], "http://127.0.0.1:9000/dev-bucket/a%2Bb/c.txt")) {
        return err.MinioUrlMismatch;
    }
}

test("s3_endpoint: normalizeS3ObjectKey and RFC 3986 encodeS3UriComponent") {
    if (!bytesEq(normalizeS3ObjectKey("///dir/file.txt//"), "dir/file.txt")) {
        return err.NormalizeSlashMismatch;
    }
    if (!bytesEq(normalizeS3ObjectKey("\\a\\b\\"), "a\\b")) {
        return err.NormalizeBackslashMismatch;
    }
    if (!bytesEq(normalizeS3ObjectKey("///"), "")) {
        return err.NormalizeAllSlashesMismatch;
    }

    let buf: [128]u8 = [0; 128];
    // encode_slash = false: 保留 `/` 且 Windows `\` -> `/`
    const n1 = encodeS3UriComponent("a\\b c/d~e_f-g.h", buf[0..], false)?;
    if (!bytesEq(buf[0..n1], "a/b%20c/d~e_f-g.h")) {
        return err.EncodeKeepSlashMismatch;
    }
    // encode_slash = true: `/` 编码为 `%2F`
    const n2 = encodeS3UriComponent("a/b+c", buf[0..], true)?;
    if (!bytesEq(buf[0..n2], "a%2Fb%2Bc")) {
        return err.EncodeEscapeSlashMismatch;
    }
}

test("s3_endpoint: presign expiry bounds validation (1..=604800)") {
    if ((validateS3PresignExpiry(S3_PRESIGN_MIN_EXPIRES)?) != 1) { return err.MinExpiryFailed; }
    if ((validateS3PresignExpiry(S3_PRESIGN_DEFAULT_EXPIRES)?) != 86400) { return err.DefaultExpiryFailed; }
    if ((validateS3PresignExpiry(S3_PRESIGN_MAX_EXPIRES)?) != 604800) { return err.MaxExpiryFailed; }

    let zero_rejected = false;
    * = validateS3PresignExpiry(0) catch blk: {
        zero_rejected = true;
        break :blk 0;
    };
    if (!zero_rejected) { return err.ZeroExpiryNotRejected; }

    let over_rejected = false;
    * = validateS3PresignExpiry(604801) catch blk: {
        over_rejected = true;
        break :blk 0;
    };
    if (!over_rejected) { return err.OverMaxExpiryNotRejected; }
}

test("s3_endpoint: CRLF header injection guard rejects \\r and \\n") {
    if (containsS3Crlf("attachment; filename=\"report.pdf\"")) {
        return err.CleanHeaderFalsePositive;
    }
    if (!(validateS3HeaderValue("public-read")?)) {
        return err.CleanHeaderValidationFailed;
    }

    if (!containsS3Crlf("bad\r\nX-Injected: 1")) {
        return err.CrlfNotDetected;
    }
    let crlf_rejected = false;
    validateS3HeaderValue("val\nInjected: yes") catch {
        crlf_rejected = true;
    };
    if (!crlf_rejected) { return err.CrlfHeaderNotRejected; }

    // Endpoint 或 bucket/key 含 CRLF 同样拒绝
    let ep_crlf = false;
    const dummy = S3ParsedEndpoint.parse("https://s3.us-east-1.amazonaws.com\r\n") catch blk: {
        ep_crlf = true;
        break :blk S3ParsedEndpoint{};
    };
    * = dummy;
    if (!ep_crlf) { return err.EndpointCrlfNotRejected; }
}

test("s3_endpoint: malformed endpoint, invalid port, empty bucket and tiny buffer guards") {
    // 空端点拒绝
    let empty_ep = false;
    const d1 = S3ParsedEndpoint.parse("") catch blk: {
        empty_ep = true;
        break :blk S3ParsedEndpoint{};
    };
    * = d1;
    if (!empty_ep) { return err.EmptyEndpointNotRejected; }

    // 非法协议（ftp://）拒绝
    let bad_scheme = false;
    const d2 = S3ParsedEndpoint.parse("ftp://s3.amazonaws.com") catch blk: {
        bad_scheme = true;
        break :blk S3ParsedEndpoint{};
    };
    * = d2;
    if (!bad_scheme) { return err.BadSchemeNotRejected; }

    // 越界端口拒绝
    let bad_port = false;
    const d3 = S3ParsedEndpoint.parse("http://localhost:99999") catch blk: {
        bad_port = true;
        break :blk S3ParsedEndpoint{};
    };
    * = d3;
    if (!bad_port) { return err.BadPortNotRejected; }

    // 空 bucket 与缓冲区过小拒绝
    const ep = S3ParsedEndpoint.parse("https://s3.us-east-1.amazonaws.com")?;
    let buf: [64]u8 = [0; 64];
    let empty_bkt = false;
    ep.formatObjectUrl("", "k", true, buf[0..]) catch {
        empty_bkt = true;
    };
    if (!empty_bkt) { return err.EmptyBucketNotRejected; }

    let tiny: [8]u8 = [0; 8];
    let buf_small = false;
    ep.formatObjectUrl("my-bucket", "some/key.txt", true, tiny[0..]) catch {
        buf_small = true;
    };
    if (!buf_small) { return err.BufferTooSmallNotRejected; }
}
