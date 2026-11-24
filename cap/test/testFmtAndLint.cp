// Official formatter & linter verification test (CP v1.0.0)
struct Counter {
    value: i32,
    step: i32,
}

fn advance(c: Counter, rounds: u32) -> i32 {
    let total: i32 = c.value;
    let i: u32 = 0;
    while (i < rounds) {
        if (i % 2 == 0) {
            total += c.step;
        } else {
            total += 1;
        }
        i += 1;
    }
    return total;
}

fn main() {
    let c = Counter { value: 10, step: 3 };
    let res = advance(c, 4);
    if (res != 18) {
        @panic("unexpected advance result");
    }
}
