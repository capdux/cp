// Type definitions for CP

export class Arn {
    partition: string,
    service: string,
    region: string,
    account_id: string,
    resource: string,
    valid: bool,
}

export fn bytesEq(a: string, b: string): bool;
export fn isValidPartition(partition: string): bool;
export fn parseArn(input: string): Arn;
export fn appendSlice(out: []u8, pos: usize, s: string): i64;
export fn formatArn(arn: Arn, out: []u8): usize;
export fn matchResourcePattern(resource: string, pattern: string): bool;
export fn resourceType(resource: string): string;
