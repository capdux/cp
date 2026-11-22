// ============================================================================
// cap/test/testPackBrotliDecompress.cp — RFC 7932 Brotli 解压全量互操作回归测试
// 测试向量均由 Node.js zlib.brotliCompressSync / createBrotliCompress 真实生成
// 覆盖：空串、短文本、q=0/4/5/6/9/11、HTML 静态字典与词变换、JSON、重叠 LZ77、
// UTF-8 中文文本、WBITS=10 小窗口、多 Meta-Block、Metadata 跳过、零堆 Into 接口与安全边界
// ============================================================================

const std = @import("std");
import { test, expect, expect_eq } from "cap:test";
import {
    Brotli,
    BrotliError,
    BrotliHeader,
    brotliWindowSizeFromBits,
    brotliDecompress,
    brotliDecompressWithLimits,
    brotliDecompressInto,
} from "cap:pack";

const BR_EMPTY: [1]u8 = [
    0x3b,
];
const BR_SHORT: [27]u8 = [
    0x0b, 0x0b, 0x80, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x42, 0x72, 0x6f, 0x74, 0x6c, 0x69,
    0x20, 0x52, 0x46, 0x43, 0x20, 0x37, 0x39, 0x33, 0x32, 0x21, 0x03,
];
const BR_Q0: [53]u8 = [
    0x0b, 0x18, 0x80, 0x55, 0x6e, 0x63, 0x6f, 0x6d, 0x70, 0x72, 0x65, 0x73, 0x73, 0x65, 0x64, 0x20,
    0x6f, 0x72, 0x20, 0x71, 0x30, 0x20, 0x73, 0x68, 0x6f, 0x72, 0x74, 0x20, 0x70, 0x61, 0x79, 0x6c,
    0x6f, 0x61, 0x64, 0x20, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x03,
];
const BR_HTML_Q11: [110]u8 = [
    0x1b, 0xb2, 0x00, 0x40, 0x8d, 0x54, 0x35, 0xfe, 0x59, 0xdb, 0x2a, 0xd5, 0x35, 0x14, 0xc5, 0xbe,
    0x4c, 0xd0, 0x10, 0x35, 0x14, 0x45, 0x3e, 0x30, 0x2c, 0x3e, 0xe0, 0x5d, 0x85, 0xbd, 0x41, 0x4d,
    0x0e, 0x1c, 0x2e, 0x8d, 0xf3, 0xf4, 0x81, 0xdb, 0x7e, 0x0b, 0x2d, 0xc1, 0xdc, 0xc6, 0xd6, 0x16,
    0x43, 0xa4, 0x11, 0x95, 0x2b, 0x20, 0x61, 0x8e, 0x40, 0x3f, 0x3e, 0x49, 0xb8, 0xab, 0x92, 0x5b,
    0xcf, 0x1c, 0xf9, 0xc9, 0x76, 0x4a, 0xe0, 0x1c, 0x02, 0x77, 0x8b, 0xb9, 0x18, 0x1b, 0x45, 0x58,
    0x52, 0x2c, 0x88, 0xfd, 0x91, 0x11, 0xed, 0xb6, 0xeb, 0x47, 0xf2, 0xd4, 0xa6, 0xe2, 0xe7, 0xf4,
    0x7f, 0x54, 0x36, 0x9d, 0x4e, 0xec, 0x09, 0x82, 0x87, 0xac, 0xd8, 0x08, 0x14, 0x03,
];
const BR_JSON_Q4: [185]u8 = [
    0x1b, 0x2a, 0x05, 0x00, 0xc4, 0xc8, 0x39, 0x95, 0xcd, 0xbd, 0x2e, 0x61, 0x84, 0x1a, 0x81, 0x1b,
    0x17, 0x08, 0x06, 0x91, 0x3b, 0xd7, 0xdf, 0x39, 0xbe, 0xf0, 0x0b, 0x85, 0x21, 0x95, 0x70, 0x5b,
    0x1a, 0x6a, 0x88, 0xe1, 0x09, 0x06, 0x72, 0xb2, 0x97, 0x87, 0x28, 0x98, 0x22, 0xc2, 0xf1, 0x82,
    0xa6, 0xb5, 0xda, 0xae, 0x01, 0x45, 0x91, 0x80, 0x81, 0x97, 0x3b, 0x85, 0x1b, 0xe5, 0x01, 0x28,
    0xec, 0x1a, 0x0c, 0x0e, 0x1c, 0x5d, 0x00, 0x7d, 0x2e, 0x88, 0x5c, 0x50, 0x81, 0x41, 0x6e, 0x65,
    0x1e, 0x68, 0x6c, 0xeb, 0x2b, 0x80, 0x81, 0xe5, 0xb4, 0x51, 0xef, 0x81, 0xb6, 0x75, 0xe7, 0x31,
    0x70, 0x47, 0xed, 0x0f, 0xca, 0x36, 0x2a, 0x72, 0x50, 0xec, 0x77, 0xbb, 0x4c, 0x76, 0xf7, 0xf3,
    0x89, 0x94, 0xd6, 0x94, 0x16, 0x96, 0x4b, 0xfc, 0xa2, 0x26, 0x87, 0x87, 0xae, 0x3f, 0x65, 0x38,
    0xd9, 0x74, 0xbe, 0x9f, 0x59, 0x39, 0x79, 0x06, 0xa1, 0xf2, 0x00, 0x7f, 0x26, 0x1b, 0x88, 0x51,
    0xfc, 0xc6, 0xb7, 0xd2, 0x46, 0x90, 0xa4, 0x52, 0x61, 0x20, 0x5b, 0x65, 0x40, 0x91, 0x2a, 0x80,
    0x6a, 0x55, 0x01, 0x4d, 0xaa, 0x01, 0xba, 0x55, 0x07, 0x0c, 0xa9, 0x01, 0x98, 0x56, 0x93, 0x9e,
    0xd8, 0x2d, 0xb4, 0xa2, 0x4d, 0xdf, 0x33, 0xdf, 0x7f,
];
const BR_JSON_Q6: [184]u8 = [
    0x1b, 0x2a, 0x05, 0x00, 0xc4, 0xe8, 0xd7, 0x7d, 0xaa, 0x6f, 0x36, 0x9f, 0x1c, 0x12, 0x43, 0xc2,
    0xc7, 0x41, 0x82, 0xa0, 0x9d, 0xc0, 0xa9, 0x75, 0xaf, 0x1a, 0x74, 0x17, 0x46, 0xd5, 0x45, 0xd6,
    0x25, 0xc3, 0x01, 0x0d, 0x67, 0x38, 0x19, 0xa0, 0x87, 0x0f, 0xa5, 0xe7, 0x35, 0xcd, 0x0f, 0xa3,
    0x41, 0xf0, 0x6c, 0x42, 0x7f, 0x6f, 0xcc, 0x4b, 0xb8, 0xac, 0x33, 0x0c, 0xf4, 0x35, 0x28, 0xd4,
    0x61, 0xd7, 0xcb, 0x2e, 0xcf, 0x10, 0x4d, 0xf9, 0x83, 0xa3, 0x9f, 0x30, 0xfc, 0x1b, 0x65, 0x02,
    0x83, 0xa3, 0xd0, 0x85, 0x6d, 0x4a, 0x6a, 0x6b, 0x3f, 0x0e, 0xb1, 0x84, 0xf1, 0x52, 0x6e, 0x29,
    0x8c, 0x5b, 0x1a, 0x95, 0x82, 0x59, 0xc6, 0x54, 0x0e, 0xfb, 0xe3, 0x01, 0x03, 0xef, 0xbb, 0x15,
    0xb2, 0xa0, 0x0b, 0x19, 0xc2, 0xb3, 0xe9, 0xc3, 0xc4, 0xdd, 0x6f, 0xd7, 0x53, 0xd5, 0xa2, 0x0c,
    0x37, 0x73, 0xcd, 0xb2, 0x36, 0xec, 0x88, 0xab, 0x26, 0x5e, 0xaa, 0x37, 0xe0, 0x63, 0x3e, 0x4d,
    0xf5, 0x29, 0x02, 0x41, 0x2a, 0x0c, 0x0c, 0x44, 0xab, 0x08, 0x48, 0x52, 0x09, 0x90, 0xad, 0x32,
    0xa0, 0x48, 0x15, 0x40, 0xb5, 0xaa, 0x80, 0x26, 0xd5, 0x00, 0xdd, 0xaa, 0x73, 0xc7, 0x9f, 0x16,
    0x5a, 0x5e, 0x97, 0x3a, 0x4f, 0xf5, 0xf3, 0x02,
];
const BR_OVERLAP_Q11: [30]u8 = [
    0x1b, 0x6b, 0x03, 0xf8, 0xc5, 0x16, 0x87, 0xc8, 0xbf, 0xce, 0x2b, 0x91, 0x91, 0x8b, 0x4d, 0xb6,
    0x5b, 0x1e, 0x80, 0xc0, 0x05, 0x74, 0x55, 0xe3, 0x1e, 0x93, 0xef, 0x3d, 0x28, 0x67,
];
const BR_UTF8_Q9: [131]u8 = [
    0x1b, 0x66, 0x02, 0x00, 0x1c, 0xa9, 0x51, 0x5f, 0xcc, 0xee, 0x40, 0xdc, 0x96, 0xea, 0x42, 0xfe,
    0xc3, 0xb1, 0x2a, 0x48, 0xd7, 0x90, 0x2c, 0x82, 0x20, 0x28, 0x47, 0x43, 0x8c, 0x82, 0xa0, 0xe2,
    0x91, 0x8e, 0x7c, 0x42, 0xb8, 0x26, 0xbd, 0x7d, 0x70, 0x5e, 0xc8, 0x75, 0x72, 0x3a, 0x84, 0x6a,
    0xd8, 0x80, 0x13, 0x90, 0x0c, 0x03, 0xc2, 0x80, 0x06, 0xe5, 0x61, 0x03, 0x4e, 0x38, 0xa9, 0xf5,
    0x9a, 0xbf, 0xa2, 0xff, 0x73, 0x05, 0xf5, 0xcd, 0xff, 0xe0, 0xb6, 0x5d, 0xaf, 0x77, 0xf2, 0x98,
    0x12, 0x4f, 0x30, 0x6a, 0x75, 0xe1, 0xca, 0x7a, 0xa5, 0xde, 0xa9, 0x81, 0x53, 0x72, 0xc2, 0x82,
    0x0a, 0xd1, 0xd7, 0x99, 0x83, 0x3f, 0xb6, 0x38, 0x6d, 0x20, 0xf3, 0xf8, 0x50, 0x78, 0x5d, 0xc2,
    0x1e, 0x3e, 0x85, 0x0c, 0xb8, 0xd9, 0x96, 0xcb, 0xe9, 0x2d, 0x2b, 0x5e, 0x74, 0x6f, 0xdb, 0x4d,
    0x0f, 0x20, 0x3c,
];
const BR_LGWIN10_Q5: [68]u8 = [
    0xa1, 0x68, 0x35, 0x00, 0x20, 0x96, 0xb7, 0x54, 0xef, 0x1b, 0x9b, 0x40, 0x53, 0x14, 0x4d, 0x7e,
    0x9b, 0x6e, 0xfb, 0x99, 0xa2, 0xfc, 0x15, 0xb0, 0x01, 0x07, 0x4e, 0x49, 0x02, 0x93, 0xec, 0xb8,
    0xa0, 0x0f, 0x13, 0x39, 0xb1, 0xb2, 0xef, 0x96, 0xba, 0x08, 0x5e, 0x6a, 0x21, 0xac, 0x3e, 0xa6,
    0xb6, 0x92, 0x53, 0x46, 0xd2, 0x15, 0xfb, 0xd3, 0x3c, 0xa2, 0x97, 0xc8, 0x7d, 0x87, 0xbc, 0x17,
    0x69, 0x4e, 0x81, 0x0c,
];
const BR_MULTI_META: [90]u8 = [
    0x0b, 0x91, 0x00, 0x00, 0x04, 0x6a, 0x72, 0xa4, 0xb7, 0xc5, 0x41, 0x1a, 0x78, 0x02, 0x0f, 0x2d,
    0x7a, 0x0b, 0x26, 0x72, 0xe0, 0x90, 0x45, 0xf4, 0x2d, 0x12, 0x7c, 0x7b, 0x01, 0x57, 0x6d, 0xf6,
    0x95, 0x1c, 0x22, 0xaa, 0x39, 0x52, 0x80, 0x8c, 0x93, 0x1d, 0xc8, 0x8f, 0x9b, 0x0c, 0x21, 0x12,
    0x00, 0x40, 0xdc, 0xc6, 0xd6, 0x75, 0x6c, 0xe7, 0x17, 0x0c, 0xa2, 0x82, 0x5a, 0xc1, 0x06, 0x9c,
    0x06, 0xca, 0x81, 0x6e, 0xb6, 0x2d, 0xcc, 0xc3, 0xc6, 0xd8, 0xd8, 0x22, 0xb6, 0x9a, 0x9b, 0x60,
    0x6b, 0x36, 0xb2, 0x0b, 0xbf, 0x95, 0x39, 0xdb, 0x6d, 0x09,
];

