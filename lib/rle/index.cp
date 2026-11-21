// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Multi-Standard RLE Suite: PackBits, PCX, TGA, Zero-RLE)

export {
    PACKBITS_NOP,
    PCX_RUN_MASK,
    PCX_MAX_RUN,
    TGA_RLE_FLAG,
    TGA_MAX_PACKET,
    ZERO_RLE_MAX_RUN,
    packbitsMaxEncodedSize,
    packbitsEncode,
    packbitsDecode,
    pcxEncode,
    pcxDecode,
    tgaRleEncode,
    tgaRleDecode,
    zeroRleEncode,
    zeroRleDecode,
} from "./rle.cp";
