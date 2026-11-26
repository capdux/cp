// @cap/shebang-regex — Zero-allocation shebang (#!) matcher & stripper
// MIT License

pub struct ShebangMatch {
    pub matched: bool,
    pub line_len: usize,
    pub cmd_start: usize,
    pub cmd_len: usize,
}

pub fn hasShebang(input: string): bool {
    return input.len >= 2 && input[0] == 35 && input[1] == 33;
}

pub fn matchShebang(input: string): ShebangMatch {
    if (!hasShebang(input)) {
        return ShebangMatch {
            matched: false,
            line_len: 0,
            cmd_start: 0,
            cmd_len: 0,
        };
    }
    let end: usize = 2;
    while (end < input.len && input[end] != 10 && input[end] != 13) {
        end = end + 1;
    }
    return ShebangMatch {
        matched: true,
        line_len: end,
        cmd_start: 2,
        cmd_len: end - 2,
    };
}

pub fn stripShebang(input: string): usize {
    if (!hasShebang(input)) {
        return 0;
    }
    let end: usize = 2;
    while (end < input.len && input[end] != 10 && input[end] != 13) {
        end = end + 1;
    }
    if (end < input.len && input[end] == 13) {
        end = end + 1;
    }
    if (end < input.len && input[end] == 10) {
        end = end + 1;
    }
    return end;
}
