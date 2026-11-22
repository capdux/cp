// Source: Absorbed from /data/dev/bun/src/http/H2FrameParser.cp | Synced: 2026-11-22T11:12:13Z
import frame from "./frame.cp";

export const CLIENT_PREFACE = frame.CLIENT_PREFACE;
export const FRAME_HEADER_SIZE = frame.FRAME_HEADER_SIZE;
export const SETTINGS_UNIT_SIZE = frame.SETTINGS_UNIT_SIZE;
export const STREAM_PRIORITY_SIZE = frame.STREAM_PRIORITY_SIZE;
export const MAX_WINDOW_SIZE = frame.MAX_WINDOW_SIZE;
export const MAX_HEADER_TABLE_SIZE = frame.MAX_HEADER_TABLE_SIZE;
export const MAX_STREAM_ID = frame.MAX_STREAM_ID;
export const MAX_FRAME_SIZE = frame.MAX_FRAME_SIZE;
export const DEFAULT_WINDOW_SIZE = frame.DEFAULT_WINDOW_SIZE;
export const DEFAULT_MAX_FRAME_SIZE = frame.DEFAULT_MAX_FRAME_SIZE;
export const H2FrameError = frame.H2FrameError;
export const FrameType = frame.FrameType;
export const FLAG_ACK = frame.FLAG_ACK;
export const FLAG_END_STREAM = frame.FLAG_END_STREAM;
export const FLAG_END_HEADERS = frame.FLAG_END_HEADERS;
export const FLAG_PADDED = frame.FLAG_PADDED;
export const FLAG_PRIORITY = frame.FLAG_PRIORITY;
export const ErrorCode = frame.ErrorCode;
export const SettingsType = frame.SettingsType;
export const UInt31WithReserved = frame.UInt31WithReserved;
export const FrameHeader = frame.FrameHeader;
export const StreamPriority = frame.StreamPriority;
export const SettingsUnit = frame.SettingsUnit;
export const hasClientPreface = frame.hasClientPreface;