const EXP_SHORT: []const u8 = "Hello, Brotli RFC 7932!";
const EXP_Q0: []const u8 = "Uncompressed or q0 short payload 0123456789abcdef";
const EXP_HTML: []const u8 = "<!DOCTYPE html><html><head><title>CapSurf Brotli Test</title></head><body><h1>Hello World</h1><p>The quick brown fox jumps over the lazy dog. Welcome to the web!</p></body></html>";
const EXP_JSON: []const u8 = "{\"status\":\"ok\",\"encoding\":\"br\",\"items\":[{\"id\":0,\"name\":\"item_0\",\"active\":true,\"description\":\"repeated JSON payload for LZ77 and Huffman testing\"},{\"id\":1,\"name\":\"item_1\",\"active\":false,\"description\":\"repeated JSON payload for LZ77 and Huffman testing\"},{\"id\":2,\"name\":\"item_2\",\"active\":true,\"description\":\"repeated JSON payload for LZ77 and Huffman testing\"},{\"id\":3,\"name\":\"item_3\",\"active\":false,\"description\":\"repeated JSON payload for LZ77 and Huffman testing\"},{\"id\":4,\"name\":\"item_4\",\"active\":true,\"description\":\"repeated JSON payload for LZ77 and Huffman testing\"},{\"id\":5,\"name\":\"item_5\",\"active\":false,\"description\":\"repeated JSON payload for LZ77 and Huffman testing\"},{\"id\":6,\"name\":\"item_6\",\"active\":true,\"description\":\"repeated JSON payload for LZ77 and Huffman testing\"},{\"id\":7,\"name\":\"item_7\",\"active\":false,\"description\":\"repeated JSON payload for LZ77 and Huffman testing\"},{\"id\":8,\"name\":\"item_8\",\"active\":true,\"description\":\"repeated JSON payload for LZ77 and Huffman testing\"},{\"id\":9,\"name\":\"item_9\",\"active\":false,\"description\":\"repeated JSON payload for LZ77 and Huffman testing\"},{\"id\":10,\"name\":\"item_10\",\"active\":true,\"description\":\"repeated JSON payload for LZ77 and Huffman testing\"},{\"id\":11,\"name\":\"item_11\",\"active\":false,\"description\":\"repeated JSON payload for LZ77 and Huffman testing\"}]}";
const EXP_OVERLAP: []const u8 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCXYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345XYZ12345";
const EXP_UTF8: []const u8 = "\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c\xef\xbc\x81CapSurf \xe6\xb5\x8f\xe8\xa7\x88\xe5\x99\xa8\xe5\x86\x85\xe6\xa0\xb8\xe5\x8e\x9f\xe7\x94\x9f\xe6\x94\xaf\xe6\x8c\x81 RFC 7932 Brotli \xe8\xa7\xa3\xe5\x8e\x8b \xe2\x80\x94 UTF-8 \xe5\xa4\x9a\xe5\xad\x97\xe8\x8a\x82\xe6\x96\x87\xe6\x9c\xac\xe4\xb8\x8e\xe4\xb8\xad\xe6\x96\x87\xe6\xa0\x87\xe7\x82\xb9\xe6\xb5\x8b\xe8\xaf\x95\xe3\x80\x82\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c\xef\xbc\x81CapSurf \xe6\xb5\x8f\xe8\xa7\x88\xe5\x99\xa8\xe5\x86\x85\xe6\xa0\xb8\xe5\x8e\x9f\xe7\x94\x9f\xe6\x94\xaf\xe6\x8c\x81 RFC 7932 Brotli \xe8\xa7\xa3\xe5\x8e\x8b \xe2\x80\x94 UTF-8 \xe5\xa4\x9a\xe5\xad\x97\xe8\x8a\x82\xe6\x96\x87\xe6\x9c\xac\xe4\xb8\x8e\xe4\xb8\xad\xe6\x96\x87\xe6\xa0\x87\xe7\x82\xb9\xe6\xb5\x8b\xe8\xaf\x95\xe3\x80\x82\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c\xef\xbc\x81CapSurf \xe6\xb5\x8f\xe8\xa7\x88\xe5\x99\xa8\xe5\x86\x85\xe6\xa0\xb8\xe5\x8e\x9f\xe7\x94\x9f\xe6\x94\xaf\xe6\x8c\x81 RFC 7932 Brotli \xe8\xa7\xa3\xe5\x8e\x8b \xe2\x80\x94 UTF-8 \xe5\xa4\x9a\xe5\xad\x97\xe8\x8a\x82\xe6\x96\x87\xe6\x9c\xac\xe4\xb8\x8e\xe4\xb8\xad\xe6\x96\x87\xe6\xa0\x87\xe7\x82\xb9\xe6\xb5\x8b\xe8\xaf\x95\xe3\x80\x82\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c\xef\xbc\x81CapSurf \xe6\xb5\x8f\xe8\xa7\x88\xe5\x99\xa8\xe5\x86\x85\xe6\xa0\xb8\xe5\x8e\x9f\xe7\x94\x9f\xe6\x94\xaf\xe6\x8c\x81 RFC 7932 Brotli \xe8\xa7\xa3\xe5\x8e\x8b \xe2\x80\x94 UTF-8 \xe5\xa4\x9a\xe5\xad\x97\xe8\x8a\x82\xe6\x96\x87\xe6\x9c\xac\xe4\xb8\x8e\xe4\xb8\xad\xe6\x96\x87\xe6\xa0\x87\xe7\x82\xb9\xe6\xb5\x8b\xe8\xaf\x95\xe3\x80\x82\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c\xef\xbc\x81CapSurf \xe6\xb5\x8f\xe8\xa7\x88\xe5\x99\xa8\xe5\x86\x85\xe6\xa0\xb8\xe5\x8e\x9f\xe7\x94\x9f\xe6\x94\xaf\xe6\x8c\x81 RFC 7932 Brotli \xe8\xa7\xa3\xe5\x8e\x8b \xe2\x80\x94 UTF-8 \xe5\xa4\x9a\xe5\xad\x97\xe8\x8a\x82\xe6\x96\x87\xe6\x9c\xac\xe4\xb8\x8e\xe4\xb8\xad\xe6\x96\x87\xe6\xa0\x87\xe7\x82\xb9\xe6\xb5\x8b\xe8\xaf\x95\xe3\x80\x82";
const EXP_LGWIN10: []const u8 = "Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! Small window (WBITS=10, 1008 bytes) sliding window test! ";
const EXP_MULTI: []const u8 = "Meta-Block #1: alpha beta gamma delta alpha beta gamma delta alpha beta gamma delta alpha beta gamma delta alpha beta gamma delta alpha beta gamma delta alpha beta gamma delta alpha beta gamma delta alpha beta gamma delta alpha beta gamma delta alpha beta gamma delta alpha beta gamma delta Meta-Block #2: epsilon zeta eta theta epsilon zeta eta theta epsilon zeta eta theta epsilon zeta eta theta epsilon zeta eta theta epsilon zeta eta theta epsilon zeta eta theta epsilon zeta eta theta epsilon zeta eta theta epsilon zeta eta theta epsilon zeta eta theta epsilon zeta eta theta ";

