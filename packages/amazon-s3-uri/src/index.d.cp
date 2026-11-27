// Type definitions for CP

import { strEq, startsWith, endsWith, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class S3Uri {
    style: u8,
    bucket: string,
    key: string,
    region: string,
    valid: bool,
}

export fn indexOfByte(s: string, ch: u8): i64;
export fn isValidBucketName(name: string): bool;
export fn stripQueryAndFrag(s: string): string;
export fn parseS3Uri(input: string): S3Uri;
export fn formatS3Uri(bucket: string, key: string, out: []u8): usize;
