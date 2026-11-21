# `cap:thumbhash` — ThumbHash 超紧凑图片占位符编解码器

源自 Bun `runtime/image/thumbhash.cp`（Evan Wallace ThumbHash 算法），纯 CP 实现 ~21–25 字节低频 DCT 图片占位符编解码、`approxAspectRatio` 零分配宽高比提取、`averageRGBA` 主色提取，以及带 `~init()` 确定性 RAII 释放的 `DecodedImage`。

## 导出符号

- `MAX_LEN`(25)、`ThumbhashError`、`DecodedImage`（含 `~init()` RAII）
- `encode(out, w, h, rgba): ([]u8)!`
- `decode(allocator, hash): DecodedImage!`
- `approxAspectRatio(hash): f32!`
- `averageRGBA(hash): [4]u8!`