fn assertSliceEq(actual: []const u8, expected: []const u8) {
    expect_eq(actual.len, expected.len);
    expect(std.mem.eql(u8, actual, expected));
}

test("brotli: empty stream & frame header compatibility", () => {
    const alloc = std.heap.page_allocator;

    // Node.js empty brotli (0x3b)
    const out1 = brotliDecompress(alloc, BR_EMPTY[0..]) catch unreachable;
    defer alloc.free(out1);
    expect_eq(out1.len, 0 as usize);

    // Minimal WBITS=16 empty frame (0x06)
    const empty_w16: [1]u8 = [0x06];
    const out2 = brotliDecompress(alloc, empty_w16[0..]) catch unreachable;
    defer alloc.free(out2);
    expect_eq(out2.len, 0 as usize);

    // Backward-compatible BrotliHeader & Brotli helpers
    const hdr = BrotliHeader.parse(empty_w16[0..]).?;
    expect_eq(hdr.window_bits, 16 as u8);
    expect(hdr.is_last);
    expect(hdr.is_empty);
    const wsize = brotliWindowSizeFromBits(16) catch unreachable;
    expect_eq(wsize, 65520 as usize);
    expect(Brotli.bound(1024) >= 1024);
    expect(Brotli.isValidQuality(6));
    expect(Brotli.isValidWindowBits(22, false));
});

