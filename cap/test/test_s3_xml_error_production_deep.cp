// cap/test/test_s3_xml_error_production_deep.cp
// 深度生产级回归测试：cap:s3 XML 错误解析、分类与退避重试引擎 (lib/s3/xml_error.cp)

import { test } from "cap:test";
import {
    S3SignErrorCode,
    S3ErrorCategory,
    getS3SignErrorInfo,
    classifyS3ErrorCode,
    isS3ErrorRetryable,
    parseS3XmlError,
    formatS3XmlError,
} from "cap:s3";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("s3_xml_error: Bun ERR_S3_* sign error code & message mapping") {
    const e1 = getS3SignErrorInfo(S3SignErrorCode.MissingCredentials);
    if (!bytesEq(e1.code, "ERR_S3_MISSING_CREDENTIALS")) return err.MissingCredCodeMismatch;
    if (e1.message.len == 0) return err.MissingCredMsgEmpty;

    const e2 = getS3SignErrorInfo(S3SignErrorCode.InvalidMethod);
    if (!bytesEq(e2.code, "ERR_S3_INVALID_METHOD")) return err.InvalidMethodCodeMismatch;

    const e3 = getS3SignErrorInfo(S3SignErrorCode.InvalidPath);
    if (!bytesEq(e3.code, "ERR_S3_INVALID_PATH")) return err.InvalidPathCodeMismatch;

    const e4 = getS3SignErrorInfo(S3SignErrorCode.InvalidEndpoint);
    if (!bytesEq(e4.code, "ERR_S3_INVALID_ENDPOINT")) return err.InvalidEndpointCodeMismatch;

    const e5 = getS3SignErrorInfo(S3SignErrorCode.InvalidSessionToken);
    if (!bytesEq(e5.code, "ERR_S3_INVALID_SESSION_TOKEN")) return err.InvalidTokenCodeMismatch;

    const e6 = getS3SignErrorInfo(S3SignErrorCode.SignError);
    if (!bytesEq(e6.code, "ERR_S3_INVALID_SIGNATURE")) return err.InvalidSigCodeMismatch;
}

test("s3_xml_error: parse complete AWS S3 <Error> XML with all metadata fields") {
    const xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" ++
        "<Error>" ++
        "<Code>NoSuchKey</Code>" ++
        "<Message>The specified key does not exist.</Message>" ++
        "<Resource>/my-bucket/docs/readme.md</Resource>" ++
        "<BucketName>my-bucket</BucketName>" ++
        "<Key>docs/readme.md</Key>" ++
        "<RequestId>4442587FB7D0A2F9</RequestId>" ++
        "<HostId>9G4+HostIdToken==</HostId>" ++
        "</Error>";

    const parsed = parseS3XmlError(xml, 404)?;
    if (!bytesEq(parsed.code(), "NoSuchKey")) return err.CodeMismatch;
    if (!bytesEq(parsed.message(), "The specified key does not exist.")) return err.MessageMismatch;
    if (!bytesEq(parsed.resource(), "/my-bucket/docs/readme.md")) return err.ResourceMismatch;
    if (!bytesEq(parsed.bucketName(), "my-bucket")) return err.BucketMismatch;
    if (!bytesEq(parsed.key(), "docs/readme.md")) return err.KeyMismatch;
    if (!bytesEq(parsed.requestId(), "4442587FB7D0A2F9")) return err.RequestIdMismatch;
    if (!bytesEq(parsed.hostId(), "9G4+HostIdToken==")) return err.HostIdMismatch;
    if (!parsed.isNotFound() || parsed.isRetryable()) return err.StatusClassificationWrong;

    const base = parsed.toS3Error();
    if (!bytesEq(base.code, "NoSuchKey") || !bytesEq(base.message, "The specified key does not exist.")) {
        return err.BridgeToS3ErrorFailed;
    }
}

test("s3_xml_error: XML entity decoding, CDATA unwrapping & lossless round-trip") {
    const xml =
        "<Error>" ++
        "<Code>AccessDenied</Code>" ++
        "<Message>User &quot;alice&quot; &amp; &apos;bob&apos; &lt;denied&gt;</Message>" ++
        "<Key><![CDATA[dir/a&b<c>.txt]]></Key>" ++
        "<RequestId>REQ-99</RequestId>" ++
        "</Error>";

    const parsed = parseS3XmlError(xml, 403)?;
    if (!bytesEq(parsed.message(), "User \"alice\" & 'bob' <denied>")) return err.EntityDecodeFailed;
    if (!bytesEq(parsed.key(), "dir/a&b<c>.txt")) return err.CdataDecodeFailed;

    // 序列化为标准 XML 后再二次解析，验证 100% 无损 round-trip
    let buf: [1024]u8 = [0; 1024];
    const n = formatS3XmlError(parsed, buf[0..])?;
    const reparsed = parseS3XmlError(buf[0..n], 403)?;
    if (!bytesEq(reparsed.code(), parsed.code())) return err.RoundTripCodeMismatch;
    if (!bytesEq(reparsed.message(), parsed.message())) return err.RoundTripMsgMismatch;
    if (!bytesEq(reparsed.key(), parsed.key())) return err.RoundTripKeyMismatch;
    if (!bytesEq(reparsed.requestId(), parsed.requestId())) return err.RoundTripReqIdMismatch;
}

