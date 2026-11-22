// Source: Absorbed from /data/dev/bun/src/s3_signing/ | Synced: 2026-11-20T07:13:34Z
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
    signed_headers: string = "host;x-amz-content-sha256;x-amz-date",
    security_token: string? = null,
}

fn s3AppendPercentEncoded(dst: []u8, pos_in: usize, src: []const u8): usize {
    const hex_upper: []const u8 = "0123456789ABCDEF";
    let pos = pos_in;
    let i: usize = 0;
    while (i < src.len) {
        const c = src[i];
        const is_unreserved = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~';
        if (is_unreserved) {
            if (pos + 1 > dst.len) break;
            dst[pos] = c;
            pos += 1;
        } else {
            if (pos + 3 > dst.len) break;
            dst[pos] = '%';
            dst[pos + 1] = hex_upper[((c >> 4) & 0x0F) as usize];
            dst[pos + 2] = hex_upper[(c & 0x0F) as usize];
            pos += 3;
        }
        i += 1;
    }
    return pos;
}

struct S3HeaderCursor {
    c_pos: usize = 0,
    sh_pos: usize = 0,
}

fn s3AppendHeaderLine(
    can_buf: []u8,
    c_pos_in: usize,
    sh_buf: []u8,
    sh_pos_in: usize,
    name: string,
    val: string,
): S3HeaderCursor {
    let c_pos = c_pos_in;
    let sh_pos = sh_pos_in;
    if (sh_pos > 0 && sh_pos < sh_buf.len) {
        sh_buf[sh_pos] = ';';
        sh_pos += 1;
    }
    if (sh_pos + name.len <= sh_buf.len) {
        @memcpy(sh_buf[sh_pos .. sh_pos + name.len], name);
        sh_pos += name.len;
    }
    if (c_pos + name.len + 1 + val.len + 1 <= can_buf.len) {
        @memcpy(can_buf[c_pos .. c_pos + name.len], name);
        c_pos += name.len;
        can_buf[c_pos] = ':';
        c_pos += 1;
        @memcpy(can_buf[c_pos .. c_pos + val.len], val);
        c_pos += val.len;
        can_buf[c_pos] = '\n';
        c_pos += 1;
    }
    return S3HeaderCursor{ .c_pos = c_pos, .sh_pos = sh_pos };
}

pub struct S3RequestBuilder {
    /// 构造标准 AWS SigV4 签名请求（支持全量可选标头与 STS session_token）
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
        if (url_buf.len < proto.len + host_len + uri_len) return err.BufferTooSmall;
        @memcpy(url_buf[0..proto.len], proto);
        u_pos += proto.len;
        @memcpy(url_buf[u_pos .. u_pos + host_len], host_str);
        u_pos += host_len;
        @memcpy(url_buf[u_pos .. u_pos + uri_len], uri_str);
        u_pos += uri_len;

