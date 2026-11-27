// Type definitions for CP

import {  } from "std";
export fn cp_hb_version_string(): [*:0]&u8!;
export fn cp_hb_buffer_create(): *auto!?;
export fn cp_hb_buffer_destroy(buf: *auto!?);
export fn cp_hb_buffer_reset(buf: *auto!?);
export fn cp_hb_buffer_add_utf8(buf: *auto!?, text: [*]&u8!, len: i32);
export fn cp_hb_buffer_guess_segment_properties(buf: *auto!?);
export fn cp_hb_buffer_get_length(buf: *auto!?): u32;
export fn cp_hb_buffer_get_direction(buf: *auto!?): i32;
export error HarfBuzzError { BufferCreateFailed }

export fn versionString(): string;
export class HbBuffer {
    raw: *auto!?,
    fn create(): !HbBuffer;
    fn addUtf8(text: string);
    fn guessSegmentProperties();
    fn length(): u32;
    fn direction(): i32;
    fn reset();
    fn __cp_deinit(_: &mut HbBuffer);
}