test("brotli: short & uncompressed q=0 vectors", () => {
    const alloc = std.heap.page_allocator;

    const d_short = brotliDecompress(alloc, BR_SHORT[0..]) catch unreachable;
    defer alloc.free(d_short);
    assertSliceEq(d_short, EXP_SHORT);

    const d_q0 = brotliDecompress(alloc, BR_Q0[0..]) catch unreachable;
    defer alloc.free(d_q0);
    assertSliceEq(d_q0, EXP_Q0);
});

test("brotli: HTML q=11 text mode (static dictionary + transforms + UTF8 context)", () => {
    const alloc = std.heap.page_allocator;

    const d_html = Brotli.decompress(alloc, BR_HTML_Q11[0..]) catch unreachable;
    defer alloc.free(d_html);
    assertSliceEq(d_html, EXP_HTML);
});

test("brotli: JSON q=4 and q=6 (LZ77 + complex Huffman + context maps)", () => {
    const alloc = std.heap.page_allocator;

    const d_j4 = brotliDecompress(alloc, BR_JSON_Q4[0..]) catch unreachable;
    defer alloc.free(d_j4);
    assertSliceEq(d_j4, EXP_JSON);

    const d_j6 = brotliDecompress(alloc, BR_JSON_Q6[0..]) catch unreachable;
    defer alloc.free(d_j6);
    assertSliceEq(d_j6, EXP_JSON);
});

