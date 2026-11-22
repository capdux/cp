// Source: Absorbed from /data/dev/bun/src/s3_signing/ | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/s3/index.cp — CP 工业级 S3 客户端引擎与 AWS SigV4 模块 (cap:s3)
// ============================================================================

export * from "./types";
export * from "./credentials";
export * from "./signing";
export * from "./simpleRequest";
export * from "./listObjects";
export * from "./multipart";
export * from "./downloadStream";
export * from "./client";

export {
    isValidBucketName,
    S3Uri,
} from "./meta.cp";
import s3meta from "./meta.cp";
pub const S3MetaAcl = s3meta.S3Acl;
pub const S3MetaStorageClass = s3meta.S3StorageClass;

export {
    S3SignError,
    SigV4Header,
    SigV4QueryParam,
    SigV4SignedResult,
    awsUriEncode,
    deriveSigningKey,
    signRequest,
    presignUrl,
} from "./sigv4.cp";
import sigv4 from "./sigv4.cp";
pub const SigV4Credentials = sigv4.S3Credentials;

export {
    S3_XML_ERR_MAX_FIELD_LEN,
    S3XmlErrorParseError,
    S3ErrorCategory,
    S3SignErrorInfo,
    getS3SignErrorInfo,
    classifyS3ErrorCode,
    isS3ErrorRetryable,
    S3XmlErrorRecord,
    parseS3XmlError,
    formatS3XmlError,
} from "./xmlErr.cp";

export {
    S3_PRESIGN_MIN_EXPIRES,
    S3_PRESIGN_MAX_EXPIRES,
    S3_PRESIGN_DEFAULT_EXPIRES,
    S3EndpointError,
    S3EndpointProvider,
    containsS3Crlf,
    validateS3HeaderValue,
    validateS3PresignExpiry,
    normalizeS3ObjectKey,
    encodeS3UriComponent,
    guessS3Bucket,
    guessS3Region,
    S3ParsedEndpoint,
} from "./endpoint.cp";
