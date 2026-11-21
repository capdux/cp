// Source: Absorbed from /data/dev/bun/src/brotli/brotli.cp | Synced: 2026-11-20T05:06:41Z
// ============================================================================
// lib/pack/brotli.cp — Brotli (RFC 7932) 压缩边界计算、帧头解析与流模式配置
// 吸收自 Bun src/brotli/brotli.cp，适配 CP 现代强类型与零拷贝视图
// ============================================================================

import {
    BrotliOperation,
    BrotliDecoderStateStruct,
    BrotliEncoderStateStruct,
    BrotliDecoderCreateInstance,
    BrotliDecoderDestroyInstance,
    BrotliEncoderMaxCompressedSize,
    CompressBoundEstimator,
} from "./_codecs.cp";

pub const BROTLI_MIN_QUALITY: i32 = 0;
pub const BROTLI_MAX_QUALITY: i32 = 11;
pub const BROTLI_DEFAULT_QUALITY: i32 = 11;

pub const BROTLI_MIN_WINDOW_BITS: i32 = 10;
pub const BROTLI_MAX_WINDOW_BITS: i32 = 24;
pub const BROTLI_LARGE_MAX_WINDOW_BITS: i32 = 30;
pub const BROTLI_DEFAULT_WINDOW: i32 = 22;

pub const BROTLI_MIN_INPUT_BLOCK_BITS: i32 = 16;
pub const BROTLI_MAX_INPUT_BLOCK_BITS: i32 = 24;

/// RFC 7932 Brotli 编码器优化模式
pub enum BrotliMode: i32 {
    /// 默认通用模式，不对输入分布做先验假设
    Generic = 0,
    /// 针对 UTF-8 文本优化的上下文建模模式
    Text = 1,
    /// 针对 WOFF 2.0 字体数据优化的压缩模式
    Font = 2,
}

/// Brotli 流式解码器返回状态码（对标 Bun BrotliReaderArrayList）
pub enum BrotliDecoderResult: i32 {
    Error = 0,
    Success = 1,
    NeedsMoreInput = 2,
    NeedsMoreOutput = 3,
}

/// Brotli 流式状态机阶段
pub enum BrotliStreamState: u8 {
    Uninitialized = 0,
    Inflating = 1,
    End = 2,
    Error = 3,
}

/// Brotli 解码器配置参数（对标 Bun DecoderOptions）
pub struct BrotliDecoderOptions {
    large_window: bool = true,
    disable_ring_buffer_reallocation: bool = false,
}

/// Brotli 编码器配置参数
pub struct BrotliEncoderOptions {
    mode: BrotliMode = BrotliMode.Generic,
    quality: i32 = BROTLI_DEFAULT_QUALITY,
    lgwin: i32 = BROTLI_DEFAULT_WINDOW,
    lgblock: i32 = 0,
    large_window: bool = false,
}

/// RFC 7932 Section 9.1 Brotli 流头解析视图
pub struct BrotliHeader {
    window_bits: u8 = 22,
    is_last: bool = false,
    is_empty: bool = false,

    /// 根据 RFC 7932 Section 9.1 规范解码首字节中的滑动窗口位数 (WBITS: 10..24)
    pub static fn decodeWindowBits(first_byte: u8): u8 {
        if ((first_byte & 0x01) == 0) {
            return 16;
        }
        const low3 = (first_byte >> 1) & 0x07;
        if (low3 != 0) {
            return (17 as u8) + low3;
        }
        const mid3 = (first_byte >> 4) & 0x07;
        if (mid3 == 1) {
            return 0; // RFC 7932 保留非法组合
        }
        if (mid3 == 0) {
            return 17;
        }
        return (8 as u8) + mid3;
    }

    /// 解析 Brotli 字节流起始帧头
    pub static fn parse(src: []const u8): BrotliHeader? {
        if (src.len == 0) {
            return null;
        }
        const b0 = src[0];
        const wbits = BrotliHeader.decodeWindowBits(b0);
        if (wbits == 0) {
            return null;
        }

        // 计算 WBITS 占用的比特数 (1, 4, 或 7 bits)
        let wbits_len: u8 = 7;
        if ((b0 & 0x01) == 0) {
            wbits_len = 1;
        } else if (((b0 >> 1) & 0x07) != 0) {
            wbits_len = 4;
        }

        let is_last = false;
        let is_empty = false;
        if (wbits_len == 1) {
            is_last = ((b0 >> 1) & 0x01) != 0;
            if (is_last) {
                is_empty = ((b0 >> 2) & 0x01) != 0;
            }
        } else if (wbits_len == 4) {
            is_last = ((b0 >> 4) & 0x01) != 0;
            if (is_last) {
                is_empty = ((b0 >> 5) & 0x01) != 0;
            }
        } else {
            is_last = ((b0 >> 7) & 0x01) != 0;
            if (is_last && src.len > 1) {
                is_empty = (src[1] & 0x01) != 0;
            }
        }

        return BrotliHeader{
            window_bits: wbits,
            is_last: is_last,
            is_empty: is_empty,
        };
    }
}

/// Brotli 面向对象门面与边界预估器
pub struct Brotli {
    /// 纯算术预估 Brotli 压缩最大输出字节数（对标 BrotliEncoderMaxCompressedSize，零 C 依赖）
    pub static fn bound(source_len: usize): usize {
        return CompressBoundEstimator.brotliBound(source_len);
    }

    /// 计算单次 Brotli 压缩所需的最大目标缓冲区长度
    pub static fn compressBound(source_len: usize): usize {
        return CompressBoundEstimator.brotliBound(source_len);
    }

    /// 校验压缩质量参数是否处于合法区间 [0, 11]
    pub static fn isValidQuality(quality: i32): bool {
        return quality >= BROTLI_MIN_QUALITY && quality <= BROTLI_MAX_QUALITY;
    }

    /// 校验滑动窗口位数 lgwin 是否处于合法区间 [10, 24]（或 Large Window 模式下 [10, 30]）
    pub static fn isValidWindowBits(lgwin: i32, large_window: bool = false): bool {
        const max_w = if (large_window) BROTLI_LARGE_MAX_WINDOW_BITS else BROTLI_MAX_WINDOW_BITS;
        return lgwin >= BROTLI_MIN_WINDOW_BITS && lgwin <= max_w;
    }
}
