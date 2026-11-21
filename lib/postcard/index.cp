// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Postcard Embedded Wire Codec + COBS)

export {
    zigzagEncodeI16,
    zigzagDecodeI16,
    zigzagEncodeI32,
    zigzagDecodeI32,
    zigzagEncodeI64,
    zigzagDecodeI64,
    PostcardWriter,
    PostcardReader,
    cobsEncode,
    cobsEncodeFramed,
    cobsDecode,
} from "./postcard.cp";
