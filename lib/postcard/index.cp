// Origin: Pure-CP Stdlib (Zero-Alloc Postcard Embedded Wire Codec + COBS) | Created: 2026-11-22

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
