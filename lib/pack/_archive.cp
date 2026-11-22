// Source: Absorbed from /data/dev/bun/src/libarchive/ | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/pack/_archive.cp — Libarchive C-FFI Bindings & Constants
// Absorbed from Bun src/libarchive_sys/bindings.cp
// ============================================================================

pub @opaque struct struct_archive {}
pub @opaque struct struct_archive_entry {}

pub const la_int64_t = i64;
pub const la_ssize_t = isize;

pub enum ArchiveResult: i32 {
    Eof = 1,
    Ok = 0,
    Retry = -10,
    Warn = -20,
    Failed = -25,
    Fatal = -30,

    pub static fn isOk(rc: i32): bool {
        return rc >= -20;
    }
}

pub enum ArchiveFileType: u32 {
    Regular = 0o100000,
    Symlink = 0o120000,
    Socket = 0o140000,
    CharDevice = 0o020000,
    BlockDevice = 0o060000,
    Directory = 0o040000,
    Fifo = 0o010000,
}

pub enum ArchiveFormat: i32 {
    Cpio = 0x10000,
    Shar = 0x20000,
    Tar = 0x30000,
    TarUstar = 0x30001,
    TarPaxInterchange = 0x30002,
    TarPaxRestricted = 0x30003,
    TarGnutar = 0x30004,
    Iso9660 = 0x40000,
    Zip = 0x50000,
    Empty = 0x60000,
    Ar = 0x70000,
    Mtree = 0x80000,
    Raw = 0x90000,
    Xar = 0xA0000,
    SevenZip = 0xE0000,
    Warc = 0xF0000,
}

pub enum ArchiveFilter: i32 {
    None = 0,
    Gzip = 1,
    Bzip2 = 2,
    Compress = 3,
    Program = 4,
    Lzma = 5,
    Xz = 6,
    Uu = 7,
    Rpm = 8,
    Lzip = 9,
    Lrzip = 10,
    Lzop = 11,
    Grzip = 12,
    Lz4 = 13,
    Zstd = 14,
}

pub @c {
    fn archive_read_new(): (*struct_archive)?;
    fn archive_read_free(a: *struct_archive): i32;
    fn archive_read_close(a: *struct_archive): i32;
    fn archive_read_support_format_all(a: *struct_archive): i32;
    fn archive_read_support_format_tar(a: *struct_archive): i32;
    fn archive_read_support_format_gnutar(a: *struct_archive): i32;
    fn archive_read_support_format_zip(a: *struct_archive): i32;
    fn archive_read_support_filter_all(a: *struct_archive): i32;
    fn archive_read_support_filter_gzip(a: *struct_archive): i32;
    fn archive_read_support_filter_zstd(a: *struct_archive): i32;
    fn archive_read_open_memory(a: *struct_archive, buff: *const !?, size: usize): i32;
    fn archive_read_next_header(a: *struct_archive, entry: **struct_archive_entry): i32;
    fn archive_read_data(a: *struct_archive, buff: *!?, len: usize): isize;
    fn archive_entry_pathname(entry: *struct_archive_entry): [*c]const u8;
    fn archive_entry_size(entry: *struct_archive_entry): i64;
    fn archive_entry_filetype(entry: *struct_archive_entry): u32;
    fn archive_error_string(a: *struct_archive): [*c]const u8;
}
