import { test, expect_eq } from "cap:test";
import {
    S3Acl,
    S3StorageClass,
    S3Error,
    S3Credentials,
    SigV4Signer,
    S3Client,
    S3SimpleRequestOptions,
    S3SignedRequest,
    S3RequestBuilder,
    S3StatResult,
    S3DownloadResult,
    S3UploadResult,
    S3DeleteResult,
    S3ListObjectsOptions,
    S3ListObjectsV2Result,
    formatListObjectsQuery,
    parseS3ListObjectsResult,
    MultiPartUpload,
    MultiPartUploadOptions,
    MultiPartUploadState,
    UploadPartResult,
    formatCompleteMultipartUploadXML,
    parseInitiateMultipartUploadResult,
    parseCompleteMultipartUploadResult,
    formatUploadPartQuery,
    formatCompleteMultipartQuery,
    formatAbortMultipartQuery,
    S3ContentRange,
    S3DownloadStreamOptions,
    formatRangeHeader,
    parseContentRange,
    calculateChunkCount
} from "cap:s3";
const std = @import("std");

fn main() {
    test("1. S3RequestBuilder: Full SigV4 HTTP request generation", () => {
        let creds = S3Credentials{
            access_key_id: "AKIAIOSFODNN7EXAMPLE",
            secret_access_key: "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY",
            region: "us-east-1",
            endpoint: "s3.amazonaws.com",
            bucket: "mybucket",
            virtual_hosted_style: true,
        };

        let opts = S3SimpleRequestOptions{
            path: "data/file.txt",
            method: "GET",
            body: "",
        };

        let url_buf: [256]u8 = undefined;
        let auth_buf: [512]u8 = undefined;

        let req = S3RequestBuilder.buildSignedRequest(
            &creds,
            opts,
            "20261119",
            "20261119T000000Z",
            url_buf[0..256],
            auth_buf[0..512]
        ) catch unreachable;

        expect_eq(req.method, "GET");
        expect_eq(req.host, "mybucket.s3.amazonaws.com");
        expect_eq(req.canonical_uri, "/data/file.txt");
        expect_eq(req.url, "https://mybucket.s3.amazonaws.com/data/file.txt");
        expect_eq(req.date_yyyymmdd, "20261119");
        expect_eq(req.amz_date, "20261119T000000Z");
        expect_eq(req.content_sha256, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

        // Verify Authorization header starts with AWS4-HMAC-SHA256
        let auth_prefix = "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20261119/us-east-1/s3/aws4_request";
        expect_eq(req.auth_header[0..auth_prefix.len], auth_prefix);
    });

    test("2. S3 ListObjectsV2: Query parameter formatting", () => {
        let query_buf: [512]u8 = undefined;

        // Query with prefix, delimiter, and max-keys
        let opts = S3ListObjectsOptions{
            prefix: "photos/",
            delimiter: "/",
            max_keys: 50,
        };
        let q_len = formatListObjectsQuery(opts, query_buf[0..512]);
        let query_str = query_buf[0..q_len];
        expect_eq(query_str, "list-type=2&prefix=photos/&delimiter=/&max-keys=50");

        // Query with continuation-token
        let cont_opts = S3ListObjectsOptions{
            continuation_token: "token123",
            fetch_owner: true,
        };
        let c_len = formatListObjectsQuery(cont_opts, query_buf[0..512]);
        let cont_str = query_buf[0..c_len];
        expect_eq(cont_str, "list-type=2&continuation-token=token123&fetch-owner=true");
    });

    test("3. S3 ListObjectsV2: Complete XML response parsing", () => {
        let xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +
            "<ListBucketResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" +
            "<Name>photos-bucket</Name>" +
            "<Prefix>vacation/</Prefix>" +
            "<KeyCount>2</KeyCount>" +
            "<MaxKeys>100</MaxKeys>" +
            "<IsTruncated>false</IsTruncated>" +
            "<Contents>" +
            "<Key>vacation/beach.jpg</Key>" +
            "<LastModified>2026-07-04T12:00:00.000Z</LastModified>" +
            "<ETag>\"3858f62230ac3c915f300c664312c11f\"</ETag>" +
            "<Size>204800</Size>" +
            "<StorageClass>STANDARD</StorageClass>" +
            "<Owner><ID>owner123</ID><DisplayName>Alice</DisplayName></Owner>" +
            "</Contents>" +
            "<Contents>" +
            "<Key>vacation/mountain.jpg</Key>" +
            "<LastModified>2026-07-05T14:30:00.000Z</LastModified>" +
            "<ETag>\"a54357acf801974020a314e39429839e\"</ETag>" +
            "<Size>409600</Size>" +
            "<StorageClass>INTELLIGENT_TIERING</StorageClass>" +
            "</Contents>" +
            "<CommonPrefixes>" +
            "<Prefix>vacation/2025/</Prefix>" +
            "</CommonPrefixes>" +
            "</ListBucketResult>";

        let res = parseS3ListObjectsResult(std.heap.page_allocator, xml) catch unreachable;
        defer res.deinit(std.heap.page_allocator);

        expect_eq(res.name, "photos-bucket");
        expect_eq(res.prefix, "vacation/");
        expect_eq(res.key_count, 2 as usize);
        expect_eq(res.max_keys, 100 as usize);
        expect_eq(res.is_truncated, false);

        expect_eq(res.contents.len, 2 as usize);
        expect_eq(res.contents[0].key, "vacation/beach.jpg");
        expect_eq(res.contents[0].etag, "\"3858f62230ac3c915f300c664312c11f\"");
        expect_eq(res.contents[0].object_size, 204800 as i64);
        expect_eq(res.contents[0].storage_class, "STANDARD");
        if (owner := res.contents[0].owner) {
            expect_eq(owner.id, "owner123");
            expect_eq(owner.display_name, "Alice");
        }

        expect_eq(res.contents[1].key, "vacation/mountain.jpg");
        expect_eq(res.contents[1].etag, "\"a54357acf801974020a314e39429839e\"");
        expect_eq(res.contents[1].object_size, 409600 as i64);
        expect_eq(res.contents[1].storage_class, "INTELLIGENT_TIERING");

        expect_eq(res.common_prefixes.len, 1 as usize);
        expect_eq(res.common_prefixes[0], "vacation/2025/");
    });

    test("4. S3 MultiPartUpload: XML formatting, XML parsing, query generation", () => {
        // 4.1 Parse InitiateMultipartUpload XML response
        let init_xml = "<InitiateMultipartUploadResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" +
            "<Bucket>examplebucket</Bucket>" +
            "<Key>exampleobject</Key>" +
            "<UploadId>VXBsb2FkIElEIGV4YW1wbGU</UploadId>" +
            "</InitiateMultipartUploadResult>";
        let parsed_upload_id = parseInitiateMultipartUploadResult(init_xml);
        expect_eq(parsed_upload_id != null, true);
        if (uid := parsed_upload_id) {
            expect_eq(uid, "VXBsb2FkIElEIGV4YW1wbGU");
        }

        // 4.2 Format CompleteMultipartUpload XML payload
        let parts: [2]UploadPartResult = [
            UploadPartResult{ number: 1, etag: "\"a54357acf801974020a314e39429839e\"" },
            UploadPartResult{ number: 2, etag: "\"0c78aef83f66abc12f8f9e1d7b92131a\"" },
        ];
        let xml_buf: [1024]u8 = undefined;
        let xml_len = formatCompleteMultipartUploadXML(parts[0..2], xml_buf[0..1024]);
        let expected_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><CompleteMultipartUpload xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" +
            "<Part><PartNumber>1</PartNumber><ETag>\"a54357acf801974020a314e39429839e\"</ETag></Part>" +
            "<Part><PartNumber>2</PartNumber><ETag>\"0c78aef83f66abc12f8f9e1d7b92131a\"</ETag></Part>" +
            "</CompleteMultipartUpload>";
        expect_eq(xml_buf[0..xml_len], expected_xml);

        // 4.3 Parse CompleteMultipartUploadResult XML
        let complete_res_xml = "<CompleteMultipartUploadResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" +
            "<Location>http://examplebucket.s3.amazonaws.com/exampleobject</Location>" +
            "<Bucket>examplebucket</Bucket>" +
            "<Key>exampleobject</Key>" +
            "<ETag>\"3858f62230ac3c915f300c664312c11f-2\"</ETag>" +
            "</CompleteMultipartUploadResult>";
        let parsed_etag = parseCompleteMultipartUploadResult(complete_res_xml);
        expect_eq(parsed_etag != null, true);
        if (tag := parsed_etag) {
            expect_eq(tag, "\"3858f62230ac3c915f300c664312c11f-2\"");
        }

        // 4.4 Format query params
        let q_buf: [256]u8 = undefined;
        let q_len = formatUploadPartQuery(3, "up123", q_buf[0..256]);
        expect_eq(q_buf[0..q_len], "?partNumber=3&uploadId=up123&x-id=UploadPart");

        let c_len = formatCompleteMultipartQuery("up123", q_buf[0..256]);
        expect_eq(q_buf[0..c_len], "?uploadId=up123");

        let a_len = formatAbortMultipartQuery("up123", q_buf[0..256]);
        expect_eq(q_buf[0..a_len], "?uploadId=up123");
    });

    test("5. S3 Download Streaming: Range header formatting & Content-Range parsing", () => {
        // 5.1 formatRangeHeader with size
        let r_buf: [64]u8 = undefined;
        let len1 = formatRangeHeader(0, 1024, r_buf[0..64]);
        expect_eq(r_buf[0..len1], "bytes=0-1023");

        let len2 = formatRangeHeader(1024, 2048, r_buf[0..64]);
        expect_eq(r_buf[0..len2], "bytes=1024-3071");

        // without size
        let len3 = formatRangeHeader(2048, null, r_buf[0..64]);
        expect_eq(r_buf[0..len3], "bytes=2048-");

        // 5.2 parseContentRange
        let range1 = parseContentRange("bytes 0-1023/4096");
        expect_eq(range1 != null, true);
        if (r1 := range1) {
            expect_eq(r1.start, 0 as usize);
            expect_eq(r1.end, 1023 as usize);
            expect_eq(r1.total, 4096 as usize);
        }

        let range2 = parseContentRange("  bytes 200-500/1000  ");
        expect_eq(range2 != null, true);
        if (r2 := range2) {
            expect_eq(r2.start, 200 as usize);
            expect_eq(r2.end, 500 as usize);
            expect_eq(r2.total, 1000 as usize);
        }

        // 5.3 calculateChunkCount
        expect_eq(calculateChunkCount(100, 10), 10 as usize);
        expect_eq(calculateChunkCount(105, 10), 11 as usize);
        expect_eq(calculateChunkCount(0, 10), 0 as usize);
    });

    test("6. S3Client: High-level client API initialization & request builders", () => {
        let creds = S3Credentials{
            access_key_id: "AKIAIOSFODNN7EXAMPLE",
            secret_access_key: "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY",
            region: "us-west-2",
            endpoint: "s3.us-west-2.amazonaws.com",
            bucket: "backup-vault",
            virtual_hosted_style: true,
        };

        let client = S3Client.init(creds);

        // Stat request
        let stat_req = client.stat("archive.tar.gz");
        expect_eq(stat_req.method, "HEAD");
        expect_eq(stat_req.path, "archive.tar.gz");

        // Download request
        let dl_req = client.download("archive.tar.gz");
        expect_eq(dl_req.method, "GET");

        // DownloadSlice request
        let range_buf: [64]u8 = undefined;
        let slice_req = client.downloadSlice("archive.tar.gz", 0, 1048576, range_buf[0..64]);
        expect_eq(slice_req.method, "GET");
        if (r := slice_req.range) {
            expect_eq(r, "bytes=0-1048575");
        }

        // Upload request
        let up_opts = S3SimpleRequestOptions{
            content_type: "application/json",
            acl: S3Acl.Private,
            storage_class: S3StorageClass.StandardIa,
        };
        let up_req = client.upload("meta.json", "{\"status\":\"ok\"}", up_opts);
        expect_eq(up_req.method, "PUT");
        expect_eq(up_req.body, "{\"status\":\"ok\"}");
        if (ct := up_req.content_type) {
            expect_eq(ct, "application/json");
        }

        // Delete request
        let del_req = client.delete("tmp.txt");
        expect_eq(del_req.method, "DELETE");

        // Multipart requests
        let init_mp = client.initiateMultipartUploadRequest("bigfile.bin", null);
        expect_eq(init_mp.method, "POST");
        if (sp := init_mp.search_params) {
            expect_eq(sp, "?uploads");
        }

        let q_buf: [256]u8 = undefined;
        let part_req = client.uploadPartRequest("bigfile.bin", "upload123", 1, "chunk1", q_buf[0..256]);
        expect_eq(part_req.method, "PUT");
        if (sp := part_req.search_params) {
            expect_eq(sp, "?partNumber=1&uploadId=upload123&x-id=UploadPart");
        }

        let comp_req = client.completeMultipartUploadRequest("bigfile.bin", "upload123", "<CompleteMultipartUpload/>", q_buf[0..256]);
        expect_eq(comp_req.method, "POST");
        expect_eq(comp_req.body, "<CompleteMultipartUpload/>");

        let abort_req = client.abortMultipartUploadRequest("bigfile.bin", "upload123", q_buf[0..256]);
        expect_eq(abort_req.method, "DELETE");
        if (sp := abort_req.search_params) {
            expect_eq(sp, "?uploadId=upload123");
        }

        // Sign request via client
        let url_buf: [256]u8 = undefined;
        let auth_buf: [512]u8 = undefined;
        let signed = client.signRequest(
            stat_req,
            "20261119",
            "20261119T120000Z",
            url_buf[0..256],
            auth_buf[0..512]
        ) catch unreachable;

        expect_eq(signed.method, "HEAD");
        expect_eq(signed.url, "https://backup-vault.s3.us-west-2.amazonaws.com/archive.tar.gz");
    });
}
