struct MixedScope {
    val: i32,

    static {
        const SCALE: i32 = 10;

        fn create(v: i32): MixedScope {
            return MixedScope { val: v };
        }

        fn scaleBy(v: i32): i32 {
            return v * SCALE;
        }

        pub fn addRaw(a: i32, b: i32): i32 {
            return a + b;
        }
    }

    fn get(): i32 {
        return val;
    }

    fn addAssign(*_, delta: i32) {
        _.val += delta;
    }
}

class MixedService {
    base: i32;

    static {
        pub fn make(b: i32): MixedService {
            return new MixedService(b);
        }

        pub fn helper(x: i32): i32 {
            return x * 2 + 1;
        }
    }

    pub fn init(*_, b: i32) {
        _.base = b;
    }

    pub fn compute(x: i32): i32 {
        return base + helper(x);
    }
}

enum OpMode: u8 {
    Fast = 1,
    Safe = 2,

    static {
        fn fromByte(b: u8): OpMode {
            if (b == 1) return OpMode.Fast;
            return OpMode.Safe;
        }
    }

    fn code(): u8 {
        return @intFromEnum(_);
    }
}

fn main() {
    let s = MixedScope.create(5);
    s.addAssign(MixedScope.scaleBy(3));
    if (s.get() != 35) std.debug.panic("UnexpectedScopeValue", .{});
    if (MixedScope.get(s) != 35) std.debug.panic("UnexpectedUfcsCall", .{});
    if (MixedScope.addRaw(7, 8) != 15) std.debug.panic("UnexpectedStaticCall", .{});

    const svc = MixedService.make(100);
    if (svc.compute(4) != 109) std.debug.panic("UnexpectedClassStaticBlock", .{});
    if (MixedService.helper(4) != 9) std.debug.panic("UnexpectedClassStaticHelper", .{});

    const mode = OpMode.fromByte(1);
    if (mode.code() != 1) std.debug.panic("UnexpectedEnumStaticBlock", .{});
}
