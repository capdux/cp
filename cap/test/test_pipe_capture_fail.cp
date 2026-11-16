// Negative test: CP does not support Zig pipe capture syntax '|val|' or '|*val|'
fn main() {
    let opt: i32? = 10;
    if (*val := opt) {
        val = 20;
    }
}