        if (sp := opts.search_params) {
            if (sp.len > 0) {
                const need_q: usize = if (sp[0] != '?') 1 else 0;
                if (u_pos + need_q + sp.len > url_buf.len) return err.BufferTooSmall;
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

        // 构造 Canonical Headers 与 SignedHeaders（严格按 ASCII 字母序）
        let canonical_buf: [2048]u8 = undefined;
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

        // 按严格字母序写入 CanonicalHeaders 与 SignedHeaders:
        // content-disposition, content-encoding, content-type, host, range,
        // x-amz-acl, x-amz-content-sha256, x-amz-date, x-amz-request-payer,
        // x-amz-security-token, x-amz-storage-class
        let sh_buf: [256]u8 = undefined;
        let sh_pos: usize = 0;

        if (cd := opts.content_disposition) {
            if (cd.len > 0) {
                const r = s3AppendHeaderLine(&canonical_buf, c_pos, &sh_buf, sh_pos, "content-disposition", cd);
                c_pos = r.c_pos;
                sh_pos = r.sh_pos;
            }
        }
        if (ce := opts.content_encoding) {
            if (ce.len > 0) {
                const r = s3AppendHeaderLine(&canonical_buf, c_pos, &sh_buf, sh_pos, "content-encoding", ce);
                c_pos = r.c_pos;
                sh_pos = r.sh_pos;
            }
        }
        if (ct := opts.content_type) {
            if (ct.len > 0) {
                const r = s3AppendHeaderLine(&canonical_buf, c_pos, &sh_buf, sh_pos, "content-type", ct);
                c_pos = r.c_pos;
                sh_pos = r.sh_pos;
            }
        }
        {
            const r = s3AppendHeaderLine(&canonical_buf, c_pos, &sh_buf, sh_pos, "host", host_str);
            c_pos = r.c_pos;
            sh_pos = r.sh_pos;
        }
        if (rng := opts.range) {
            if (rng.len > 0) {
                const r = s3AppendHeaderLine(&canonical_buf, c_pos, &sh_buf, sh_pos, "range", rng);
                c_pos = r.c_pos;
                sh_pos = r.sh_pos;
            }
        }
        let effective_acl: S3Acl? = opts.acl;
        if (effective_acl == null) {
            effective_acl = creds.acl;
        }
        if (acl := effective_acl) {
            const acl_str = S3Acl.toString(acl);
            const r = s3AppendHeaderLine(&canonical_buf, c_pos, &sh_buf, sh_pos, "x-amz-acl", acl_str);
            c_pos = r.c_pos;
            sh_pos = r.sh_pos;
        }
        {
            const r = s3AppendHeaderLine(&canonical_buf, c_pos, &sh_buf, sh_pos, "x-amz-content-sha256", payload_hash);
            c_pos = r.c_pos;
            sh_pos = r.sh_pos;
        }
        {
            const r = s3AppendHeaderLine(&canonical_buf, c_pos, &sh_buf, sh_pos, "x-amz-date", amz_date);
            c_pos = r.c_pos;
            sh_pos = r.sh_pos;
        }
        if (opts.request_payer) {
            const r = s3AppendHeaderLine(&canonical_buf, c_pos, &sh_buf, sh_pos, "x-amz-request-payer", "requester");
            c_pos = r.c_pos;
            sh_pos = r.sh_pos;
        }
        if (creds.session_token.len > 0) {
            const r = s3AppendHeaderLine(&canonical_buf, c_pos, &sh_buf, sh_pos, "x-amz-security-token", creds.session_token);
            c_pos = r.c_pos;
            sh_pos = r.sh_pos;
        }
        let effective_sc: S3StorageClass? = opts.storage_class;
        if (effective_sc == null) {
            effective_sc = creds.storage_class;
        }
        if (sc := effective_sc) {
            const sc_str = S3StorageClass.toString(sc);
            const r = s3AppendHeaderLine(&canonical_buf, c_pos, &sh_buf, sh_pos, "x-amz-storage-class", sc_str);
            c_pos = r.c_pos;
            sh_pos = r.sh_pos;
        }

        // Blank line separating headers & signed headers
        canonical_buf[c_pos] = '\n';
        c_pos += 1;

        // SignedHeaders
        @memcpy(canonical_buf[c_pos .. c_pos + sh_pos], sh_buf[0..sh_pos]);
        c_pos += sh_pos;
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
        if (sig_len != 64) return err.SignError;
        const sig_str = sig_buf[0..sig_len];

        // 将 signed_headers 持久化于 auth_buf[64 .. 64 + sh_pos]，auth_header 紧随其后
        let a_pos: usize = 64;
        if (a_pos + sh_pos > auth_buf.len) return err.BufferTooSmall;
        @memcpy(auth_buf[a_pos .. a_pos + sh_pos], sh_buf[0..sh_pos]);
        const signed_headers_slice = auth_buf[a_pos .. a_pos + sh_pos];
        a_pos += sh_pos;

        const auth_start = a_pos;
        const auth_prefix = "AWS4-HMAC-SHA256 Credential=";
        if (a_pos + auth_prefix.len + creds.access_key_id.len + 1 + 64 + 17 + sh_pos + 12 + sig_len > auth_buf.len) {
            return err.BufferTooSmall;
        }
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

        const sh_label = ", SignedHeaders=";
        @memcpy(auth_buf[a_pos .. a_pos + sh_label.len], sh_label);
        a_pos += sh_label.len;
        @memcpy(auth_buf[a_pos .. a_pos + sh_pos], sh_buf[0..sh_pos]);
        a_pos += sh_pos;

        const sig_label = ", Signature=";
        @memcpy(auth_buf[a_pos .. a_pos + sig_label.len], sig_label);
        a_pos += sig_label.len;

        @memcpy(auth_buf[a_pos .. a_pos + sig_len], sig_str);
        a_pos += sig_len;

        const auth_header = auth_buf[auth_start..a_pos];

        return S3SignedRequest{
            method: opts.method,
            url: full_url,
            host: url_buf[proto.len .. proto.len + host_len],
            canonical_uri: url_buf[proto.len + host_len .. proto.len + host_len + uri_len],
            date_yyyymmdd: date_yyyymmdd,
            amz_date: amz_date,
            content_sha256: payload_hash,
            auth_header: auth_header,
            signed_headers: signed_headers_slice,
            security_token: if (creds.session_token.len > 0) creds.session_token else null,
        };
    }

    /// 构造 AWS SigV4 预签名 URL (Presigned URL, Query-based Authentication)
    /// 采用 `UNSIGNED-PAYLOAD` 与 `X-Amz-SignedHeaders=host`，支持 STS `session_token` 与附加查询参数
    pub static fn buildPresignedUrl(
        creds: *const S3Credentials,
        method: string,
        path: string,
        date_yyyymmdd: string,
        amz_date: string,
        expires_seconds: usize,
        url_buf: []u8,
    ): string! {
        return S3RequestBuilder.buildPresignedUrlWithQuery(
            creds,
            method,
            path,
            null,
            date_yyyymmdd,
            amz_date,
            expires_seconds,
            url_buf,
        )?;
    }

    /// 构造带自定义查询参数（如 `partNumber=1&uploadId=...`）的 AWS SigV4 预签名 URL
    pub static fn buildPresignedUrlWithQuery(
        creds: *const S3Credentials,
        method: string,
        path: string,
        extra_query: string?,
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
        // X-Amz-Algorithm, X-Amz-Credential, X-Amz-Date, X-Amz-Expires,
        // [X-Amz-Security-Token], X-Amz-SignedHeaders, [extra lowercase query params]
        let query_buf: [1024]u8 = undefined;
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

        if (creds.session_token.len > 0) {
            const p_tok = "&X-Amz-Security-Token=";
            if (q_pos + p_tok.len + creds.session_token.len > query_buf.len) return err.BufferTooSmall;
            @memcpy(query_buf[q_pos .. q_pos + p_tok.len], p_tok);
            q_pos += p_tok.len;
            q_pos = s3AppendPercentEncoded(&query_buf, q_pos, creds.session_token);
        }

        const p4 = "&X-Amz-SignedHeaders=host";
        if (q_pos + p4.len > query_buf.len) return err.BufferTooSmall;
        @memcpy(query_buf[q_pos .. q_pos + p4.len], p4);
        q_pos += p4.len;

        if (eq := extra_query) {
            const clean_eq = if (eq.len > 0 && eq[0] == '?') eq[1..] else eq;
            if (clean_eq.len > 0) {
                if (q_pos + 1 + clean_eq.len > query_buf.len) return err.BufferTooSmall;
                query_buf[q_pos] = '&';
                q_pos += 1;
                @memcpy(query_buf[q_pos .. q_pos + clean_eq.len], clean_eq);
                q_pos += clean_eq.len;
            }
        }
        const canonical_query = query_buf[0..q_pos];

        // Build Canonical Request:
        // <Method>\n<CanonicalURI>\n<CanonicalQueryString>\nhost:<host>\n\nhost\nUNSIGNED-PAYLOAD
        let canonical_buf: [2048]u8 = undefined;
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
