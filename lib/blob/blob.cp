// Source: Absorbed from /data/dev/bun/src/runtime/webcore/Blob.cp | Synced: 2026-11-22T16:44:15Z
// ============================================================================
// Absorbed from Bun: bun/src/runtime/webcore/Blob.cp
// Upstream Sync: Refer to bun/src/runtime/webcore/Blob.zig for future upstream updates.
// ============================================================================

const std = @import("std");
import { BlobStore, BlobKind } from "./store";

fn readFileBytesC(path: string): ([]const u8)? {
    if (path.len == 0 || path.len >= 4096) {
        return null;
    }
    let path_buf: [4096]u8 = undefined;
    zig {
        @memcpy(path_buf[0..path.len], path);
        path_buf[path.len] = 0;
    }
    let path_z: [*:0]const u8 = path_buf[0..path.len :0].ptr;

    let z_flags = std.c.O{ .CLOEXEC = true };
    let fd = std.c.open(path_z, z_flags, @as(std.c.mode_t, 0));
    if (fd < 0) {
        return null;
    }

    let stx: std.os.linux.Statx = undefined;
    const mask = std.os.linux.STATX{ .SIZE = true, .MTIME = true };
    const rc = std.c.statx(fd, "", std.os.linux.AT.EMPTY_PATH, mask, &stx);
    if (rc != 0) {
        * = std.c.close(fd);
        return null;
    }

    const file_size = stx.size as usize;
    if (file_size == 0) {
        * = std.c.close(fd);
        return "";
    }

    let buf_ptr: [*]u8 = @alloc(file_size);
    let buf = unsafe { buf_ptr[0..file_size] };
    let total_read: usize = 0;
    while (total_read < file_size) {
        let n = std.c.read(fd, buf[total_read..file_size].ptr, file_size - total_read);
        if (n <= 0) {
            break;
        }
        total_read += (n as usize);
    }
    * = std.c.close(fd);
    return buf[0..total_read];
}

/// 分块流式读取迭代器 (支持 `for (chunk of blob.stream(size))`)
pub struct BlobChunkReader {
    pub blob: Blob,
    pub chunk_size: usize,
    pub cursor: usize = 0,

    pub static fn init(blob: Blob, chunk_size: usize): BlobChunkReader {
        const safe_chunk = if (chunk_size == 0) 65536 else chunk_size;
        return BlobChunkReader{
            blob: blob,
            chunk_size: safe_chunk,
            cursor: 0,
        };
    }

    pub fn next(*_): ([]const u8)? {
        const data = _.blob.bytes();
        if (_.cursor >= data.len) {
            return null;
        }
        const rem = data.len - _.cursor;
        const take = if (rem < _.chunk_size) rem else _.chunk_size;
        const out = data[_.cursor .. _.cursor + take];
        _.cursor += take;
        return out;
    }

    pub fn remaining(): usize {
        if (_.cursor >= _.blob.size) {
            return 0;
        }
        return _.blob.size - _.cursor;
    }

    pub fn reset(*_) {
        _.cursor = 0;
    }
}

pub struct Blob {
    pub size: usize,
    pub type: string,
    pub store: *BlobStore,
    pub offset: usize,

    /// 从内存字节切片构建 Blob
    pub static fn fromBytes(data: []const u8, mime_type: string): Blob {
        return Blob{
            size: data.len,
            type: if (mime_type.len > 0) mime_type else "application/octet-stream",
            store: BlobStore.fromBytes(data),
            offset: 0,
        };
    }

    /// 统一 init 接口
    pub static fn init(data: []const u8, mime_type: string): Blob {
        return Blob.fromBytes(data, mime_type);
    }

    /// 从 UTF-8 文本构建 Blob
    pub static fn fromString(content: string, mime_type: string): Blob {
        return Blob{
            size: content.len,
            type: if (mime_type.len > 0) mime_type else "text/plain;charset=utf-8",
            store: BlobStore.fromBytes(content),
            offset: 0,
        };
    }

    /// 从磁盘文件读取构建 Blob (文件不存在或不可读时安全返回 null)
    pub static fn fromFile(path: string, mime_type: string): Blob? {
        if (content := readFileBytesC(path)) {
            let resolved_mime = if (mime_type.len > 0) mime_type else Blob.guessMime(path);
            return Blob{
                size: content.len,
                type: resolved_mime,
                store: BlobStore.fromFile(path, content),
                offset: 0,
            };
        }
        return null;
    }

