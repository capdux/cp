// Source: cp/lib/pack/zip_writer.cp (PKWARE APPNOTE 6.3.10 ZIP Writer)
// ============================================================================
// lib/pack/zip_writer.cp — PKWARE APPNOTE 6.3.10 工业级流式/内存 ZIP 归档写入器
// 支持 Stored / Deflate 条目、IEEE CRC-32、UTF-8 标志位 (bit 11)、目录条目与 EOCD 生成
// ============================================================================

import { Crc32 } from "./checksum.cp";
import { deflateRaw, inflateRaw } from "./gzip.cp";
import {
    ZIP_LOCAL_HEADER_SIZE,
    ZIP_CENTRAL_DIR_SIZE,
    ZIP_EOCD_SIZE,
    ZIP_METHOD_STORE,
    ZIP_METHOD_DEFLATE,
    ZipLocalHeader,
    ZipCentralHeader,
    ZipEocd,
    ZipEntryView,
} from "./zip.cp";

pub const ZIP_FLAG_UTF8: u16 = 0x0800;
pub const ZIP_EXT_ATTR_DIR: u32 = 0x41ED0010;
pub const ZIP_EXT_ATTR_FILE: u32 = 0x81A40000;

/// PKZIP 条目压缩算法枚举
pub enum ZipCompressionMethod: u16 {
    Stored = 0,
    Deflate = 8,
}

/// 待写入 ZIP 归档的条目配置描述符
pub struct ZipWriteEntry {
    name: []const u8 = "",
    data: []const u8 = "",
    method: ZipCompressionMethod = ZipCompressionMethod.Stored,
    level: i32 = 6,
    mtime: u16 = 0,
    mdate: u16 = 0,
    external_attrs: u32 = ZIP_EXT_ATTR_FILE,
    utf8: bool = true,
    is_directory: bool = false,
}

/// PKWARE APPNOTE 6.3.10 内存 ZIP 归档构造器
/// 自动维护 Local File Header、Central Directory 与 End of Central Directory (EOCD)
pub struct ZipWriter {
    local_buf: []u8,
    local_len: usize = 0,
    cd_buf: []u8,
    cd_len: usize = 0,
    entry_count: u16 = 0,
    comment: []const u8 = "",

    pub static fn init(): ZipWriter {
        const init_local_cap: usize = 1024;
        const init_cd_cap: usize = 512;
        const lraw: [*]u8 = @alloc(init_local_cap);
        const craw: [*]u8 = @alloc(init_cd_cap);
        return ZipWriter {
            local_buf: lraw[0..init_local_cap],
            local_len: 0,
            cd_buf: craw[0..init_cd_cap],
            cd_len: 0,
            entry_count: 0,
            comment: "",
        };
    }

    fn ensureLocalCap(*_, extra: usize) {
        const needed = _.local_len + extra;
        if (needed <= _.local_buf.len) {
            return;
        }
        let next_cap = _.local_buf.len * 2;
        while (next_cap < needed) {
            next_cap = next_cap * 2;
        }
        const raw: [*]u8 = @alloc(next_cap);
        let new_buf = raw[0..next_cap];
        if (_.local_len > 0) {
            @memcpy(new_buf[0.._.local_len], _.local_buf[0.._.local_len]);
        }
        _.local_buf = new_buf;
    }

    fn ensureCdCap(*_, extra: usize) {
        const needed = _.cd_len + extra;
        if (needed <= _.cd_buf.len) {
            return;
        }
        let next_cap = _.cd_buf.len * 2;
        while (next_cap < needed) {
            next_cap = next_cap * 2;
        }
        const raw: [*]u8 = @alloc(next_cap);
        let new_buf = raw[0..next_cap];
        if (_.cd_len > 0) {
            @memcpy(new_buf[0.._.cd_len], _.cd_buf[0.._.cd_len]);
        }
        _.cd_buf = new_buf;
    }

