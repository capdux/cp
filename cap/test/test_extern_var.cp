// Regression test for extern const and extern var declarations
const StaticStringImpl = anyopaque;

@c extern const IPv4: StaticStringImpl;
@c extern const IPv6: StaticStringImpl;

extern const ExternalFlag: i32;
extern let ExternalCounter: u32;

@c {
    extern const BlockConst: i32;
    let BlockVar: u32;
}

pub fn main() {
    // Verified AST and codegen compilation of extern declarations
}
