// lib/pdb/index.cp — Windows PDB MSF 7.0 superblock & stream directory metadata parser (cap:pdb)
// Reuses Zig 0.17 std.pdb.SuperBlock magic; zero heap allocation, strict bounds checks.

const std = @import("std");

pub err PdbError {
    Truncated,
    InvalidMagic,
    InvalidBlockSize,
    InvalidFreeBlockMap,
    InvalidBlockCount,
    InvalidBlockMapAddr,
}

pub const MSF_MAGIC: []const u8 = std.pdb.SuperBlock.expect_magic;
pub const MSF_MAGIC_LEN: usize = std.pdb.SuperBlock.expect_magic.len;
pub const SUPERBLOCK_SIZE: usize = MSF_MAGIC_LEN + 24;

pub type StreamType = std.pdb.StreamType;

fn pdbReadU32Le(bytes: []const u8, off: usize): u32 {
    const b0: u32 = bytes[off];
    const b1: u32 = bytes[off + 1];
    const b2: u32 = bytes[off + 2];
    const b3: u32 = bytes[off + 3];
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

pub fn isValidBlockSize(block_size: u32): bool {
    return block_size == 512 || block_size == 1024 || block_size == 2048 || block_size == 4096;
}

pub struct PdbSuperBlock {
    pub block_size: u32 = 0;
    pub free_block_map_block: u32 = 0;
    pub num_blocks: u32 = 0;
    pub num_directory_bytes: u32 = 0;
    pub unknown: u32 = 0;
    pub block_map_addr: u32 = 0;

    pub fn expectedFileSize(): u64 {
        return (_.block_size as u64) * (_.num_blocks as u64);
    }

    pub fn directoryBlockCount(): u32 {
        if (_.block_size == 0 || _.num_directory_bytes == 0) {
            return 0;
        }
        return (_.num_directory_bytes + _.block_size - 1) / _.block_size;
    }

    pub fn blockMapOffset(): u64 {
        return (_.block_map_addr as u64) * (_.block_size as u64);
    }
}

pub fn parsePdbHeader(bytes: []const u8): PdbSuperBlock! {
    if (bytes.len < SUPERBLOCK_SIZE) {
        return error.Truncated;
    }

    let i: usize = 0;
    while (i < MSF_MAGIC_LEN) : (i += 1) {
        if (bytes[i] != MSF_MAGIC[i]) {
            return error.InvalidMagic;
        }
    }

    const block_size = pdbReadU32Le(bytes, MSF_MAGIC_LEN);
    const free_block_map_block = pdbReadU32Le(bytes, MSF_MAGIC_LEN + 4);
    const num_blocks = pdbReadU32Le(bytes, MSF_MAGIC_LEN + 8);
    const num_directory_bytes = pdbReadU32Le(bytes, MSF_MAGIC_LEN + 12);
    const unknown = pdbReadU32Le(bytes, MSF_MAGIC_LEN + 16);
    const block_map_addr = pdbReadU32Le(bytes, MSF_MAGIC_LEN + 20);

    if (!isValidBlockSize(block_size)) {
        return error.InvalidBlockSize;
    }
    if (free_block_map_block != 1 && free_block_map_block != 2) {
        return error.InvalidFreeBlockMap;
    }
    if (num_blocks == 0) {
        return error.InvalidBlockCount;
    }
    if (block_map_addr >= num_blocks) {
        return error.InvalidBlockMapAddr;
    }

    return PdbSuperBlock{
        block_size: block_size,
        free_block_map_block: free_block_map_block,
        num_blocks: num_blocks,
        num_directory_bytes: num_directory_bytes,
        unknown: unknown,
        block_map_addr: block_map_addr,
    };
}

pub fn isPdb(bytes: []const u8): bool {
    const sb = parsePdbHeader(bytes) catch return false;
    return sb.block_size >= 512;
}