    /// 设置归档末尾 ZIP 注释（最大 65535 字节）
    pub fn setComment(*_, comment: []const u8): bool {
        if (comment.len > 65535) {
            return false;
        }
        _.comment = comment;
        return true;
    }

    /// 返回当前已写入的条目数量
    pub fn count(): u16 {
        return _.entry_count;
    }

    /// 写入一个 Stored（未压缩）常规文件条目
    pub fn addStored(*_, name: []const u8, data: []const u8): bool {
        return _.addEntry(name, data, ZipCompressionMethod.Stored, 0);
    }

    /// 写入一个 Deflate（RFC 1951 裸 Deflate 流）压缩文件条目
    pub fn addDeflate(*_, name: []const u8, data: []const u8, level: i32 = 6): bool {
        return _.addEntry(name, data, ZipCompressionMethod.Deflate, level);
    }

    /// 写入一个目录条目（自动补全末尾 '/'，载荷与 CRC32 均置 0）
    pub fn addDirectory(*_, name: []const u8): bool {
        if (name.len == 0 || name.len > 65534 || _.entry_count == 65535) {
            return false;
        }
        let dir_name = name;
        if (name[name.len - 1] != '/') {
            const raw: [*]u8 = @alloc(name.len + 1);
            let tmp = raw[0 .. name.len + 1];
            @memcpy(tmp[0..name.len], name);
            tmp[name.len] = '/';
            dir_name = tmp;
        }
        return _.addWriteEntry(ZipWriteEntry {
            name: dir_name,
            data: "",
            method: ZipCompressionMethod.Stored,
            level: 0,
            mtime: 0,
            mdate: 0,
            external_attrs: ZIP_EXT_ATTR_DIR,
            utf8: true,
            is_directory: true,
        });
    }

    /// 写入指定压缩方式的文件条目
    pub fn addEntry(
        *_,
        name: []const u8,
        data: []const u8,
        method: ZipCompressionMethod,
        level: i32 = 6,
    ): bool {
        return _.addWriteEntry(ZipWriteEntry {
            name: name,
            data: data,
            method: method,
            level: level,
            mtime: 0,
            mdate: 0,
            external_attrs: ZIP_EXT_ATTR_FILE,
            utf8: true,
            is_directory: false,
        });
    }

    /// 使用完整条目配置写入 ZIP 记录
    pub fn addWriteEntry(*_, entry: ZipWriteEntry): bool {
        if (entry.name.len == 0 || entry.name.len > 65535 || _.entry_count == 65535) {
            return false;
        }
        if (entry.data.len > 0xFFFFFFFF || _.local_len > 0xFFFFFFFF) {
            return false;
        }

        const raw_data: []const u8 = if (entry.is_directory) "" else entry.data;
        const crc = Crc32.checksum(raw_data);

        let method_u16: u16 = ZIP_METHOD_STORE;
        let payload: []const u8 = raw_data;

        if (!entry.is_directory && entry.method == ZipCompressionMethod.Deflate) {
            const compressed = deflateRaw(raw_data, entry.level);
            if (compressed.len == 0 && raw_data.len > 0) {
                return false;
            }
            method_u16 = ZIP_METHOD_DEFLATE;
            payload = compressed;
        }

        if (payload.len > 0xFFFFFFFF) {
            return false;
        }

        const flags: u16 = if (entry.utf8) ZIP_FLAG_UTF8 else (0 as u16);
        const local_offset: u32 = _.local_len as u32;

        const lhdr = ZipLocalHeader {
            version_needed: 20,
            flags: flags,
            compression_method: method_u16,
            mtime: entry.mtime,
            mdate: entry.mdate,
            crc32: crc,
            compressed_size: payload.len as u32,
            uncompressed_size: raw_data.len as u32,
            name_len: entry.name.len as u16,
            extra_len: 0,
        };

        const local_entry_size = ZIP_LOCAL_HEADER_SIZE + entry.name.len + payload.len;
        _.ensureLocalCap(local_entry_size);

        const lpos = _.local_len;
        * = lhdr.write(_.local_buf[lpos .. lpos + ZIP_LOCAL_HEADER_SIZE]);
        @memcpy(_.local_buf[lpos + ZIP_LOCAL_HEADER_SIZE .. lpos + ZIP_LOCAL_HEADER_SIZE + entry.name.len], entry.name);
        if (payload.len > 0) {
            @memcpy(_.local_buf[lpos + ZIP_LOCAL_HEADER_SIZE + entry.name.len .. lpos + local_entry_size], payload);
        }
        _.local_len += local_entry_size;

        const chdr = ZipCentralHeader {
            version_made_by: (3 << 8) | 20,
            version_needed: 20,
            flags: flags,
            compression_method: method_u16,
            mtime: entry.mtime,
            mdate: entry.mdate,
            crc32: crc,
            compressed_size: payload.len as u32,
            uncompressed_size: raw_data.len as u32,
            name_len: entry.name.len as u16,
            extra_len: 0,
            comment_len: 0,
            disk_num_start: 0,
            internal_attrs: 0,
            external_attrs: entry.external_attrs,
            local_header_offset: local_offset,
        };

        const cd_entry_size = ZIP_CENTRAL_DIR_SIZE + entry.name.len;
        _.ensureCdCap(cd_entry_size);

        const cpos = _.cd_len;
        * = chdr.write(_.cd_buf[cpos .. cpos + ZIP_CENTRAL_DIR_SIZE]);
        @memcpy(_.cd_buf[cpos + ZIP_CENTRAL_DIR_SIZE .. cpos + cd_entry_size], entry.name);
        _.cd_len += cd_entry_size;

        _.entry_count += 1;
        return true;
    }

