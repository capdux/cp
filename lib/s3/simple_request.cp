// Source: /data/dev/bun/src/s3_signing/
// ============================================================================
// lib/s3/simple_request.cp — S3 基础 HTTP 请求、签名构建、预签名 URL 与结果模型
// 吸收自 Bun src/runtime/webcore/s3/simple_request.cp
// ============================================================================

import { S3Acl, S3StorageClass, S3Error, S3SignErrorCode } from "./types.cp";
import { S3Credentials } from "./credentials.cp";
import { SigV4Signer } from "./signing.cp";
const std = @import("std");

pub struct S3StatSuccess {
    size: usize = 0,
    etag: string = "",
    last_modified: string = "",
    content_type: string = "",
}

pub struct S3StatResult {
    is_success: bool = false,
    is_not_found: bool = false,
    stat: S3StatSuccess = S3StatSuccess{},
    err: S3Error? = null,

    pub static fn success(stat: S3StatSuccess): S3StatResult {
        return S3StatResult{
            is_success: true,
            is_not_found: false,
            stat: stat,
            err: null,
        };
    }

    pub static fn notFound(e: S3Error): S3StatResult {
        return S3StatResult{
            is_success: false,
            is_not_found: true,
            stat: S3StatSuccess{},
            err: e,
        };
    }

    pub static fn failure(e: S3Error): S3StatResult {
        return S3StatResult{
            is_success: false,
            is_not_found: false,
            stat: S3StatSuccess{},
            err: e,
        };
    }
}

pub struct S3DownloadSuccess {
    etag: string = "",
    body: []const u8 = "",
    content_type: string = "",
}

pub struct S3DownloadResult {
    is_success: bool = false,
    is_not_found: bool = false,
    data: S3DownloadSuccess = S3DownloadSuccess{},
    err: S3Error? = null,

    pub static fn success(data: S3DownloadSuccess): S3DownloadResult {
        return S3DownloadResult{
            is_success: true,
            is_not_found: false,
            data: data,
            err: null,
        };
    }

    pub static fn notFound(e: S3Error): S3DownloadResult {
        return S3DownloadResult{
            is_success: false,
            is_not_found: true,
            data: S3DownloadSuccess{},
            err: e,
        };
    }

    pub static fn failure(e: S3Error): S3DownloadResult {
        return S3DownloadResult{
            is_success: false,
            is_not_found: false,
            data: S3DownloadSuccess{},
            err: e,
        };
    }
}

pub struct S3UploadResult {
    is_success: bool = false,
    etag: string = "",
    err: S3Error? = null,

    pub static fn success(etag: string): S3UploadResult {
        return S3UploadResult{
            is_success: true,
            etag: etag,
            err: null,
        };
    }

    pub static fn failure(e: S3Error): S3UploadResult {
        return S3UploadResult{
            is_success: false,
            etag: "",
            err: e,
        };
    }
}

pub struct S3DeleteResult {
    is_success: bool = false,
    err: S3Error? = null,

    pub static fn success(): S3DeleteResult {
        return S3DeleteResult{
            is_success: true,
            err: null,
        };
    }

    pub static fn failure(e: S3Error): S3DeleteResult {
        return S3DeleteResult{
            is_success: false,
            err: e,
        };
    }
}

pub struct S3PartResult {
    is_success: bool = false,
    etag: string = "",
    err: S3Error? = null,

    pub static fn success(etag: string): S3PartResult {
        return S3PartResult{
            is_success: true,
            etag: etag,
            err: null,
        };
    }

    pub static fn failure(e: S3Error): S3PartResult {
        return S3PartResult{
            is_success: false,
            etag: "",
            err: e,
        };
    }
}

pub struct S3SimpleRequestOptions {
    path: string = "",
    method: string = "GET",
    body: string = "",
    range: string? = null,
    content_type: string? = null,
    content_disposition: string? = null,
    content_encoding: string? = null,
    acl: S3Acl? = null,
    storage_class: S3StorageClass? = null,
    search_params: string? = null,
    request_payer: bool = false,
}

pub struct S3SignedRequest {
    method: string = "GET",
    url: string = "",
    host: string = "",
    canonical_uri: string = "",
    date_yyyymmdd: string = "",
    amz_date: string = "",
    content_sha256: string = "",
    auth_header: string = "",
}

