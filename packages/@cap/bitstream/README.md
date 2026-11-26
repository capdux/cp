# @cap/bitstream

零堆分配的 CP MSB-first / LSB-first 双模式位级流式读写器 — 洁净室 MIT 实现，对标 Rust `bitstream-io`，支持任意 0..64 位跨字节读写、无损 `peekBits`、字节对齐与 H.264/HEVC Exp-Golomb 指数哥伦布编解码。

Zero-allocation MSB-first & LSB-first `BitReader` and `BitWriter` for CP — clean-room MIT parity with Rust `bitstream-io`.

## 安装 / Install

```bash
cap add @cap/bitstream
```

## API 参考 / API Reference

- **`BitReadResult`**：`{ value: u64, ok: bool }`
- **`BitReader`**：`initMsb(buf)`, `initLsb(buf)`, `bitsRemaining()`, `bitPosition()`, `readBit(*_)`, `readBits(*_, count)`, `peekBits(count)`, `skipBits(*_, count)`, `alignToByte(*_)`, `readUe(*_)`
- **`BitWriter`**：`initMsb(buf)`, `initLsb(buf)`, `bitsWritten()`, `bytesUsed()`, `writeBit(*_, bit)`, `writeBits(*_, value, count)`, `alignToByte(*_)`, `writeUe(*_, value)`

## 使用示例 / Usage

```cp
import { BitWriter, BitReader } from "@cap/bitstream";

let buf: [16]u8 = undefined;
let w = BitWriter.initMsb(buf[0..16]);
* = w.writeBits(5 as u64, 3 as u8);
* = w.writeUe(42 as u64);
* = w.alignToByte();

let r = BitReader.initMsb(buf[0..w.bytesUsed()]);
const a = r.readBits(3 as u8); // value=5, ok=true
const b = r.readUe();          // value=42, ok=true
```

## License

MIT
