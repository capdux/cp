// Source: /data/dev/bun/src/s3_signing/
// ============================================================================
// lib/s3/list_objects.cp — S3 ListObjectsV2 参数构造与 XML 结果解析
// 吸收自 Bun src/runtime/webcore/s3/list_objects.cp
// ============================================================================

const std = @import("std");

pub struct S3ListObjectsOptions {
    prefix: string? = null,
    delimiter: string? = null,
    max_keys: usize? = null,
    continuation_token: string? = null,
    start_after: string? = null,
    fetch_owner: bool? = null,
    encoding_type: string? = null,
}

pub struct ObjectOwner {
    id: string = "",
    display_name: string = "",
}

pub struct S3ListObjectsContents {
    key: string = "",
    etag: string = "",
    last_modified: string = "",
    object_size: i64 = 0,
    storage_class: string = "STANDARD",
    owner: ObjectOwner? = null,
}

pub struct S3ListObjectsV2Result {
    name: string = "",
    prefix: string = "",
    key_count: usize = 0,
    max_keys: usize = 1000,
    delimiter: string = "",
    is_truncated: bool = false,
    continuation_token: string = "",
    next_continuation_token: string = "",
    start_after: string = "",
    contents: []S3ListObjectsContents = [],
    common_prefixes: []string = [],

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.contents.len > 0) {
            allocator.free(_.contents);
            _.contents = [];
        }
        if (_.common_prefixes.len > 0) {
            allocator.free(_.common_prefixes);
            _.common_prefixes = [];
        }
    }
}

