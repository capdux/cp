// Source: Absorbed from /data/dev/bun/src/s3_signing/error.cp | Synced: 2026-11-22T09:18:00Z
// Ported from Bun: /data/dev/bun/src/s3_signing/error.cp & /data/dev/bun/docs/runtime/s3.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation AWS S3 XML Error Parser, Error Classification & Retry Policy Engine:
// - Maps Bun S3 signing error codes (ERR_S3_MISSING_CREDENTIALS .. ERR_S3_INVALID_SESSION_TOKEN)
// - Parses AWS S3 / R2 / MinIO <Error> XML responses with entity decoding & CDATA unwrapping
// - Classifies error categories and computes safe retry eligibility (429/5xx/SlowDown/RequestTimeout)
// - Supports lossless XML escaping and serialization round-trips with zero heap allocation.

import { S3Error, S3SignErrorCode } from "./types.cp";

/// 单个 S3 XML 错误字段的最大字节容量
pub const S3_XML_ERR_MAX_FIELD_LEN: usize = 256;

/// S3 XML 错误解析与序列化错误集
pub err S3XmlErrorParseError {
    InvalidXml,
    MissingErrorRoot,
    FieldTooLong,
    BufferTooSmall,
}

/// S3 错误语义分类
pub enum S3ErrorCategory: u8 {
    Unknown = 0,
    MissingCredentials = 1,
    InvalidRequest = 2,
    AuthOrSignature = 3,
    AccessDenied = 4,
    NotFound = 5,
    Conflict = 6,
    PreconditionFailed = 7,
    Throttled = 8,
    TransientServer = 9,
}

