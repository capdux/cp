import { test, expect_eq } from "cap:test";

pub struct Sample {
    x: i32,

    // 省略只读接收者，方法体内写 * = _; 返回 type
    fn parentType(): type {
        * = _;
        return Sample;
    }

    // 省略只读接收者，方法体内写 * = _; 返回普通值
    fn constantValue(): i32 {
        * = _;
        return 42;
    }

    // 省略只读接收者且带其它参数，方法体内写 * = _;
    fn addOffset(delta: i32): i32 {
        * = _;
        return delta + 1;
    }

    // 显式可变接收者，方法体内写 * = _;
    fn noopMut(*_) {
        * = _;
    }
}

fn checkParentType(): i32 {
    const cs = Sample{ x: 10 };
    const T = cs.parentType();
    const s2 = T{ x: 20 };
    return s2.x;
}

fn main() {
    test("discard underscore receiver in methods", () => {
        expect_eq(checkParentType(), 20);
        let s = Sample{ x: 10 };
        expect_eq(s.constantValue(), 42);
        expect_eq(s.addOffset(6), 7);
        s.noopMut();

        let local_val: i32 = 99;
        * = local_val;
    });
}
