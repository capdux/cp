// Type definitions for CP

export class S3Uri {
    style: u8,
    bucket: string,
    key: string,
    region: string,
    valid: bool,
}

export fn eqBytes(a: string, b: string): bool;
export fn startsWith(s: string, prefix: string): bool;
export fn endsWith(s: string, suffix: string): bool;
export fn indexOfByte(s: string, ch: u8): i64;
export fn isValidBucketName(name: string): bool;
export fn stripQueryAndFrag(s: string): string;
export fn parseS3Uri(input: string): S3Uri;
export fn formatS3Uri(bucket: string, key: string, out: []u8): usize;