/// Bun S3 签名错误码与可读描述（对齐 `bun/src/s3_signing/error.cp` `ErrorCodeAndMessage`）
pub struct S3SignErrorInfo {
    code: []const u8 = "",
    message: []const u8 = "",
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// 按 Bun S3 规范返回精确的 `ERR_S3_*` 错误码与提示文案
pub fn getS3SignErrorInfo(kind: S3SignErrorCode): S3SignErrorInfo {
    if (kind == .MissingCredentials) {
        return S3SignErrorInfo{
            code: "ERR_S3_MISSING_CREDENTIALS",
            message: "Missing S3 credentials. 'accessKeyId', 'secretAccessKey', 'bucket', and 'endpoint' are required",
        };
    }
    if (kind == .InvalidMethod) {
        return S3SignErrorInfo{
            code: "ERR_S3_INVALID_METHOD",
            message: "Method must be GET, PUT, DELETE or HEAD when using s3:// protocol",
        };
    }
    if (kind == .InvalidPath) {
        return S3SignErrorInfo{
            code: "ERR_S3_INVALID_PATH",
            message: "Invalid S3 bucket, key combination",
        };
    }
    if (kind == .InvalidEndpoint) {
        return S3SignErrorInfo{
            code: "ERR_S3_INVALID_ENDPOINT",
            message: "Invalid S3 endpoint",
        };
    }
    if (kind == .InvalidSessionToken) {
        return S3SignErrorInfo{
            code: "ERR_S3_INVALID_SESSION_TOKEN",
            message: "Invalid session token",
        };
    }
    return S3SignErrorInfo{
        code: "ERR_S3_INVALID_SIGNATURE",
        message: "Failed to retrieve S3 content. Are the credentials correct?",
    };
}

/// 根据 S3 XML `<Code>` 与 HTTP 状态码进行语义分类
pub fn classifyS3ErrorCode(code: []const u8, http_status: u16): S3ErrorCategory {
    if (strEq(code, "ERR_S3_MISSING_CREDENTIALS")) return .MissingCredentials;
    if (strEq(code, "NoSuchKey") || strEq(code, "NoSuchBucket") ||
        strEq(code, "NoSuchUpload") || strEq(code, "NoSuchVersion") ||
        strEq(code, "NotFound"))
    {
        return .NotFound;
    }
    if (strEq(code, "AccessDenied") || strEq(code, "AccountProblem") ||
        strEq(code, "AllAccessDisabled") || strEq(code, "Forbidden"))
    {
        return .AccessDenied;
    }
    if (strEq(code, "SignatureDoesNotMatch") || strEq(code, "InvalidAccessKeyId") ||
        strEq(code, "ExpiredToken") || strEq(code, "InvalidToken") ||
        strEq(code, "ERR_S3_INVALID_SIGNATURE") || strEq(code, "ERR_S3_INVALID_SESSION_TOKEN"))
    {
        return .AuthOrSignature;
    }
    if (strEq(code, "SlowDown") || strEq(code, "Throttling") ||
        strEq(code, "ThrottlingException") || strEq(code, "RequestLimitExceeded"))
    {
        return .Throttled;
    }
    if (strEq(code, "InternalError") || strEq(code, "ServiceUnavailable") ||
        strEq(code, "RequestTimeout") || strEq(code, "OperationAborted"))
    {
        return .TransientServer;
    }
    if (strEq(code, "BucketAlreadyExists") || strEq(code, "BucketAlreadyOwnedByYou") ||
        strEq(code, "BucketNotEmpty") || strEq(code, "Conflict"))
    {
        return .Conflict;
    }
    if (strEq(code, "PreconditionFailed") || strEq(code, "InvalidRange")) {
        return .PreconditionFailed;
    }
    if (strEq(code, "InvalidArgument") || strEq(code, "InvalidBucketName") ||
        strEq(code, "InvalidURI") || strEq(code, "BadDigest") ||
        strEq(code, "EntityTooLarge") || strEq(code, "EntityTooSmall") ||
        strEq(code, "MalformedXML") || strEq(code, "ERR_S3_INVALID_METHOD") ||
        strEq(code, "ERR_S3_INVALID_PATH") || strEq(code, "ERR_S3_INVALID_ENDPOINT"))
    {
        return .InvalidRequest;
    }

    if (http_status == 404) return .NotFound;
    if (http_status == 403) return .AccessDenied;
    if (http_status == 401) return .AuthOrSignature;
    if (http_status == 409) return .Conflict;
    if (http_status == 412 || http_status == 416) return .PreconditionFailed;
    if (http_status == 429) return .Throttled;
    if (http_status == 408 || http_status >= 500) return .TransientServer;
    if (http_status >= 400 && http_status < 500) return .InvalidRequest;
    return .Unknown;
}

/// 判断 S3 错误是否可安全退避重试（对齐 Bun S3 `retry` 策略与 AWS SDK 重试规范）
pub fn isS3ErrorRetryable(code: []const u8, http_status: u16): bool {
    if (http_status == 408 || http_status == 429 ||
        http_status == 500 || http_status == 502 ||
        http_status == 503 || http_status == 504)
    {
        return true;
    }
    const cat = classifyS3ErrorCode(code, http_status);
    if (cat == .Throttled || cat == .TransientServer) return true;
    if (strEq(code, "RequestTimeTooSkewed") || strEq(code, "PriorRequestNotComplete")) {
        return true;
    }
    return false;
}

/// 解析后的 S3 服务端 XML 错误记录（零动态堆分配）
pub struct S3XmlErrorRecord {
    http_status: u16 = 0,
    code_buf: [S3_XML_ERR_MAX_FIELD_LEN]u8 = [0; S3_XML_ERR_MAX_FIELD_LEN],
    code_len: usize = 0,
    message_buf: [S3_XML_ERR_MAX_FIELD_LEN]u8 = [0; S3_XML_ERR_MAX_FIELD_LEN],
    message_len: usize = 0,
    resource_buf: [S3_XML_ERR_MAX_FIELD_LEN]u8 = [0; S3_XML_ERR_MAX_FIELD_LEN],
    resource_len: usize = 0,
    request_id_buf: [S3_XML_ERR_MAX_FIELD_LEN]u8 = [0; S3_XML_ERR_MAX_FIELD_LEN],
    request_id_len: usize = 0,
    host_id_buf: [S3_XML_ERR_MAX_FIELD_LEN]u8 = [0; S3_XML_ERR_MAX_FIELD_LEN],
    host_id_len: usize = 0,
    bucket_buf: [S3_XML_ERR_MAX_FIELD_LEN]u8 = [0; S3_XML_ERR_MAX_FIELD_LEN],
    bucket_len: usize = 0,
    key_buf: [S3_XML_ERR_MAX_FIELD_LEN]u8 = [0; S3_XML_ERR_MAX_FIELD_LEN],
    key_len: usize = 0,

    fn code(): []const u8 {
        return _.code_buf[0.._.code_len];
    }

    fn message(): []const u8 {
        return _.message_buf[0.._.message_len];
    }

    fn resource(): []const u8 {
        return _.resource_buf[0.._.resource_len];
    }

    fn requestId(): []const u8 {
        return _.request_id_buf[0.._.request_id_len];
    }

    fn hostId(): []const u8 {
        return _.host_id_buf[0.._.host_id_len];
    }

    fn bucketName(): []const u8 {
        return _.bucket_buf[0.._.bucket_len];
    }

    fn key(): []const u8 {
        return _.key_buf[0.._.key_len];
    }

