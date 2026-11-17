// Source: Custom (cap:test harness, per CHANGELOG)
/**
 * CP Test SDK (cap:test)
 * Zero-overhead native test runner
 */

pub class Expectation {
    actual: i32, // placeholder, wait, I can just use macros directly in the test!
}

pub fn test(name: string, callback: auto) {
    @test(name, callback);
}

pub fn expect_eq(actual: auto, expected: auto) {
    @expect_eq(actual, expected);
}
