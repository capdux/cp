// Source: Absorbed from /data/dev/bun/src/s3_signing/ | Synced: 2026-11-21T17:30:09Z
// ============================================================================
// lib/s3/index.cp — CP 工业级 S3 客户端引擎与 AWS SigV4 模块 (cap:s3)
// ============================================================================

export * from "./types";
export * from "./credentials";
export * from "./signing";
export * from "./simple_request";
export * from "./list_objects";
export * from "./multipart";
export * from "./download_stream";
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