    fn category(): S3ErrorCategory {
        return classifyS3ErrorCode(_.code(), _.http_status);
    }

    fn isRetryable(): bool {
        return isS3ErrorRetryable(_.code(), _.http_status);
    }

    fn isNotFound(): bool {
        return _.category() == .NotFound;
    }

    /// 桥接转换为 `cap:s3` 基础 `S3Error` 视图（借用当前对象内部缓冲）
    fn toS3Error(): S3Error {
        return S3Error{
            code: _.code(),
            message: _.message(),
        };
    }
}

fn findSubstr(haystack: []const u8, start: usize, needle: []const u8): usize? {
    if (needle.len == 0) return start;
    if (start + needle.len > haystack.len) return null;
    let i: usize = start;
    while (i + needle.len <= haystack.len) {
        if (strEq(haystack[i..i + needle.len], needle)) return i;
        i += 1;
    }
    return null;
}

fn decodeXmlContentInto(raw: []const u8, dst: *[S3_XML_ERR_MAX_FIELD_LEN]u8): usize! {
    let slice = raw;
    // 解包 <![CDATA[...]]>
    if (slice.len >= 12 &&
        strEq(slice[0..9], "<![CDATA[") &&
        strEq(slice[slice.len - 3..slice.len], "]]>"))
    {
        slice = slice[9..slice.len - 3];
        if (slice.len > S3_XML_ERR_MAX_FIELD_LEN) return err.FieldTooLong;
        let i: usize = 0;
        while (i < slice.len) {
            dst[i] = slice[i];
            i += 1;
        }
        return slice.len;
    }

    let out_len: usize = 0;
    let i: usize = 0;
    while (i < slice.len) {
        if (out_len >= S3_XML_ERR_MAX_FIELD_LEN) return err.FieldTooLong;
        if (slice[i] == '&') {
            if (i + 5 <= slice.len && strEq(slice[i..i + 5], "&amp;")) {
                dst[out_len] = '&';
                out_len += 1;
                i += 5;
                continue;
            }
            if (i + 4 <= slice.len && strEq(slice[i..i + 4], "&lt;")) {
                dst[out_len] = '<';
                out_len += 1;
                i += 4;
                continue;
            }
            if (i + 4 <= slice.len && strEq(slice[i..i + 4], "&gt;")) {
                dst[out_len] = '>';
                out_len += 1;
                i += 4;
                continue;
            }
            if (i + 6 <= slice.len && strEq(slice[i..i + 6], "&quot;")) {
                dst[out_len] = '"';
                out_len += 1;
                i += 6;
                continue;
            }
            if (i + 6 <= slice.len && strEq(slice[i..i + 6], "&apos;")) {
                dst[out_len] = '\'';
                out_len += 1;
                i += 6;
                continue;
            }
        }
        dst[out_len] = slice[i];
        out_len += 1;
        i += 1;
    }
    return out_len;
}

struct TagExtractResult {
    found: bool = false,
    len: usize = 0,
}

fn extractTagValue(
    body: []const u8,
    open_tag: []const u8,
    close_tag: []const u8,
    dst: *[S3_XML_ERR_MAX_FIELD_LEN]u8,
): TagExtractResult! {
    const open_pos = findSubstr(body, 0, open_tag) orelse return TagExtractResult{};
    const val_start = open_pos + open_tag.len;
    const close_pos = findSubstr(body, val_start, close_tag) orelse return err.InvalidXml;
    const decoded_len = decodeXmlContentInto(body[val_start..close_pos], dst)?;
    return TagExtractResult{ found: true, len: decoded_len };
}

fn copyLiteralInto(src: []const u8, dst: *[S3_XML_ERR_MAX_FIELD_LEN]u8): usize {
    let n: usize = src.len;
    if (n > S3_XML_ERR_MAX_FIELD_LEN) {
        n = S3_XML_ERR_MAX_FIELD_LEN;
    }
    let i: usize = 0;
    while (i < n) {
        dst[i] = src[i];
        i += 1;
    }
    return n;
}

fn defaultCodeForStatus(http_status: u16): []const u8 {
    if (http_status == 400) return "BadRequest";
    if (http_status == 401) return "Unauthorized";
    if (http_status == 403) return "AccessDenied";
    if (http_status == 404) return "NoSuchKey";
    if (http_status == 408) return "RequestTimeout";
    if (http_status == 409) return "Conflict";
    if (http_status == 412) return "PreconditionFailed";
    if (http_status == 429) return "SlowDown";
    if (http_status == 503) return "ServiceUnavailable";
    if (http_status >= 500) return "InternalError";
    return "UnknownError";
}

/// 解析 AWS S3 / R2 / MinIO `<Error>...</Error>` XML 响应体（零动态堆分配）
pub fn parseS3XmlError(xml_input: []const u8, http_status: u16): S3XmlErrorRecord! {
    let root_start: usize = 0;
    if (rs := findSubstr(xml_input, 0, "<Error>")) {
        root_start = rs;
    } else {
        return err.MissingErrorRoot;
    }
    let root_end: usize = 0;
    if (re := findSubstr(xml_input, root_start + 7, "</Error>")) {
        root_end = re;
    } else {
        return err.InvalidXml;
    }
    const body = xml_input[root_start + 7..root_end];

    let parsed = S3XmlErrorRecord{
        http_status: http_status,
    };

    const r_code = extractTagValue(body, "<Code>", "</Code>", &parsed.code_buf)?;
    if (r_code.found) {
        parsed.code_len = r_code.len;
    } else {
        parsed.code_len = copyLiteralInto(defaultCodeForStatus(http_status), &parsed.code_buf);
    }

    const r_msg = extractTagValue(body, "<Message>", "</Message>", &parsed.message_buf)?;
    if (r_msg.found) parsed.message_len = r_msg.len;

    const r_res = extractTagValue(body, "<Resource>", "</Resource>", &parsed.resource_buf)?;
    if (r_res.found) parsed.resource_len = r_res.len;

    const r_req = extractTagValue(body, "<RequestId>", "</RequestId>", &parsed.request_id_buf)?;
    if (r_req.found) parsed.request_id_len = r_req.len;

    const r_host = extractTagValue(body, "<HostId>", "</HostId>", &parsed.host_id_buf)?;
    if (r_host.found) parsed.host_id_len = r_host.len;

    const r_bucket = extractTagValue(body, "<BucketName>", "</BucketName>", &parsed.bucket_buf)?;
    if (r_bucket.found) parsed.bucket_len = r_bucket.len;

    const r_key = extractTagValue(body, "<Key>", "</Key>", &parsed.key_buf)?;
    if (r_key.found) parsed.key_len = r_key.len;

    return parsed;
}

fn appendRaw(out: []u8, pos: *usize, data: []const u8): ! {
    if (pos.* + data.len > out.len) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < data.len) {
        out[pos.* + i] = data[i];
        i += 1;
    }
    pos.* += data.len;
}

