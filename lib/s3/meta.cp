// Source: Absorbed from /data/dev/bun/src/s3_signing/{acl,storage_class,credentials}.cp

fn strEql(a: []const u8, b: []const u8): bool {
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return strEql(s[0..prefix.len], prefix);
}

fn endsWith(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) {
        return false;
    }
    return strEql(s[s.len - suffix.len..], suffix);
}

fn findByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn findSubstr(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0 || haystack.len < needle.len) {
        return null;
    }
    let i: usize = 0;
    const limit = haystack.len - needle.len;
    while (i <= limit) {
        if (strEql(haystack[i..i + needle.len], needle)) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// AWS S3 / R2 / MinIO 8 种标准 Canned ACL 访问控制策略
export enum S3Acl {
    private_acl,
    public_read,
    public_read_write,
    aws_exec_read,
    authenticated_read,
    bucket_owner_read,
    bucket_owner_full_control,
    log_delivery_write,

    /// 解析标准 S3 ACL 字符串
    fn parse(s: []const u8): S3Acl? {
        if (strEql(s, "private")) return S3Acl.private_acl;
        if (strEql(s, "public-read")) return S3Acl.public_read;
        if (strEql(s, "public-read-write")) return S3Acl.public_read_write;
        if (strEql(s, "aws-exec-read")) return S3Acl.aws_exec_read;
        if (strEql(s, "authenticated-read")) return S3Acl.authenticated_read;
        if (strEql(s, "bucket-owner-read")) return S3Acl.bucket_owner_read;
        if (strEql(s, "bucket-owner-full-control")) return S3Acl.bucket_owner_full_control;
        if (strEql(s, "log-delivery-write")) return S3Acl.log_delivery_write;
        return null;
    }

    /// 返回标准 HTTP 头 `x-amz-acl` 字符串表示
    fn asStr(): []const u8 {
        return switch (_) {
            .private_acl => "private",
            .public_read => "public-read",
            .public_read_write => "public-read-write",
            .aws_exec_read => "aws-exec-read",
            .authenticated_read => "authenticated-read",
            .bucket_owner_read => "bucket-owner-read",
            .bucket_owner_full_control => "bucket-owner-full-control",
            .log_delivery_write => "log-delivery-write",
        };
    }
}

/// AWS S3 11 种标准存储类别
export enum S3StorageClass {
    STANDARD,
    STANDARD_IA,
    INTELLIGENT_TIERING,
    EXPRESS_ONEZONE,
    ONEZONE_IA,
    GLACIER,
    GLACIER_IR,
    REDUCED_REDUNDANCY,
    OUTPOSTS,
    DEEP_ARCHIVE,
    SNOW,

    /// 解析标准 S3 StorageClass 字符串
    fn parse(s: []const u8): S3StorageClass? {
        if (strEql(s, "STANDARD")) return S3StorageClass.STANDARD;
        if (strEql(s, "STANDARD_IA")) return S3StorageClass.STANDARD_IA;
        if (strEql(s, "INTELLIGENT_TIERING")) return S3StorageClass.INTELLIGENT_TIERING;
        if (strEql(s, "EXPRESS_ONEZONE")) return S3StorageClass.EXPRESS_ONEZONE;
        if (strEql(s, "ONEZONE_IA")) return S3StorageClass.ONEZONE_IA;
        if (strEql(s, "GLACIER")) return S3StorageClass.GLACIER;
        if (strEql(s, "GLACIER_IR")) return S3StorageClass.GLACIER_IR;
        if (strEql(s, "REDUCED_REDUNDANCY")) return S3StorageClass.REDUCED_REDUNDANCY;
        if (strEql(s, "OUTPOSTS")) return S3StorageClass.OUTPOSTS;
        if (strEql(s, "DEEP_ARCHIVE")) return S3StorageClass.DEEP_ARCHIVE;
        if (strEql(s, "SNOW")) return S3StorageClass.SNOW;
        return null;
    }

    /// 返回标准 HTTP 头 `x-amz-storage-class` 字符串表示
    fn asStr(): []const u8 {
        return switch (_) {
            .STANDARD => "STANDARD",
            .STANDARD_IA => "STANDARD_IA",
            .INTELLIGENT_TIERING => "INTELLIGENT_TIERING",
            .EXPRESS_ONEZONE => "EXPRESS_ONEZONE",
            .ONEZONE_IA => "ONEZONE_IA",
            .GLACIER => "GLACIER",
            .GLACIER_IR => "GLACIER_IR",
            .REDUCED_REDUNDANCY => "REDUCED_REDUNDANCY",
            .OUTPOSTS => "OUTPOSTS",
            .DEEP_ARCHIVE => "DEEP_ARCHIVE",
            .SNOW => "SNOW",
        };
    }

    /// 是否属于归档冷存储层级（GLACIER / GLACIER_IR / DEEP_ARCHIVE）
    fn isArchiveTier(): bool {
        return switch (_) {
            .GLACIER, .GLACIER_IR, .DEEP_ARCHIVE => true,
            else => false,
        };
    }
}

fn isLowerAlnum(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

fn looksLikeIpv4(s: []const u8): bool {
    let parts: usize = 0;
    let i: usize = 0;
    while (parts < 4) {
        if (i >= s.len) {
            return false;
        }
        const seg_start = i;
        let val: u32 = 0;
        while (i < s.len && s[i] != '.') {
            const c = s[i];
            if (c < '0' || c > '9') {
                return false;
            }
            val = val * 10 + u32(c - '0');
            if (val > 255) {
                return false;
            }
            i += 1;
        }
        if (i == seg_start) {
            return false;
        }
        parts += 1;
        if (parts < 4) {
            if (i >= s.len || s[i] != '.') {
                return false;
            }
            i += 1;
        }
    }
    return i == s.len;
}

/// 校验 AWS S3 Bucket 名称是否合法（长度 3..63、仅小写字母/数字/连字符/点、首尾字母或数字、禁止 `..` 与 IPv4 格式）
export fn isValidBucketName(name: []const u8): bool {
    if (name.len < 3 || name.len > 63) {
        return false;
    }
    if (!isLowerAlnum(name[0]) || !isLowerAlnum(name[name.len - 1])) {
        return false;
    }
    let prev: u8 = 0;
    for (c of name) {
        const ok = isLowerAlnum(c) || c == '-' || c == '.';
        if (!ok) {
            return false;
        }
        if (c == '.' && prev == '.') {
            return false;
        }
        prev = c;
    }
    if (looksLikeIpv4(name)) {
        return false;
    }
    return true;
}

fn stripLeadingSlash(s: []const u8): []const u8 {
    if (s.len > 0 && s[0] == '/') {
        return s[1..];
    }
    return s;
}

/// 规范化 S3 对象 URI（零拷贝切片）
export struct S3Uri {
    bucket: []const u8;
    key: []const u8;
    region: ([]const u8)?;

    /// 解析 s3://、Virtual-Hosted-Style 与 Path-Style S3 URI
    fn parse(input: []const u8): S3Uri? {
        if (input.len == 0) {
            return null;
        }

        // 1. s3://<bucket>[/<key>]
        if (startsWith(input, "s3://")) {
            const rest = input[5..];
            if (rest.len == 0) {
                return null;
            }
            if (slash := findByte(rest, '/')) {
                const bucket = rest[0..slash];
                if (!isValidBucketName(bucket)) {
                    return null;
                }
                return S3Uri{
                    bucket: bucket,
                    key: rest[slash + 1..],
                    region: null,
                };
            }
            if (!isValidBucketName(rest)) {
                return null;
            }
            return S3Uri{
                bucket: rest,
                key: "",
                region: null,
            };
        }

        // 2. https:// 或 http:// AWS S3 Endpoint
        let rest: []const u8 = "";
        if (startsWith(input, "https://")) {
            rest = input[8..];
        } else if (startsWith(input, "http://")) {
            rest = input[7..];
        } else {
            return null;
        }

        let host: []const u8 = rest;
        let path: []const u8 = "";
        if (slash := findByte(rest, '/')) {
            host = rest[0..slash];
            path = rest[slash + 1..];
        }

        // 剥离可选查询串与片段
        if (q := findByte(path, '?')) {
            path = path[0..q];
        }
        if (h := findByte(path, '#')) {
            path = path[0..h];
        }

        const aws_suffix = ".amazonaws.com";
        if (!endsWith(host, aws_suffix)) {
            return null;
        }
        const host_prefix = host[0..host.len - aws_suffix.len];
        if (host_prefix.len == 0) {
            return null;
        }

        // 2a. Path-Style: s3.amazonaws.com/<bucket>/<key> 或 s3.<region>.amazonaws.com/<bucket>/<key>
        if (strEql(host_prefix, "s3") || startsWith(host_prefix, "s3.")) {
            let region: ([]const u8)? = null;
            if (startsWith(host_prefix, "s3.")) {
                const r = host_prefix[3..];
                if (r.len == 0) {
                    return null;
                }
                region = r;
            }
            const clean_path = stripLeadingSlash(path);
            if (clean_path.len == 0) {
                return null;
            }
            let bucket: []const u8 = clean_path;
            let key: []const u8 = "";
            if (slash := findByte(clean_path, '/')) {
                bucket = clean_path[0..slash];
                key = clean_path[slash + 1..];
            }
            if (!isValidBucketName(bucket)) {
                return null;
            }
            return S3Uri{
                bucket: bucket,
                key: key,
                region: region,
            };
        }

        // 2b. Virtual-Hosted-Style: <bucket>.s3.<region>.amazonaws.com/<key> 或 <bucket>.s3.amazonaws.com/<key>
        if (s3_idx := findSubstr(host_prefix, ".s3.")) {
            const bucket = host_prefix[0..s3_idx];
            const r = host_prefix[s3_idx + 4..];
            if (!isValidBucketName(bucket) || r.len == 0) {
                return null;
            }
            return S3Uri{
                bucket: bucket,
                key: stripLeadingSlash(path),
                region: r,
            };
        }
        if (endsWith(host_prefix, ".s3")) {
            const bucket = host_prefix[0..host_prefix.len - 3];
            if (!isValidBucketName(bucket)) {
                return null;
            }
            return S3Uri{
                bucket: bucket,
                key: stripLeadingSlash(path),
                region: null,
            };
        }

        return null;
    }

    /// 规范化输出 `s3://<bucket>/<key>`（零堆分配；若 key 为空则输出 `s3://<bucket>`）
    fn toCanonicalS3(out: []u8): ([]const u8)? {
        const prefix = "s3://";
        const extra_slash: usize = if (_.key.len > 0) 1 else 0;
        const needed = prefix.len + _.bucket.len + extra_slash + _.key.len;
        if (out.len < needed) {
            return null;
        }
        let pos: usize = 0;
        for (c of prefix) {
            out[pos] = c;
            pos += 1;
        }
        for (c of _.bucket) {
            out[pos] = c;
            pos += 1;
        }
        if (_.key.len > 0) {
            out[pos] = '/';
            pos += 1;
            for (c of _.key) {
                out[pos] = c;
                pos += 1;
            }
        }
        return out[0..pos];
    }
}