pub struct S3RequestBuilder {
    /// 构造标准 AWS SigV4 签名请求
    pub static fn buildSignedRequest(
        creds: *const S3Credentials,
        opts: S3SimpleRequestOptions,
        date_yyyymmdd: string,
        amz_date: string,
        url_buf: []u8,
        auth_buf: []u8,
    ): S3SignedRequest! {
        let host_buf: [128]u8 = undefined;
        let host_len = creds.formatHost(&host_buf);
        if (host_len == 0) return err.InvalidHost;
        const host_str = host_buf[0..host_len];

        let uri_buf: [256]u8 = undefined;
        let uri_len = creds.formatCanonicalUri(opts.path, &uri_buf);
        if (uri_len == 0) return err.InvalidUri;
        const uri_str = uri_buf[0..uri_len];

        // 构造完整请求 URL
        const proto = if (creds.insecure_http) "http://" else "https://";
        let u_pos: usize = 0;
        @memcpy(url_buf[0..proto.len], proto);
        u_pos += proto.len;
        @memcpy(url_buf[u_pos .. u_pos + host_len], host_str);
        u_pos += host_len;
        @memcpy(url_buf[u_pos .. u_pos + uri_len], uri_str);
        u_pos += uri_len;

        if (sp := opts.search_params) {
            if (sp.len > 0) {
                if (sp[0] != '?') {
                    url_buf[u_pos] = '?';
                    u_pos += 1;
                }
                @memcpy(url_buf[u_pos .. u_pos + sp.len], sp);
                u_pos += sp.len;
            }
        }
        const full_url = url_buf[0..u_pos];

        // 计算 Payload SHA-256 并持久化于 auth_buf[0..64]
        if (auth_buf.len < 256) return err.BufferTooSmall;
        let sha_len = SigV4Signer.sha256Hex(opts.body, auth_buf[0..64]);
        const payload_hash = auth_buf[0..sha_len];

        // 构造 Canonical Headers 与 SignedHeaders
        // host, x-amz-content-sha256, x-amz-date
        let canonical_buf: [1024]u8 = undefined;
        let c_pos: usize = 0;

        // Method\n
        @memcpy(canonical_buf[c_pos .. c_pos + opts.method.len], opts.method);
        c_pos += opts.method.len;
        canonical_buf[c_pos] = '\n';
        c_pos += 1;

        // CanonicalURI\n
        @memcpy(canonical_buf[c_pos .. c_pos + uri_len], uri_str);
        c_pos += uri_len;
        canonical_buf[c_pos] = '\n';
        c_pos += 1;

        // CanonicalQueryString\n
        if (sp := opts.search_params) {
            let clean_sp = if (sp.len > 0 && sp[0] == '?') sp[1..] else sp;
            @memcpy(canonical_buf[c_pos .. c_pos + clean_sp.len], clean_sp);
            c_pos += clean_sp.len;
        }
        canonical_buf[c_pos] = '\n';
        c_pos += 1;

        // Headers (严格字母序): host, x-amz-content-sha256, x-amz-date
        // host:...
        const h_prefix = "host:";
        @memcpy(canonical_buf[c_pos .. c_pos + h_prefix.len], h_prefix);
        c_pos += h_prefix.len;
        @memcpy(canonical_buf[c_pos .. c_pos + host_len], host_str);
        c_pos += host_len;
        canonical_buf[c_pos] = '\n';
        c_pos += 1;

        // x-amz-content-sha256:...
        const sha_hdr = "x-amz-content-sha256:";
        @memcpy(canonical_buf[c_pos .. c_pos + sha_hdr.len], sha_hdr);
        c_pos += sha_hdr.len;
        @memcpy(canonical_buf[c_pos .. c_pos + sha_len], payload_hash);
        c_pos += sha_len;
        canonical_buf[c_pos] = '\n';
        c_pos += 1;

        // x-amz-date:...
        const date_hdr = "x-amz-date:";
        @memcpy(canonical_buf[c_pos .. c_pos + date_hdr.len], date_hdr);
        c_pos += date_hdr.len;
        @memcpy(canonical_buf[c_pos .. c_pos + amz_date.len], amz_date);
        c_pos += amz_date.len;
        canonical_buf[c_pos] = '\n';
        c_pos += 1;

        // Blank line separating headers & signed headers
        canonical_buf[c_pos] = '\n';
        c_pos += 1;

        // SignedHeaders
        const signed_headers = "host;x-amz-content-sha256;x-amz-date";
        @memcpy(canonical_buf[c_pos .. c_pos + signed_headers.len], signed_headers);
        c_pos += signed_headers.len;
        canonical_buf[c_pos] = '\n';
        c_pos += 1;

        // Payload Hash
        @memcpy(canonical_buf[c_pos .. c_pos + sha_len], payload_hash);
        c_pos += sha_len;

        const canonical_req = canonical_buf[0..c_pos];

        // 计算 SigV4 Signature
        let sig_buf: [64]u8 = undefined;
        let sig_len = SigV4Signer.signS3CanonicalRequest(
            creds,
            date_yyyymmdd,
            amz_date,
            canonical_req,
            &sig_buf,
        );
        const sig_str = sig_buf[0..sig_len];

        // 构造 Authorization 报头（写入 auth_buf[64..]）
        // AWS4-HMAC-SHA256 Credential=<ak>/<scope>, SignedHeaders=<signed_headers>, Signature=<sig>
        let a_pos: usize = 64;
        const auth_prefix = "AWS4-HMAC-SHA256 Credential=";
        @memcpy(auth_buf[a_pos .. a_pos + auth_prefix.len], auth_prefix);
        a_pos += auth_prefix.len;

        @memcpy(auth_buf[a_pos .. a_pos + creds.access_key_id.len], creds.access_key_id);
        a_pos += creds.access_key_id.len;
        auth_buf[a_pos] = '/';
        a_pos += 1;

        let scope_len = SigV4Signer.formatCredentialScope(
            date_yyyymmdd,
            creds.effectiveRegion(),
            "s3",
            auth_buf[a_pos..],
        );
        a_pos += scope_len;

        const sh_part = ", SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature=";
        @memcpy(auth_buf[a_pos .. a_pos + sh_part.len], sh_part);
        a_pos += sh_part.len;

        @memcpy(auth_buf[a_pos .. a_pos + sig_len], sig_str);
        a_pos += sig_len;

        const auth_header = auth_buf[64..a_pos];

        return S3SignedRequest{
            method: opts.method,
            url: full_url,
            host: url_buf[proto.len .. proto.len + host_len],
            canonical_uri: url_buf[proto.len + host_len .. proto.len + host_len + uri_len],
            date_yyyymmdd: date_yyyymmdd,
            amz_date: amz_date,
            content_sha256: payload_hash,
            auth_header: auth_header,
        };
    }

