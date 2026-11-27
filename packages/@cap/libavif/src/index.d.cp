// Type definitions for CP

import {  } from "std";
export fn cp_avif_version(): [*:0]&u8!?;
export fn cp_avif_pixel_format_to_string(fmt: i32): [*:0]&u8!?;
export fn cp_avif_image_create(width: u32, height: u32, depth: u32, yuv_format: i32): *auto!?;
export fn cp_avif_image_destroy(img: *auto!?);
export fn cp_avif_image_width(img: *auto!?): u32;
export fn cp_avif_image_height(img: *auto!?): u32;
export fn cp_avif_image_depth(img: *auto!?): u32;
export fn cp_avif_image_create_and_destroy(width: u32, height: u32, depth: u32): i32;
export error AvifError { InvalidArgument, AllocFailed }

export fn version(): string;
export fn pixelFormatName(fmt: i32): string;
export fn imageCreateAndDestroy(width: u32, height: u32, depth: u32): !void;
export class AvifImage {
    raw: *auto!?,
    fn create(w: u32, h: u32, d: u32, yuv_format: i32): !AvifImage;
    fn width(): u32;
    fn height(): u32;
    fn depth(): u32;
    fn __cp_deinit(_: &mut AvifImage);
}