test("s3_xml_error: classification & retry policy matrix") {
    // 可重试错误：503 SlowDown, 429, 500 InternalError, 408 RequestTimeout, RequestTimeTooSkewed
    if (!isS3ErrorRetryable("SlowDown", 503)) return err.SlowDownShouldRetry;
    if (classifyS3ErrorCode("SlowDown", 503) != S3ErrorCategory.Throttled) return err.SlowDownCategoryWrong;
    if (!isS3ErrorRetryable("InternalError", 500)) return err.InternalErrorShouldRetry;
    if (!isS3ErrorRetryable("TooManyRequests", 429)) return err.Http429ShouldRetry;
    if (!isS3ErrorRetryable("RequestTimeout", 408)) return err.Http408ShouldRetry;
    if (!isS3ErrorRetryable("RequestTimeTooSkewed", 403)) return err.ClockSkewShouldRetry;

    // 不可重试错误：404 NoSuchKey, 403 AccessDenied, 403 SignatureDoesNotMatch, 400 InvalidBucketName, 409 Conflict
    if (isS3ErrorRetryable("NoSuchKey", 404)) return err.NoSuchKeyShouldNotRetry;
    if (isS3ErrorRetryable("AccessDenied", 403)) return err.AccessDeniedShouldNotRetry;
    if (isS3ErrorRetryable("SignatureDoesNotMatch", 403)) return err.SigMismatchShouldNotRetry;
    if (classifyS3ErrorCode("SignatureDoesNotMatch", 403) != S3ErrorCategory.AuthOrSignature) {
        return err.SigCategoryWrong;
    }
    if (isS3ErrorRetryable("InvalidBucketName", 400)) return err.InvalidBucketShouldNotRetry;
    if (classifyS3ErrorCode("BucketAlreadyExists", 409) != S3ErrorCategory.Conflict) {
        return err.ConflictCategoryWrong;
    }
}

test("s3_xml_error: fallback code derivation when <Code> tag is omitted") {
    const minimal = "<Error><Message>Gateway timeout</Message></Error>";
    const parsed = parseS3XmlError(minimal, 503)?;
    if (!bytesEq(parsed.code(), "ServiceUnavailable")) return err.FallbackCode503Wrong;
    if (!parsed.isRetryable()) return err.Fallback503ShouldRetry;

    const not_found = parseS3XmlError("<Error></Error>", 404)?;
    if (!bytesEq(not_found.code(), "NoSuchKey") || !not_found.isNotFound()) {
        return err.FallbackCode404Wrong;
    }
}

test("s3_xml_error: malformed XML & missing root rejection") {
    // 1. 缺少 <Error> 根标签
    let caught_missing_root = false;
    * = parseS3XmlError("<Response><Code>Err</Code></Response>", 500) catch {
        caught_missing_root = true;
    };
    if (!caught_missing_root) return err.ExpectedMissingErrorRoot;

    // 2. 截断未闭合的 </Error>
    let caught_unclosed_root = false;
    * = parseS3XmlError("<Error><Code>NoSuchKey</Code>", 404) catch {
        caught_unclosed_root = true;
    };
    if (!caught_unclosed_root) return err.ExpectedInvalidXmlRoot;

    // 3. 截断未闭合的子标签 </Code>
    let caught_unclosed_tag = false;
    * = parseS3XmlError("<Error><Code>NoSuchKey</Error>", 404) catch {
        caught_unclosed_tag = true;
    };
    if (!caught_unclosed_tag) return err.ExpectedInvalidXmlTag;
}

test("s3_xml_error: field overflow & serialization BufferTooSmall guards") {
    // 1. 超过 S3_XML_ERR_MAX_FIELD_LEN (256) 的超长字段被安全拦截
    let huge_xml: [320]u8 = ['a'; 320];
    const prefix = "<Error><Code>";
    const suffix = "</Code></Error>";
    let i: usize = 0;
    while (i < prefix.len) {
        huge_xml[i] = prefix[i];
        i += 1;
    }
    let j: usize = 0;
    while (j < suffix.len) {
        huge_xml[320 - suffix.len + j] = suffix[j];
        j += 1;
    }
    let caught_long = false;
    * = parseS3XmlError(huge_xml[0..], 400) catch {
        caught_long = true;
    };
    if (!caught_long) return err.ExpectedFieldTooLong;

    // 2. formatS3XmlError 输出缓冲过小安全返回 BufferTooSmall
    const parsed = parseS3XmlError("<Error><Code>NoSuchKey</Code><Message>missing</Message></Error>", 404)?;
    let tiny: [16]u8 = [0; 16];
    let caught_small = false;
    * = formatS3XmlError(parsed, tiny[0..]) catch {
        caught_small = true;
    };
    if (!caught_small) return err.ExpectedBufferTooSmall;
}
