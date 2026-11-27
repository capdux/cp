// Type definitions for CP

export fn snappy_compress(input: [*]&u8!, input_length: usize, compressed: [*]u8!, compressed_length: *usize!): i32;
export fn snappy_uncompress(compressed: [*]&u8!, compressed_length: usize, uncompressed: [*]u8!, uncompressed_length: *usize!): i32;
export fn snappy_max_compressed_length(source_length: usize): usize;
export fn snappy_uncompressed_length(compressed: [*]&u8!, compressed_length: usize, result: *usize!): i32;
export fn snappy_validate_compressed_buffer(compressed: [*]&u8!, compressed_length: usize): i32;
export error SnappyError { InvalidInput, BufferTooSmall }

export fn maxCompressedLength(source_len: usize): usize;
export fn uncompressedLength(compressed: &[]u8): !usize;
export fn compressInto(src: &[]u8, dst: []u8): !&[]u8;
export fn uncompressInto(compressed: &[]u8, dst: []u8): !&[]u8;
export fn validateCompressedBuffer(compressed: &[]u8): bool;