test("brotli: overlapping LZ77 copy q=11 & UTF-8 multibyte text q=9", () => {
    const alloc = std.heap.page_allocator;

    const d_ov = brotliDecompress(alloc, BR_OVERLAP_Q11[0..]) catch unreachable;
    defer alloc.free(d_ov);
    assertSliceEq(d_ov, EXP_OVERLAP);

    const d_utf8 = brotliDecompress(alloc, BR_UTF8_Q9[0..]) catch unreachable;
    defer alloc.free(d_utf8);
    assertSliceEq(d_utf8, EXP_UTF8);
});

test("brotli: WBITS=10 small window & multi-meta-block stream", () => {
    const alloc = std.heap.page_allocator;

    const d_w10 = brotliDecompress(alloc, BR_LGWIN10_Q5[0..]) catch unreachable;
    defer alloc.free(d_w10);
    assertSliceEq(d_w10, EXP_LGWIN10);

    const d_multi = brotliDecompress(alloc, BR_MULTI_META[0..]) catch unreachable;
    defer alloc.free(d_multi);
    assertSliceEq(d_multi, EXP_MULTI);
});

test("brotli: metadata skip block (MNIBBLES=3) + uncompressed block", () => {
    const alloc = std.heap.page_allocator;
    // WBITS=16 (bit0=0)
    // Meta-block 1 (Metadata): ISLAST=0 (bit1=0), MNIBBLES=3 (bits2..3=11), RESERVED=0 (bit4=0),
    //   MSKIPBITS=1 (bits5..6=01), MSKIPBYTES-1=2 (bits7..14 -> 3 skip bytes: 0xAA, 0xBB, 0xCC), align bit15=0
    //   byte0: b0=0, b1=0, b2=1, b3=1, b4=0, b5=1, b6=0, b7=0 -> 0b0010_1100 = 0x2C
    //   byte1: bits8..14 = 0b0000001 (val 2 shifted >> 1 = 1), bit15=0 -> 0x01
    //   skip 3 bytes: 0xAA, 0xBB, 0xCC
    // Meta-block 2 (Uncompressed): ISLAST=0 (bit0=0), MNIBBLES=0 (bits1..2=00), MLEN-1=2 (16 bits: 0x0002),
    //   ISUNCOMPRESSED=1 (bit19=1), pad bits20..23=0
    //   byte5: b0=0, b1..2=00, b3..7 = low 5 bits of 2 (00010) -> 0b00010_000 = 0x10
    //   byte6: bits8..15 = 0x00
    //   byte7: bits16..18 = 000, bit19 = 1, bits20..23 = 0000 -> 0x08
    //   raw 3 bytes: 'O', 'K', '!'
    // Meta-block 3 (Empty Last): ISLAST=1, ISLASTEMPTY=1 -> 0x03
    const meta_stream: [12]u8 = [
        0x2C, 0x01, 0xAA, 0xBB, 0xCC,
        0x10, 0x00, 0x08, 0x4F, 0x4B, 0x21,
        0x03,
    ];
    const decoded = brotliDecompress(alloc, meta_stream[0..]) catch unreachable;
    defer alloc.free(decoded);
    assertSliceEq(decoded, "OK!");
});

