import { print } from "cap:io";

@c {
    "stdio.h"
}

pub fn main() {
    * = c.puts("hello from c stdio puts via @c header");
    print("c inline header test passed\n");
}
