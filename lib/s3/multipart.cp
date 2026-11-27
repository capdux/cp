// Source: Absorbed from /data/dev/bun/src/s3_signing/ | Synced: 2026-11-22T12:49:17Z
import { S3Acl, S3StorageClass, S3Error } from "./types";
const std = @import("std");

pub enum MultiPartUploadState: u8 {
    wait_stream_check = 0,
    not_started = 1,
    multipart_started = 2,
    multipart_completed = 3,
    singlefile_started = 4,
    finished = 5,
}

pub enum UploadPartState: u8 {
    not_assigned = 0,
    pending = 1,
    started = 2,
    completed = 3,
    canceled = 4,
}

pub enum WriteEncoding: u8 {
    bytes = 0,
    latin1 = 1,
    utf16 = 2,
}

pub struct UploadPartResult {
    pub number: u16 = 0,
    pub etag: string = "",
}

pub struct MultiPartUploadOptions {
    pub const OneMiB: usize = 1048576;
    pub const MAX_SINGLE_UPLOAD_SIZE: usize = 5120 * 1048576;
    pub const MIN_SINGLE_UPLOAD_SIZE: usize = 5 * 1048576;
    pub const DefaultPartSize: usize = 5 * 1048576;
    pub const MAX_QUEUE_SIZE: usize = 64;

    pub queue_size: u8 = 5,
    pub part_size: usize = 5 * 1048576,
    pub retry: u8 = 3,
    pub acl: S3Acl? = null,
    pub storage_class: S3StorageClass? = null,
}

pub struct MultiPartUpload {
    pub upload_id: string = "",
    pub key: string = "",
    pub options: MultiPartUploadOptions = MultiPartUploadOptions{},
    pub state: MultiPartUploadState = MultiPartUploadState.not_started,

    pub static fn init(upload_id: string, key: string, options: MultiPartUploadOptions?): MultiPartUpload {
        let opts = MultiPartUploadOptions{};
        if (o := options) {
            opts = o;
        }
        return MultiPartUpload{
            upload_id: upload_id,
            key: key,
            options: opts,
            state: MultiPartUploadState.multipart_started,
        };
    }
}

fn mpAppendSlice(dst: []u8, offset: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len and offset + i < dst.len) {
        dst[offset + i] = src[i];
        i += 1;
    }
    return offset + i;
}

fn mpAppendU16(dst: []u8, offset: usize, val: u16): usize {
    if (val == 0) {
        if (offset < dst.len) {
            dst[offset] = 48 as u8; // '0'
            return offset + 1;
        }
        return offset;
    }
    let temp: [8]u8 = undefined;
    let count: usize = 0;
    let v = val;
    while (v > 0) {
        temp[count] = ((48 as u8) + ((v % 10) as u8)) as u8;
        v /= 10;
        count += 1;
    }
    let cur = offset;
    while (count > 0 and cur < dst.len) {
        count -= 1;
        dst[cur] = temp[count];
        cur += 1;
    }
    return cur;
}

fn mpExtractXmlTag(xml: string, tag: string): string? {
    let open_buf: [64]u8 = undefined;
    if (tag.len + 2 > open_buf.len) return null;
    open_buf[0] = 60 as u8; // '<'
    let i: usize = 0;
    while (i < tag.len) {
        open_buf[1 + i] = tag[i];
        i += 1;
    }
    open_buf[1 + tag.len] = 62 as u8; // '>'
    let open_tag = open_buf[0 .. tag.len + 2];

    let close_buf: [64]u8 = undefined;
    if (tag.len + 3 > close_buf.len) return null;
    close_buf[0] = 60 as u8; // '<'
    close_buf[1] = 47 as u8; // '/'
    i = 0;
    while (i < tag.len) {
        close_buf[2 + i] = tag[i];
        i += 1;
    }
    close_buf[2 + tag.len] = 62 as u8; // '>'
    let close_tag = close_buf[0 .. tag.len + 3];

    let start_idx = mpFindSubstr(xml, open_tag);
    if (s := start_idx) {
        let content_start = s + open_tag.len;
        let rest = xml[content_start..xml.len];
        let end_idx = mpFindSubstr(rest, close_tag);
        if (e := end_idx) {
            return rest[0..e];
        }
    }
    return null;
}

