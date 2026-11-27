// @cap/arn-parse — Zero-heap AWS ARN parser, formatter & IAM wildcard matcher
// MIT License (clean-room CP implementation)

pub struct Arn {
    pub partition: string,
    pub service: string,
    pub region: string,
    pub account_id: string,
    pub resource: string,
    pub valid: bool,
}

fn bytesEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn isValidPartition(partition: string) -> bool {
    return bytesEq(partition, "aws")
        || bytesEq(partition, "aws-cn")
        || bytesEq(partition, "aws-us-gov")
        || bytesEq(partition, "aws-iso")
        || bytesEq(partition, "aws-iso-b");
}

pub fn parseArn(input: string) -> Arn {
    const fail = Arn {
        partition: "",
        service: "",
        region: "",
        account_id: "",
        resource: "",
        valid: false,
    };

    if (input.len < 9 || !bytesEq(input[0..4], "arn:")) {
        return fail;
    }

    let c1: usize = 4;
    while (c1 < input.len && input[c1] != ':' as u8) {
        c1 += 1;
    }
    if (c1 >= input.len) {
        return fail;
    }

    let c2: usize = c1 + 1;
    while (c2 < input.len && input[c2] != ':' as u8) {
        c2 += 1;
    }
    if (c2 >= input.len) {
        return fail;
    }

    let c3: usize = c2 + 1;
    while (c3 < input.len && input[c3] != ':' as u8) {
        c3 += 1;
    }
    if (c3 >= input.len) {
        return fail;
    }

    let c4: usize = c3 + 1;
    while (c4 < input.len && input[c4] != ':' as u8) {
        c4 += 1;
    }
    if (c4 >= input.len) {
        return fail;
    }

    const partition: string = input[4..c1];
    const service: string = input[(c1 + 1)..c2];
    const region: string = input[(c2 + 1)..c3];
    const account_id: string = input[(c3 + 1)..c4];
    const resource: string = input[(c4 + 1)..input.len];

    if (partition.len == 0 || service.len == 0 || resource.len == 0) {
        return fail;
    }

    return Arn {
        partition: partition,
        service: service,
        region: region,
        account_id: account_id,
        resource: resource,
        valid: true,
    };
}

fn appendSlice(out: []u8, pos: usize, s: string) -> i64 {
    if (s.len > out.len - pos) {
        return -1;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos + i] = s[i];
        i += 1;
    }
    return (pos + s.len) as i64;
}

pub fn formatArn(arn: Arn, out: []u8) -> usize {
    if (arn.partition.len == 0 || arn.service.len == 0 || arn.resource.len == 0) {
        return 0;
    }
    let p: i64 = appendSlice(out, 0, "arn:");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, arn.partition);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, ":");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, arn.service);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, ":");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, arn.region);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, ":");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, arn.account_id);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, ":");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, arn.resource);
    if (p < 0) {
        return 0;
    }
    return p as usize;
}

pub fn matchResourcePattern(resource: string, pattern: string) -> bool {
    let ri: usize = 0;
    let pi: usize = 0;
    let star_pi: usize = 0;
    let star_ri: usize = 0;
    let has_star: bool = false;

    while (ri < resource.len) {
        if (pi < pattern.len && (pattern[pi] == '?' as u8 || pattern[pi] == resource[ri])) {
            ri += 1;
            pi += 1;
        } else if (pi < pattern.len && pattern[pi] == '*' as u8) {
            has_star = true;
            star_pi = pi;
            star_ri = ri;
            pi += 1;
        } else if (has_star) {
            star_ri += 1;
            ri = star_ri;
            pi = star_pi + 1;
        } else {
            return false;
        }
    }

    while (pi < pattern.len && pattern[pi] == '*' as u8) {
        pi += 1;
    }
    return pi == pattern.len;
}

pub fn resourceType(resource: string) -> string {
    let i: usize = 0;
    while (i < resource.len) {
        const c: u8 = resource[i];
        if (c == '/' as u8 || c == ':' as u8) {
            if (i == 0) {
                return "";
            }
            return resource[0..i];
        }
        i += 1;
    }
    return "";
}