fn appendXmlEscaped(out: []u8, pos: *usize, text: []const u8): ! {
    let i: usize = 0;
    while (i < text.len) {
        const ch = text[i];
        if (ch == '&') {
            appendRaw(out, pos, "&amp;")?;
        } else if (ch == '<') {
            appendRaw(out, pos, "&lt;")?;
        } else if (ch == '>') {
            appendRaw(out, pos, "&gt;")?;
        } else if (ch == '"') {
            appendRaw(out, pos, "&quot;")?;
        } else if (ch == '\'') {
            appendRaw(out, pos, "&apos;")?;
        } else {
            if (pos.* >= out.len) return err.BufferTooSmall;
            out[pos.*] = ch;
            pos.* += 1;
        }
        i += 1;
    }
}

fn appendOptionalTag(
    out: []u8,
    pos: *usize,
    open_tag: []const u8,
    close_tag: []const u8,
    val: []const u8,
): ! {
    if (val.len == 0) return;
    appendRaw(out, pos, open_tag)?;
    appendXmlEscaped(out, pos, val)?;
    appendRaw(out, pos, close_tag)?;
}

/// 将 `S3XmlErrorRecord` 安全转义并序列化为标准 AWS S3 `<Error>...</Error>` XML（支持无损 round-trip）
pub fn formatS3XmlError(info: S3XmlErrorRecord, out: []u8): usize! {
    let pos: usize = 0;
    appendRaw(out, &pos, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Error>")?;
    appendOptionalTag(out, &pos, "<Code>", "</Code>", info.code())?;
    appendOptionalTag(out, &pos, "<Message>", "</Message>", info.message())?;
    appendOptionalTag(out, &pos, "<Resource>", "</Resource>", info.resource())?;
    appendOptionalTag(out, &pos, "<BucketName>", "</BucketName>", info.bucketName())?;
    appendOptionalTag(out, &pos, "<Key>", "</Key>", info.key())?;
    appendOptionalTag(out, &pos, "<RequestId>", "</RequestId>", info.requestId())?;
    appendOptionalTag(out, &pos, "<HostId>", "</HostId>", info.hostId())?;
    appendRaw(out, &pos, "</Error>")?;
    return pos;
}