fn mpFindSubstr(haystack: string, needle: []const u8): usize? {
    if (needle.len == 0) return 0 as usize;
    if (haystack.len < needle.len) return null;
    let max_idx = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= max_idx) {
        let is_match = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                is_match = false;
                break;
            }
            j += 1;
        }
        if (is_match) return i;
        i += 1;
    }
    return null;
}

/// Parses `<UploadId>...</UploadId>` from InitiateMultipartUpload XML response.
pub fn parseInitiateMultipartUploadResult(xml: string): string? {
    return mpExtractXmlTag(xml, "UploadId");
}

/// Parses `<ETag>...</ETag>` from CompleteMultipartUpload XML response.
pub fn parseCompleteMultipartUploadResult(xml: string): string? {
    return mpExtractXmlTag(xml, "ETag");
}

/// Formats the XML payload for CompleteMultipartUpload:
/// `<?xml version="1.0" encoding="UTF-8"?><CompleteMultipartUpload xmlns="http://s3.amazonaws.com/doc/2006-03-01/"><Part><PartNumber>1</PartNumber><ETag>"..."</ETag></Part>...</CompleteMultipartUpload>`
pub fn formatCompleteMultipartUploadXML(parts: []const UploadPartResult, out: []u8): usize {
    let offset: usize = 0;
    offset = mpAppendSlice(out, offset, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><CompleteMultipartUpload xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">");

    let i: usize = 0;
    while (i < parts.len) {
        let p = parts[i];
        offset = mpAppendSlice(out, offset, "<Part><PartNumber>");
        offset = mpAppendU16(out, offset, p.number);
        offset = mpAppendSlice(out, offset, "</PartNumber><ETag>");
        offset = mpAppendSlice(out, offset, p.etag);
        offset = mpAppendSlice(out, offset, "</ETag></Part>");
        i += 1;
    }

    offset = mpAppendSlice(out, offset, "</CompleteMultipartUpload>");
    return offset;
}

/// Formats the XML payload for CompleteMultipartUpload into an allocated string.
pub fn formatCompleteMultipartUploadString(parts: []const UploadPartResult, allocator: std.mem.Allocator): !string {
    let buf: [16384]u8 = undefined;
    let len = formatCompleteMultipartUploadXML(parts, buf[0..16384]);
    let result = allocator.alloc(u8, len)?;
    std.mem.copyForwards(u8, result, buf[0..len]);
    return result;
}

/// Formats query params for uploading a single part: `?partNumber={}&uploadId={}`
pub fn formatUploadPartQuery(part_number: u16, upload_id: string, out: []u8): usize {
    let offset: usize = 0;
    offset = mpAppendSlice(out, offset, "?partNumber=");
    offset = mpAppendU16(out, offset, part_number);
    offset = mpAppendSlice(out, offset, "&uploadId=");
    offset = mpAppendSlice(out, offset, upload_id);
    offset = mpAppendSlice(out, offset, "&x-id=UploadPart");
    return offset;
}

/// Formats query params for completing a multipart upload: `?uploadId={}`
pub fn formatCompleteMultipartQuery(upload_id: string, out: []u8): usize {
    let offset: usize = 0;
    offset = mpAppendSlice(out, offset, "?uploadId=");
    offset = mpAppendSlice(out, offset, upload_id);
    return offset;
}

/// Formats query params for aborting a multipart upload: `?uploadId={}`
pub fn formatAbortMultipartQuery(upload_id: string, out: []u8): usize {
    return formatCompleteMultipartQuery(upload_id, out);
}
