// Source: Absorbed from /data/dev/bun/src/s3_signing/ | Synced: 2026-11-22T12:49:17Z
import { S3Acl, S3StorageClass, S3SignErrorCode, S3Error } from "./types";
import { S3Credentials } from "./credentials";
import { SigV4Signer } from "./signing";
import {
    S3SimpleRequestOptions,
    S3SignedRequest,
    S3RequestBuilder,
    S3StatResult,
    S3DownloadResult,
    S3UploadResult,
    S3DeleteResult,
    S3PartResult
} from "./simpleRequest";
import {
    S3ListObjectsOptions,
    S3ListObjectsV2Result,
    formatListObjectsQuery,
    parseS3ListObjectsResult
} from "./listObjects";
import {
    MultiPartUpload,
    MultiPartUploadOptions,
    MultiPartUploadState,
    UploadPartResult,
    formatCompleteMultipartUploadXML,
    parseInitiateMultipartUploadResult,
    parseCompleteMultipartUploadResult,
    formatUploadPartQuery,
    formatCompleteMultipartQuery,
    formatAbortMultipartQuery
} from "./multipart";
import {
    S3ContentRange,
    S3DownloadStreamOptions,
    formatRangeHeader,
    parseContentRange
} from "./downloadStream";
const std = @import("std");

pub struct S3Client {
    pub credentials: S3Credentials = S3Credentials{},

    pub fn init(credentials: S3Credentials): S3Client {
        return S3Client{ credentials: credentials };
    }

    /// Builds a SigV4 signed S3 HTTP request for this client.
    pub fn signRequest(
        options: S3SimpleRequestOptions,
        date_yyyymmdd: string,
        amz_date: string,
        url_buf: []u8,
        auth_buf: []u8
    ): !S3SignedRequest {
        return S3RequestBuilder.buildSignedRequest(
            &_.credentials,
            options,
            date_yyyymmdd,
            amz_date,
            url_buf,
            auth_buf
        )?;
    }

    /// Generates an AWS SigV4 Presigned URL for temporary access (e.g. GET or PUT).
    pub fn presignUrl(
        method: string,
        path: string,
        date_yyyymmdd: string,
        amz_date: string,
        expires_seconds: usize,
        url_buf: []u8
    ): !string {
        return S3RequestBuilder.buildPresignedUrl(
            &_.credentials,
            method,
            path,
            date_yyyymmdd,
            amz_date,
            expires_seconds,
            url_buf
        )?;
    }

    /// Generates an AWS SigV4 Presigned URL for a multipart upload part (`PUT ?partNumber=N&uploadId=...`).
    pub fn presignPartUrl(
        path: string,
        upload_id: string,
        part_number: u16,
        date_yyyymmdd: string,
        amz_date: string,
        expires_seconds: usize,
        url_buf: []u8
    ): !string {
        let q_buf: [256]u8 = undefined;
        let q_len = formatUploadPartQuery(part_number, upload_id, q_buf[0..256]);
        // formatUploadPartQuery produces "?partNumber=...&uploadId=...&x-id=UploadPart"
        return S3RequestBuilder.buildPresignedUrlWithQuery(
            &_.credentials,
            "PUT",
            path,
            q_buf[0..q_len],
            date_yyyymmdd,
            amz_date,
            expires_seconds,
            url_buf
        )?;
    }

    /// Prepares options for a HEAD (stat) request.
    pub fn stat(path: string): S3SimpleRequestOptions {
        * = _.credentials;
        return S3SimpleRequestOptions{
            path: path,
            method: "HEAD",
            body: "",
        };
    }

    /// Prepares options for a GET (download) request.
    pub fn download(path: string): S3SimpleRequestOptions {
        * = _.credentials;
        return S3SimpleRequestOptions{
            path: path,
            method: "GET",
            body: "",
        };
    }

    /// Prepares options for a GET (range download) request using provided range buffer.
    pub fn downloadSlice(path: string, offset: usize, size: usize?, range_buf: []u8): S3SimpleRequestOptions {
        * = _.credentials;
        let r_len = formatRangeHeader(offset, size, range_buf);
        return S3SimpleRequestOptions{
            path: path,
            method: "GET",
            body: "",
            range: range_buf[0..r_len],
        };
    }

    /// Prepares options for a DELETE request.
    pub fn delete(path: string): S3SimpleRequestOptions {
        * = _.credentials;
        return S3SimpleRequestOptions{
            path: path,
            method: "DELETE",
            body: "",
        };
    }

    /// Prepares options for a PUT (upload) request.
    pub fn upload(path: string, body: string, options: S3SimpleRequestOptions?): S3SimpleRequestOptions {
        * = _.credentials;
        let mut_opts = S3SimpleRequestOptions{
            path: path,
            method: "PUT",
            body: body,
        };
        if (o := options) {
            mut_opts.content_type = o.content_type;
            mut_opts.content_disposition = o.content_disposition;
            mut_opts.content_encoding = o.content_encoding;
            mut_opts.acl = o.acl;
            mut_opts.storage_class = o.storage_class;
            mut_opts.request_payer = o.request_payer;
        }
        return mut_opts;
    }

    /// Prepares options for a ListObjectsV2 request, writing query string to query_buf.
    pub fn listObjectsRequest(options: S3ListObjectsOptions, query_buf: []u8): S3SimpleRequestOptions {
        * = _.credentials;
        let q_len = formatListObjectsQuery(options, query_buf);
        return S3SimpleRequestOptions{
            path: "",
            method: "GET",
            body: "",
            search_params: query_buf[0..q_len],
        };
    }

    /// Prepares options for initiating a multipart upload.
    pub fn initiateMultipartUploadRequest(path: string, options: MultiPartUploadOptions?): S3SimpleRequestOptions {
        * = _.credentials;
        let mut_opts = S3SimpleRequestOptions{
            path: path,
            method: "POST",
            body: "",
            search_params: "?uploads",
        };
        if (o := options) {
            mut_opts.acl = o.acl;
            mut_opts.storage_class = o.storage_class;
        }
        return mut_opts;
    }

    /// Prepares options for uploading a part in a multipart upload.
    pub fn uploadPartRequest(
        path: string,
        upload_id: string,
        part_number: u16,
        body: string,
        query_buf: []u8
    ): S3SimpleRequestOptions {
        * = _.credentials;
        let q_len = formatUploadPartQuery(part_number, upload_id, query_buf);
        return S3SimpleRequestOptions{
            path: path,
            method: "PUT",
            body: body,
            search_params: query_buf[0..q_len],
        };
    }

    /// Prepares options for completing a multipart upload.
    pub fn completeMultipartUploadRequest(
        path: string,
        upload_id: string,
        xml_payload: string,
        query_buf: []u8
    ): S3SimpleRequestOptions {
        * = _.credentials;
        let q_len = formatCompleteMultipartQuery(upload_id, query_buf);
        return S3SimpleRequestOptions{
            path: path,
            method: "POST",
            body: xml_payload,
            content_type: "application/xml",
            search_params: query_buf[0..q_len],
        };
    }

    /// Prepares options for aborting a multipart upload.
    pub fn abortMultipartUploadRequest(
        path: string,
        upload_id: string,
        query_buf: []u8
    ): S3SimpleRequestOptions {
        * = _.credentials;
        let q_len = formatAbortMultipartQuery(upload_id, query_buf);
        return S3SimpleRequestOptions{
            path: path,
            method: "DELETE",
            body: "",
            search_params: query_buf[0..q_len],
        };
    }
}