fn isBrotliErr(res: []u8 ! BrotliError, expected: BrotliError): bool {
    const val = res catch |e| return e == expected;
    std.heap.page_allocator.free(val);
    return false;
}

fn isBrotliIntoErr(res: usize ! BrotliError, expected: BrotliError): bool {
    * = res catch |e| return e == expected;
    return false;
}

test("brotli: brotliDecompressInto external zero-output-heap slice", () => {
    let out_buf: [2048]u8 = [0; 2048];
    let out_slice: []u8 = out_buf[0..];

    const n_html = brotliDecompressInto(BR_HTML_Q11[0..], &out_slice) catch unreachable;
    assertSliceEq(out_buf[0..n_html], EXP_HTML);

    const n_json = Brotli.decompressInto(BR_JSON_Q6[0..], &out_slice) catch unreachable;
    assertSliceEq(out_buf[0..n_json], EXP_JSON);

    // Buffer too small check
    let tiny: [8]u8 = [0; 8];
    let tiny_s: []u8 = tiny[0..];
    expect(isBrotliIntoErr(brotliDecompressInto(BR_HTML_Q11[0..], &tiny_s), BrotliError.OutputBufferTooSmall));
});

test("brotli: error handling & resource limits", () => {
    const alloc = std.heap.page_allocator;

    // Empty slice -> UnexpectedEof
    const empty_in: []const u8 = "";
    expect(isBrotliErr(brotliDecompress(alloc, empty_in), BrotliError.UnexpectedEof));

    // Invalid WBITS (0x11)
    const bad_wbits: [2]u8 = [0x11, 0x00];
    expect(isBrotliErr(brotliDecompress(alloc, bad_wbits[0..]), BrotliError.InvalidWindowBits));

    // Truncated stream -> UnexpectedEof
    expect(isBrotliErr(brotliDecompress(alloc, BR_JSON_Q6[0..10]), BrotliError.UnexpectedEof));

    // WindowTooLarge limit
    expect(isBrotliErr(
        brotliDecompressWithLimits(alloc, BR_HTML_Q11[0..], 512, 65536),
        BrotliError.WindowTooLarge,
    ));

    // OutputTooLarge limit
    expect(isBrotliErr(
        brotliDecompressWithLimits(alloc, BR_JSON_Q6[0..], 16777216, 64),
        BrotliError.OutputTooLarge,
    ));
});
