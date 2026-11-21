// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ISO BMFF / MP4 Box Container Parser)

import * as mp4 from "./mp4.cp";

pub const MP4_BOX_HEADER_SIZE = mp4.MP4_BOX_HEADER_SIZE;
pub const MP4_LARGE_HEADER_SIZE = mp4.MP4_LARGE_HEADER_SIZE;
pub const MP4_UUID_SIZE = mp4.MP4_UUID_SIZE;
pub const Mp4Box = mp4.Mp4Box;
pub const FtypBox = mp4.FtypBox;
pub const MvhdBox = mp4.MvhdBox;
pub const MdhdBox = mp4.MdhdBox;
pub const TkhdBox = mp4.TkhdBox;
pub const BoxCursor = mp4.BoxCursor;
pub const fourccEquals = mp4.fourccEquals;
pub const findBox = mp4.findBox;
pub const findBoxPath = mp4.findBoxPath;
pub const parseFtyp = mp4.parseFtyp;
pub const isMp4 = mp4.isMp4;
pub const parseMvhd = mp4.parseMvhd;
pub const parseMdhd = mp4.parseMdhd;
pub const parseTkhd = mp4.parseTkhd;
