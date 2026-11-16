// ============================================================================
// Absorbed from Bun: bun/src/runtime/webcore/Blob.cp
// Upstream Sync: Refer to bun/src/runtime/webcore/Blob.zig for future upstream updates.
// ============================================================================

import { BlobStore, BlobKind } from "./store";
import { readFileSync } from "cap:fs";

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

    /// 从磁盘文件读取构建 Blob
    pub static fn fromFile(path: string, mime_type: string): Blob? {
        let content = readFileSync(path);
        let resolved_mime = if (mime_type.len > 0) mime_type else Blob.guessMime(path);
        return Blob{
            size: content.len,
            type: resolved_mime,
            store: BlobStore.fromFile(path, content),
            offset: 0,
        };
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

    /// 切片借用构造新 Blob (零拷贝借用同一底层存储)
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

    /// 获取独立堆拷贝缓冲区 (ArrayBuffer 语义)
    pub fn arrayBuffer(): []u8 {
        let raw = _.bytes();
        let len = raw.len;
        if (len == 0) {
            return "";
        }
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];
        for (let i in 0..len) {
            out[i] = raw[i];
        }
        return out;
    }

    /// 释放引用计数与关联资源
    pub fn release(*_) {
        _.store.deref();
    }

    pub fn deinit(*_) {
        _.release();
    }

    static fn guessMime(path: string): string {
        if (path.endsWith(".png")) return "image/png";
        if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
        if (path.endsWith(".webp")) return "image/webp";
        if (path.endsWith(".bmp")) return "image/bmp";
        if (path.endsWith(".gif")) return "image/gif";
        if (path.endsWith(".json")) return "application/json";
        if (path.endsWith(".txt") || path.endsWith(".log")) return "text/plain";
        if (path.endsWith(".html") || path.endsWith(".htm")) return "text/html";
        if (path.endsWith(".css")) return "text/css";
        if (path.endsWith(".js")) return "application/javascript";
        return "application/octet-stream";
    }
}
