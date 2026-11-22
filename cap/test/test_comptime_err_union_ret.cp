import { test, expect_eq } from "cap:test";

err MyErr {
    Fail,
}

fn Res(comptime T: type): type {
    return T ! MyErr;
}

fn AnyRes(comptime T: type): type {
    return T!;
}

fn makeOk(): Res(i32) {
    return 42;
}

fn makeErr(): Res(i32) {
    return err.Fail;
}

fn makeAnyOk(): AnyRes(i32) {
    return 7;
}

fn runResOk(): i32 {
    return makeOk() catch 0;
}

fn runResErr(): i32 {
    return makeErr() catch -1;
}

fn runAnyOk(): i32 {
    return makeAnyOk() catch 0;
}

fn main() {
    test("comptime function returning error union type T ! MyErr and T!", () => {
        expect_eq(runResOk(), 42);
        expect_eq(runResErr(), -1);
        expect_eq(runAnyOk(), 7);
    });
}