    /// 封闭 Central Directory 与 EOCD，输出完整的 PKZIP 二进制切片
    pub fn finish(*_): []u8 {
        const total_len = _.local_len + _.cd_len + ZIP_EOCD_SIZE + _.comment.len;
        const raw: [*]u8 = @alloc(total_len);
        let out = raw[0..total_len];

        let pos: usize = 0;
        if (_.local_len > 0) {
            @memcpy(out[pos .. pos + _.local_len], _.local_buf[0.._.local_len]);
            pos += _.local_len;
        }
        if (_.cd_len > 0) {
            @memcpy(out[pos .. pos + _.cd_len], _.cd_buf[0.._.cd_len]);
            pos += _.cd_len;
        }

        const eocd = ZipEocd {
            disk_num: 0,
            central_dir_disk: 0,
            entries_on_disk: _.entry_count,
            total_entries: _.entry_count,
            central_dir_size: _.cd_len as u32,
            central_dir_offset: _.local_len as u32,
            comment_len: _.comment.len as u16,
        };
        * = eocd.write(out[pos .. pos + ZIP_EOCD_SIZE]);
        pos += ZIP_EOCD_SIZE;

        if (_.comment.len > 0) {
            @memcpy(out[pos .. pos + _.comment.len], _.comment);
            pos += _.comment.len;
        }

        return out[0..pos];
    }

    /// 解压并验证单个 ZipEntryView（同时支持 Stored 与 Deflate 并自动核验 IEEE CRC-32）
    pub static fn decompressAndVerify(entry: ZipEntryView): ([]u8)? {
        if (entry.compression_method == ZIP_METHOD_STORE) {
            if (!entry.verifyCrc()) {
                return null;
            }
            const raw: [*]u8 = @alloc(entry.data.len);
            let out = raw[0..entry.data.len];
            if (entry.data.len > 0) {
                @memcpy(out[0..entry.data.len], entry.data);
            }
            return out;
        }
        if (entry.compression_method == ZIP_METHOD_DEFLATE) {
            const decomp_opt = inflateRaw(entry.data);
            if (decomp_opt == null) {
                return null;
            }
            const decomp = decomp_opt.?;
            if ((decomp.len as u32) != entry.uncompressed_size) {
                return null;
            }
            if (Crc32.checksum(decomp) != entry.crc32) {
                return null;
            }
            return decomp;
        }
        return null;
    }
}
