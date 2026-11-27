# @cap/snappy@1.2.1 (Agent Quickref)

> Google Snappy 1.2.1 fast compression/decompression library (Kafka/Parquet/LevelDB/RocksDB) for CP | License: BSD-3-Clause
> Import: `import { ... } from "@cap/snappy";` | Full types: `src/index.d.cp`

## Functions
- `export fn snappy_compress(input: [*]&u8!, input_length: usize, compressed: [*]u8!, compressed_length: *usize!): i32;`
- `export fn snappy_uncompress(compressed: [*]&u8!, compressed_length: usize, uncompressed: [*]u8!, uncompressed_length: *usize!): i32;`
- `export fn snappy_max_compressed_length(source_length: usize): usize;`
- `export fn snappy_uncompressed_length(compressed: [*]&u8!, compressed_length: usize, result: *usize!): i32;`
- `export fn snappy_validate_compressed_buffer(compressed: [*]&u8!, compressed_length: usize): i32;`
- `export fn maxCompressedLength(source_len: usize): usize;`
- `export fn uncompressedLength(compressed: &[]u8): !usize;`
- `export fn compressInto(src: &[]u8, dst: []u8): !&[]u8;`
- `export fn uncompressInto(compressed: &[]u8, dst: []u8): !&[]u8;`
- `export fn validateCompressedBuffer(compressed: &[]u8): bool;`
