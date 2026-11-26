// cap/test/testCapHuffman.cp
// Comprehensive unit tests for @cap/huffman (RFC 1951 Canonical Huffman)

import { test, expect_eq } from "cap:test";
import { HuffmanCodec } from "../../packages/@cap/huffman/src/index.cp";

fn buildFreqs(data: string, out_freqs: []u32) {
    let i: usize = 0;
    while (i < 256) {
        out_freqs[i] = 0;
        i += 1;
    }
    i = 0;
    while (i < data.len) {
        out_freqs[data[i] as usize] += 1;
        i += 1;
    }
}

fn main() {
    test("1. @cap/huffman — empty input encode & decode", () => {
        let freqs: [256]u32 = undefined;
        buildFreqs("", freqs[0..256]);
        const codec = HuffmanCodec.fromFrequencies(freqs[0..256]);
        expect_eq(codec.symbol_count, 0 as usize);

        let enc_buf: [16]u8 = undefined;
        let dec_buf: [16]u8 = undefined;
        let empty: [1]u8 = [0];

        const enc_len = codec.encode(empty[0..0], enc_buf[0..16]);
        expect_eq(enc_len, 4 as i64);
        const dec_len = codec.decode(enc_buf[0..4], dec_buf[0..16]);
        expect_eq(dec_len, 0 as i64);
    });

    test("2. @cap/huffman — single-symbol repeated input (1-bit code)", () => {
        const msg = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"; // 32 bytes
        let freqs: [256]u32 = undefined;
        buildFreqs(msg, freqs[0..256]);

        const codec = HuffmanCodec.fromFrequencies(freqs[0..256]);
        expect_eq(codec.symbol_count, 1 as usize);
        expect_eq(codec.codeLength('A' as u8), 1 as u8);

        let enc_buf: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;
        const enc_len = codec.encode(msg, enc_buf[0..64]);
        // 4-byte header + 32 bits (4 bytes) = 8 bytes total!
        expect_eq(enc_len, 8 as i64);

        const dec_len = codec.decode(enc_buf[0..8], dec_buf[0..64]);
        expect_eq(dec_len, 32 as i64);
        expect_eq(dec_buf[0..32], msg);
    });

    test("3. @cap/huffman — highly skewed text achieves strong compression", () => {
        const msg = "aaaaaaaabbbbccdeaaaaaaaabbbbccdeaaaaaaaabbbbccdeaaaaaaaabbbbccde"; // 64 bytes
        let freqs: [256]u32 = undefined;
        buildFreqs(msg, freqs[0..256]);

        const codec = HuffmanCodec.fromFrequencies(freqs[0..256]);
        // 'a' is most frequent -> shortest code (1 bit)
        expect_eq(codec.codeLength('a' as u8), 1 as u8);
        expect_eq(codec.codeLength('e' as u8) > codec.codeLength('a' as u8), true);

        let enc_buf: [128]u8 = undefined;
        let dec_buf: [128]u8 = undefined;
        const enc_len = codec.encode(msg, enc_buf[0..128]);
        expect_eq(enc_len > 4 && enc_len < 32, true);

        const dec_len = codec.decode(enc_buf[0..(enc_len as usize)], dec_buf[0..128]);
        expect_eq(dec_len, 64 as i64);
        expect_eq(dec_buf[0..64], msg);
    });

    test("4. @cap/huffman — all 256 byte symbols round-trip cleanly", () => {
        let raw: [256]u8 = undefined;
        let freqs: [256]u32 = undefined;
        let i: usize = 0;
        while (i < 256) {
            raw[i] = i as u8;
            freqs[i] = 1;
            i += 1;
        }

        const codec = HuffmanCodec.fromFrequencies(freqs[0..256]);
        expect_eq(codec.symbol_count, 256 as usize);
        expect_eq(codec.codeLength(0 as u8), 8 as u8);
        expect_eq(codec.codeLength(255 as u8), 8 as u8);

        let enc_buf: [512]u8 = undefined;
        let dec_buf: [256]u8 = undefined;
        const enc_len = codec.encode(raw[0..256], enc_buf[0..512]);
        expect_eq(enc_len, 260 as i64); // 4 header + 256 * 8 bits

        const dec_len = codec.decode(enc_buf[0..(enc_len as usize)], dec_buf[0..256]);
        expect_eq(dec_len, 256 as i64);
        expect_eq(dec_buf[0], 0 as u8);
        expect_eq(dec_buf[127], 127 as u8);
        expect_eq(dec_buf[255], 255 as u8);
    });

    test("5. @cap/huffman — fromCodeLengths matches fromFrequencies canonically", () => {
        const msg = "the quick brown fox jumps over the lazy dog and the quick fox";
        let freqs: [256]u32 = undefined;
        buildFreqs(msg, freqs[0..256]);

        const c1 = HuffmanCodec.fromFrequencies(freqs[0..256]);
        const c2 = HuffmanCodec.fromCodeLengths(c1.code_len[0..256]);

        expect_eq(c2.symbol_count, c1.symbol_count);
        let s: usize = 0;
        while (s < 256) {
            expect_eq(c2.codeLength(s as u8), c1.codeLength(s as u8));
            expect_eq(c2.codeBits(s as u8), c1.codeBits(s as u8));
            s += 1;
        }

        let enc_buf: [256]u8 = undefined;
        let dec_buf: [256]u8 = undefined;
        const enc_len = c1.encode(msg, enc_buf[0..256]);
        const dec_len = c2.decode(enc_buf[0..(enc_len as usize)], dec_buf[0..256]);
        expect_eq(dec_len, msg.len as i64);
        expect_eq(dec_buf[0..(dec_len as usize)], msg);
    });

    test("6. @cap/huffman — rejects truncated streams, unregistered symbols & small buffers", () => {
        let freqs: [256]u32 = undefined;
        buildFreqs("aaabbc", freqs[0..256]);
        const codec = HuffmanCodec.fromFrequencies(freqs[0..256]);

        let enc_buf: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;

        // Unregistered symbol 'z' fails encode with -1
        expect_eq(codec.encode("abz", enc_buf[0..64]), -1 as i64);

        // Output buffer too small (< 4 bytes) fails encode with -1
        let tiny: [3]u8 = undefined;
        expect_eq(codec.encode("aaabbc", tiny[0..3]), -1 as i64);

        // Truncated header (< 4 bytes) fails decode with -1
        expect_eq(codec.decode(tiny[0..3], dec_buf[0..64]), -1 as i64);

        // Truncated bitstream payload fails decode with -1
        let bad_stream: [5]u8 = [0, 0, 0, 100, 0xFF]; // claims 100 bytes, only 1 byte payload
        expect_eq(codec.decode(bad_stream[0..5], dec_buf[0..64]), -1 as i64);

        // Destination buffer too small for declared original length
        const enc_len = codec.encode("aaabbc", enc_buf[0..64]);
        expect_eq(codec.decode(enc_buf[0..(enc_len as usize)], tiny[0..3]), -1 as i64);
    });
}