    /// 将多段 Blob 合并为一个连续的 Blob
    pub static fn concat(parts: []const Blob, mime_type: string): Blob {
        let total_len: usize = 0;
        let i: usize = 0;
        while (i < parts.len) {
            total_len += parts[i].size;
            i += 1;
        }
        if (total_len == 0) {
            return Blob.fromBytes("", mime_type);
        }
        let out_ptr: [*]u8 = @alloc(total_len);
        let out = unsafe { out_ptr[0..total_len] };
        let pos: usize = 0;
        let p: usize = 0;
        while (p < parts.len) {
            const raw = parts[p].bytes();
            if (raw.len > 0) {
                zig {
                    @memcpy(out[pos .. pos + raw.len], raw);
                }
                pos += raw.len;
            }
            p += 1;
        }
        return Blob.fromBytes(out[0..pos], mime_type);
    }

    /// 获取底层只读字节切片 (零拷贝)
    pub fn bytes(): []const u8 {
        if (_.store.bytes.len == 0) {
            return "";
        }
        let end_idx = _.offset + _.size;
        let actual_end = if (end_idx > _.store.bytes.len) _.store.bytes.len else end_idx;
        if (_.offset >= actual_end) {
            return "";
        }
        return _.store.bytes[_.offset..actual_end];
    }

    /// 获取文本内容
    pub fn text(): string {
        return _.bytes();
    }

    /// 切片借用构造新 Blob (零拷贝借用同一底层存储，兼容原有 usize 签名)
    pub fn slice(start_in: usize, end_in: usize, content_type: string): Blob {
        let start = if (start_in > _.size) _.size else start_in;
        let end = if (end_in > _.size) _.size else end_in;
        let actual_size = if (end > start) (end - start) else 0;

        let new_mime = if (content_type.len > 0) content_type else _.type;
        _.store.ref();

        return Blob{
            size: actual_size,
            type: new_mime,
            store: _.store,
            offset: _.offset + start,
        };
    }

    /// 支持负索引与反转范围的 W3C 标准切片 (e.g. `sliceSigned(-4, -1, "")`)
    pub fn sliceSigned(start_in: isize, end_in: isize, content_type: string): Blob {
        const len_signed = _.size as isize;
        let s = if (start_in < 0) (len_signed + start_in) else start_in;
        if (s < 0) { s = 0; }
        if (s > len_signed) { s = len_signed; }

        let e = if (end_in < 0) (len_signed + end_in) else end_in;
        if (e < 0) { e = 0; }
        if (e > len_signed) { e = len_signed; }

        const start_u = s as usize;
        const end_u = e as usize;
        const actual_size = if (end_u > start_u) (end_u - start_u) else 0;

        let new_mime = if (content_type.len > 0) content_type else _.type;
        _.store.ref();

        return Blob{
            size: actual_size,
            type: new_mime,
            store: _.store,
            offset: _.offset + start_u,
        };
    }

    /// 按字节内容比较两个 Blob 是否相等
    pub fn equals(other: Blob): bool {
        if (_.size != other.size) {
            return false;
        }
        if (_.size == 0) {
            return true;
        }
        if (_.store == other.store && _.offset == other.offset) {
            return true;
        }
        const a = _.bytes();
        const b = other.bytes();
        let eq: bool = false;
        zig {
            eq = std.mem.eql(u8, a, b);
        }
        return eq;
    }

    /// 返回按 `chunk_size` 分块读取的流式迭代器
    pub fn stream(chunk_size: usize = 65536): BlobChunkReader {
        return BlobChunkReader.init(_, chunk_size);
    }

    /// 获取独立堆拷贝缓冲区 (ArrayBuffer 语义)
    pub fn arrayBuffer(): []u8 {
        let raw = _.bytes();
        let len = raw.len;
        if (len == 0) {
            return "";
        }
        let out_ptr: [*]u8 = @alloc(len);
        let out = unsafe { out_ptr[0..len] };
        zig {
            @memcpy(out, raw);
        }
        return out;
    }

    /// 释放引用计数与关联资源
    ~init() {
        _.store.deref();
    }

    pub fn deinit(*_) {
        _.~init();
    }

    pub static fn guessMime(path: string): string {
        if (path.endsWith(".png")) return "image/png";
        if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
        if (path.endsWith(".webp")) return "image/webp";
        if (path.endsWith(".bmp")) return "image/bmp";
        if (path.endsWith(".gif")) return "image/gif";
        if (path.endsWith(".ico")) return "image/x-icon";
        if (path.endsWith(".svg")) return "image/svg+xml";
        if (path.endsWith(".json")) return "application/json";
        if (path.endsWith(".txt") || path.endsWith(".log")) return "text/plain";
        if (path.endsWith(".html") || path.endsWith(".htm")) return "text/html";
        if (path.endsWith(".css")) return "text/css";
        if (path.endsWith(".js")) return "application/javascript";
        return "application/octet-stream";
    }
}