fn listAppendUsize(dst: []u8, offset: usize, val: usize): usize {
    if (val == 0) {
        if (offset < dst.len) {
            dst[offset] = 48 as u8; // '0'
            return offset + 1;
        }
        return offset;
    }
    let temp: [20]u8 = undefined;
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

/// 格式化 ListObjectsV2 查询参数：`list-type=2&prefix=...&delimiter=...`
pub fn formatListObjectsQuery(opts: S3ListObjectsOptions, out: []u8): usize {
    let pos: usize = 0;
    const base = "list-type=2";
    if (out.len < base.len) return 0;
    @memcpy(out[0..base.len], base);
    pos += base.len;

    if (p := opts.prefix) {
        if (p.len > 0 && pos + 9 + p.len <= out.len) {
            const pref = "&prefix=";
            @memcpy(out[pos .. pos + pref.len], pref);
            pos += pref.len;
            @memcpy(out[pos .. pos + p.len], p);
            pos += p.len;
        }
    }

    if (d := opts.delimiter) {
        if (d.len > 0 && pos + 12 + d.len <= out.len) {
            const d_str = "&delimiter=";
            @memcpy(out[pos .. pos + d_str.len], d_str);
            pos += d_str.len;
            @memcpy(out[pos .. pos + d.len], d);
            pos += d.len;
        }
    }

    if (mk := opts.max_keys) {
        let mk_buf: [32]u8 = undefined;
        let mk_len = listAppendUsize(mk_buf[0..32], 0, mk);
        if (pos + 11 + mk_len <= out.len) {
            const k_str = "&max-keys=";
            @memcpy(out[pos .. pos + k_str.len], k_str);
            pos += k_str.len;
            @memcpy(out[pos .. pos + mk_len], mk_buf[0..mk_len]);
            pos += mk_len;
        }
    }

    if (ct := opts.continuation_token) {
        if (ct.len > 0 && pos + 21 + ct.len <= out.len) {
            const ct_str = "&continuation-token=";
            @memcpy(out[pos .. pos + ct_str.len], ct_str);
            pos += ct_str.len;
            @memcpy(out[pos .. pos + ct.len], ct);
            pos += ct.len;
        }
    }

    if (sa := opts.start_after) {
        if (sa.len > 0 && pos + 14 + sa.len <= out.len) {
            const sa_str = "&start-after=";
            @memcpy(out[pos .. pos + sa_str.len], sa_str);
            pos += sa_str.len;
            @memcpy(out[pos .. pos + sa.len], sa);
            pos += sa.len;
        }
    }

    if (fo := opts.fetch_owner) {
        if (fo) {
            const fo_str = "&fetch-owner=true";
            if (pos + fo_str.len <= out.len) {
                @memcpy(out[pos .. pos + fo_str.len], fo_str);
                pos += fo_str.len;
            }
        }
    }

    if (et := opts.encoding_type) {
        if (et.len > 0 && pos + 16 + et.len <= out.len) {
            const et_str = "&encoding-type=";
            @memcpy(out[pos .. pos + et_str.len], et_str);
            pos += et_str.len;
            @memcpy(out[pos .. pos + et.len], et);
            pos += et.len;
        }
    }

    return pos;
}

/// 解析 S3 ListObjectsV2 官方 XML 响应
pub fn parseS3ListObjectsResult(allocator: std.mem.Allocator, xml: string): !S3ListObjectsV2Result {
    let result = S3ListObjectsV2Result{
        name: "",
        prefix: "",
        key_count: 0,
        max_keys: 1000,
        delimiter: "",
        is_truncated: false,
        continuation_token: "",
        next_continuation_token: "",
        start_after: "",
        contents: [],
        common_prefixes: [],
    };

    let contents_list: std.ArrayListUnmanaged(S3ListObjectsContents) = .empty;
    errdefer contents_list.deinit(allocator);

    let prefixes_list: std.ArrayListUnmanaged(string) = .empty;
    errdefer prefixes_list.deinit(allocator);

    // 简单标量标签提取
    if (name_val := extractXmlTag(xml, "Name")) {
        result.name = name_val;
    }
    if (prefix_val := extractXmlTag(xml, "Prefix")) {
        result.prefix = prefix_val;
    }
    if (del_val := extractXmlTag(xml, "Delimiter")) {
        result.delimiter = del_val;
    }
    if (sa_val := extractXmlTag(xml, "StartAfter")) {
        result.start_after = sa_val;
    }
    if (ct_val := extractXmlTag(xml, "ContinuationToken")) {
        result.continuation_token = ct_val;
    }
    if (nct_val := extractXmlTag(xml, "NextContinuationToken")) {
        result.next_continuation_token = nct_val;
    }
    if (trunc_val := extractXmlTag(xml, "IsTruncated")) {
        result.is_truncated = std.mem.eql(u8, trunc_val, "true");
    }
    if (kc_val := extractXmlTag(xml, "KeyCount")) {
        result.key_count = std.fmt.parseInt(usize, kc_val, 10) catch 0;
    }
    if (mk_val := extractXmlTag(xml, "MaxKeys")) {
        result.max_keys = std.fmt.parseInt(usize, mk_val, 10) catch 1000;
    }

    // 扫描所有 <Contents>...</Contents>
    let pos: usize = 0;
    while (pos < xml.len) {
        const start_tag = "<Contents>";
        const end_tag = "</Contents>";

        let start_idx_opt = std.mem.indexOfPos(u8, xml, pos, start_tag);
        if (start_idx_opt == null) break;
        let c_start = (start_idx_opt ?? 0) + start_tag.len;

        let end_idx_opt = std.mem.indexOfPos(u8, xml, c_start, end_tag);
        if (end_idx_opt == null) break;
        let c_end = end_idx_opt ?? 0;

        const content_xml = xml[c_start..c_end];
        let item = S3ListObjectsContents{
            key: extractXmlTag(content_xml, "Key") ?? "",
            etag: extractXmlTag(content_xml, "ETag") ?? "",
            last_modified: extractXmlTag(content_xml, "LastModified") ?? "",
            object_size: 0,
            storage_class: extractXmlTag(content_xml, "StorageClass") ?? "STANDARD",
            owner: null,
        };

        if (sz_str := extractXmlTag(content_xml, "Size")) {
            item.object_size = std.fmt.parseInt(i64, sz_str, 10) catch 0;
        }

        if (owner_id := extractXmlTag(content_xml, "ID")) {
            item.owner = ObjectOwner{
                id: owner_id,
                display_name: extractXmlTag(content_xml, "DisplayName") ?? "",
            };
        }

        contents_list.append(allocator, item)?;
        pos = c_end + end_tag.len;
    }

    // 扫描所有 <CommonPrefixes><Prefix>...</Prefix></CommonPrefixes>
    pos = 0;
    while (pos < xml.len) {
        const cp_start_tag = "<CommonPrefixes>";
        const cp_end_tag = "</CommonPrefixes>";

        let start_idx_opt = std.mem.indexOfPos(u8, xml, pos, cp_start_tag);
        if (start_idx_opt == null) break;
        let cp_start = (start_idx_opt ?? 0) + cp_start_tag.len;

        let end_idx_opt = std.mem.indexOfPos(u8, xml, cp_start, cp_end_tag);
        if (end_idx_opt == null) break;
        let cp_end = end_idx_opt ?? 0;

        const cp_xml = xml[cp_start..cp_end];
        if (p_val := extractXmlTag(cp_xml, "Prefix")) {
            prefixes_list.append(allocator, p_val)?;
        }
        pos = cp_end + cp_end_tag.len;
    }

    result.contents = contents_list.toOwnedSlice(allocator)?;
    result.common_prefixes = prefixes_list.toOwnedSlice(allocator)?;
    if (result.key_count == 0) {
        result.key_count = result.contents.len;
    }

    return result;
}

fn extractXmlTag(xml: string, tag: string): string? {
    let open_buf: [64]u8 = undefined;
    if (tag.len + 2 > 64) return null;
    open_buf[0] = '<';
    @memcpy(open_buf[1 .. 1 + tag.len], tag);
    open_buf[1 + tag.len] = '>';
    const open_tag = open_buf[0 .. 2 + tag.len];

    let close_buf: [64]u8 = undefined;
    if (tag.len + 3 > 64) return null;
    close_buf[0] = '<';
    close_buf[1] = '/';
    @memcpy(close_buf[2 .. 2 + tag.len], tag);
    close_buf[2 + tag.len] = '>';
    const close_tag = close_buf[0 .. 3 + tag.len];

    let start_opt = std.mem.indexOf(u8, xml, open_tag);
    if (start_opt == null) return null;
    let val_start = (start_opt ?? 0) + open_tag.len;

    let end_opt = std.mem.indexOfPos(u8, xml, val_start, close_tag);
    if (end_opt == null) return null;
    let val_end = end_opt ?? 0;

    return xml[val_start..val_end];
}
