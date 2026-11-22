fn foo(flag: bool): i32 {
    const val = while (true) {
        if (flag) break 42;
        break 7;
    };
    return val;
}

fn find_first_gt(limit: i32): i32 {
    let i: i32 = 0;
    const found = while (i < 10) : (i += 1) {
        if (i > limit) break i * 10;
    } else -1;
    return found;
}

fn labeled_while(flag: bool): i32 {
    const res = outer: while (true) {
        if (flag) break :outer 100;
        break :outer 200;
    };
    return res;
}

fn main() {
    if (foo(true) != 42) {
        @panic("foo(true) should be 42");
    }
    if (foo(false) != 7) {
        @panic("foo(false) should be 7");
    }
    if (find_first_gt(5) != 60) {
        @panic("find_first_gt(5) should be 60");
    }
    if (find_first_gt(20) != -1) {
        @panic("find_first_gt(20) should be -1");
    }
    if (labeled_while(true) != 100) {
        @panic("labeled_while(true) should be 100");
    }
    if (labeled_while(false) != 200) {
        @panic("labeled_while(false) should be 200");
    }
}
