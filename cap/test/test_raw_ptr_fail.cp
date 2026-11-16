import { print } from "cap:io";

fn test_unbounded(*ptr: [*]u8!) {
    ptr[10] = 99;
}

fn main() {
    print("fail\n");
}