    /// 构造 AWS SigV4 预签名 URL (Presigned URL, Query-based Authentication)
    /// 采用 `UNSIGNED-PAYLOAD` 与 `X-Amz-SignedHeaders=host`，将完整 URL 写入 `url_buf` 并返回切片
    pub static fn buildPresignedUrl(
        creds: *const S3Credentials,
        method: string,
        path: string,
        date_yyyymmdd: string,
        amz_date: string,
        expires_seconds: usize,
        url_buf: []u8,
    ): string! {
        let host_buf: [128]u8 = undefined;
        let host_len = creds.formatHost(&host_buf);
        if (host_len == 0) return err.InvalidHost;
        const host_str = host_buf[0..host_len];

        let uri_buf: [256]u8 = undefined;
        let uri_len = creds.formatCanonicalUri(path, &uri_buf);
        if (uri_len == 0) return err.InvalidUri;
        const uri_str = uri_buf[0..uri_len];

        // Format expires_seconds to decimal ASCII
        let exp_buf: [24]u8 = undefined;
        let exp_len: usize = 0;
        if (expires_seconds == 0) {
            exp_buf[0] = '0';
            exp_len = 1;
        } else {
            let tmp: [24]u8 = undefined;
            let cnt: usize = 0;
            let v = expires_seconds;
            while (v > 0) {
                tmp[cnt] = (48 as u8) + ((v % 10) as u8);
                v /= 10;
                cnt += 1;
            }
            while (cnt > 0) {
                cnt -= 1;
                exp_buf[exp_len] = tmp[cnt];
                exp_len += 1;
            }
        }
        const exp_str = exp_buf[0..exp_len];
        const region = creds.effectiveRegion();

        // Build Canonical Query String in alphabetical order:
        // X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=<ak>%2F<date>%2F<region>%2Fs3%2Faws4_request&X-Amz-Date=<amz_date>&X-Amz-Expires=<expires>&X-Amz-SignedHeaders=host
        let query_buf: [512]u8 = undefined;
        let q_pos: usize = 0;
        const p1 = "X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=";
        @memcpy(query_buf[q_pos .. q_pos + p1.len], p1);
        q_pos += p1.len;
        @memcpy(query_buf[q_pos .. q_pos + creds.access_key_id.len], creds.access_key_id);
        q_pos += creds.access_key_id.len;
        @memcpy(query_buf[q_pos .. q_pos + 3], "%2F");
        q_pos += 3;
        @memcpy(query_buf[q_pos .. q_pos + date_yyyymmdd.len], date_yyyymmdd);
        q_pos += date_yyyymmdd.len;
        @memcpy(query_buf[q_pos .. q_pos + 3], "%2F");
        q_pos += 3;
        @memcpy(query_buf[q_pos .. q_pos + region.len], region);
        q_pos += region.len;
        const p2 = "%2Fs3%2Faws4_request&X-Amz-Date=";
        @memcpy(query_buf[q_pos .. q_pos + p2.len], p2);
        q_pos += p2.len;
        @memcpy(query_buf[q_pos .. q_pos + amz_date.len], amz_date);
        q_pos += amz_date.len;
        const p3 = "&X-Amz-Expires=";
        @memcpy(query_buf[q_pos .. q_pos + p3.len], p3);
        q_pos += p3.len;
        @memcpy(query_buf[q_pos .. q_pos + exp_len], exp_str);
        q_pos += exp_len;
        const p4 = "&X-Amz-SignedHeaders=host";
        @memcpy(query_buf[q_pos .. q_pos + p4.len], p4);
        q_pos += p4.len;
        const canonical_query = query_buf[0..q_pos];

        // Build Canonical Request:
        // <Method>\n<CanonicalURI>\n<CanonicalQueryString>\nhost:<host>\n\nhost\nUNSIGNED-PAYLOAD
        let canonical_buf: [1024]u8 = undefined;
        let c_pos: usize = 0;
        @memcpy(canonical_buf[c_pos .. c_pos + method.len], method);
        c_pos += method.len;
        canonical_buf[c_pos] = '\n';
        c_pos += 1;
        @memcpy(canonical_buf[c_pos .. c_pos + uri_len], uri_str);
        c_pos += uri_len;
        canonical_buf[c_pos] = '\n';
        c_pos += 1;
        @memcpy(canonical_buf[c_pos .. c_pos + q_pos], canonical_query);
        c_pos += q_pos;
        const h_part = "\nhost:";
        @memcpy(canonical_buf[c_pos .. c_pos + h_part.len], h_part);
        c_pos += h_part.len;
        @memcpy(canonical_buf[c_pos .. c_pos + host_len], host_str);
        c_pos += host_len;
        const tail_part = "\n\nhost\nUNSIGNED-PAYLOAD";
        @memcpy(canonical_buf[c_pos .. c_pos + tail_part.len], tail_part);
        c_pos += tail_part.len;

        let sig_buf: [64]u8 = undefined;
        let sig_len = SigV4Signer.signS3CanonicalRequest(
            creds,
            date_yyyymmdd,
            amz_date,
            canonical_buf[0..c_pos],
            &sig_buf,
        );
        if (sig_len != 64) return err.SignError;

        // Assemble final presigned URL into url_buf
        const proto = if (creds.insecure_http) "http://" else "https://";
        const sig_param = "&X-Amz-Signature=";
        const total_needed = proto.len + host_len + uri_len + 1 + q_pos + sig_param.len + 64;
        if (url_buf.len < total_needed) return err.BufferTooSmall;

        let u_pos: usize = 0;
        @memcpy(url_buf[u_pos .. u_pos + proto.len], proto);
        u_pos += proto.len;
        @memcpy(url_buf[u_pos .. u_pos + host_len], host_str);
        u_pos += host_len;
        @memcpy(url_buf[u_pos .. u_pos + uri_len], uri_str);
        u_pos += uri_len;
        url_buf[u_pos] = '?';
        u_pos += 1;
        @memcpy(url_buf[u_pos .. u_pos + q_pos], canonical_query);
        u_pos += q_pos;
        @memcpy(url_buf[u_pos .. u_pos + sig_param.len], sig_param);
        u_pos += sig_param.len;
        @memcpy(url_buf[u_pos .. u_pos + 64], sig_buf[0..64]);
        u_pos += 64;

        return url_buf[0..u_pos];
    }
}
