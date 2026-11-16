// cap/test/test_fuzz_driver.cp
// Tests libFuzzer C-ABI driver integration and crash reproducer

import { print } from "cap:io";

fuzz "protocol payload validator" (input: []const u8) {
    if (input.len >= 5 && input[0] == 67 && input[1] == 82 && input[2] == 65 && input[3] == 83 && input[4] == 72) { // "CRASH"
        @panic("libFuzzer detected target crash on 'CRASH' signature!");
    }
}

fn main() {
    print("test_fuzz_driver: normal execution passed.\n");
}
