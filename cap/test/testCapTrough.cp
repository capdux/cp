import { test, expect_eq } from "cap:test";
import {
    TROUGH_CAPACITY,
    StageOutcome,
    StageFn,
    TagRoute,
    stageOk,
    stageErr,
    stageHalt,
    TroughPipeline,
    dispatchTag,
} from "../../packages/@cap/trough/src/index.cp";

fn addTen(v: i64): StageOutcome {
    return stageOk(v + 10);
}

fn doubleVal(v: i64): StageOutcome {
    return stageOk(v * 2);
}

fn failIfNeg(v: i64): StageOutcome {
    if (v < 0) return stageErr(400);
    return stageOk(v);
}

fn haltOnFortyTwo(v: i64): StageOutcome {
    if (v == 42) return stageHalt(42);
    return stageOk(v + 1);
}

fn main() {
    test("1. @cap/trough — runs middleware pipeline in registration order", () => {
        let pipe = TroughPipeline.new();
        expect_eq(pipe.len(), 0 as usize);
        expect_eq(pipe.use(addTen), true);
        expect_eq(pipe.use(doubleVal), true);
        expect_eq(pipe.len(), 2 as usize);

        const res = pipe.run(5);
        expect_eq(res.ok, true);
        expect_eq(res.halted, false);
        expect_eq(res.err_code, 0 as i32);
        expect_eq(res.value, 30 as i64);
    });

    test("2. @cap/trough — short-circuits immediately on stage error", () => {
        let pipe = TroughPipeline.new();
        * = pipe.use(failIfNeg);
        * = pipe.use(doubleVal);

        const err_res = pipe.run(-3);
        expect_eq(err_res.ok, false);
        expect_eq(err_res.err_code, 400 as i32);

        const ok_res = pipe.run(7);
        expect_eq(ok_res.ok, true);
        expect_eq(ok_res.value, 14 as i64);
    });

    test("3. @cap/trough — supports early halt without error", () => {
        let pipe = TroughPipeline.new();
        * = pipe.use(haltOnFortyTwo);
        * = pipe.use(doubleVal);

        const h = pipe.run(42);
        expect_eq(h.ok, true);
        expect_eq(h.halted, true);
        expect_eq(h.value, 42 as i64);

        const cont = pipe.run(10);
        expect_eq(cont.ok, true);
        expect_eq(cont.halted, false);
        expect_eq(cont.value, 22 as i64);
    });

    test("4. @cap/trough — enforces 32-stage capacity and supports clear()", () => {
        let pipe = TroughPipeline.new();
        let i: usize = 0;
        while (i < TROUGH_CAPACITY) : (i += 1) {
            expect_eq(pipe.use(addTen), true);
        }
        expect_eq(pipe.len(), 32 as usize);
        expect_eq(pipe.use(addTen), false);
        expect_eq(pipe.run(0).value, 320 as i64);

        pipe.clear();
        expect_eq(pipe.len(), 0 as usize);
        expect_eq(pipe.run(99).value, 99 as i64);
    });

    test("5. @cap/trough — dispatchTag routes known tags and wildcard fallback (zwitch)", () => {
        const routes: [4]TagRoute = [
            TagRoute{ tag: "heading", handler_id: 1 },
            TagRoute{ tag: "paragraph", handler_id: 2 },
            TagRoute{ tag: "code", handler_id: 3 },
            TagRoute{ tag: "*", handler_id: 99 },
        ];
        expect_eq(dispatchTag("heading", routes[0..4]), 1 as i32);
        expect_eq(dispatchTag("code", routes[0..4]), 3 as i32);
        expect_eq(dispatchTag("blockquote", routes[0..4]), 99 as i32);
    });

    test("6. @cap/trough — dispatchTag returns -1 when no route or wildcard matches", () => {
        const routes: [2]TagRoute = [
            TagRoute{ tag: "text", handler_id: 10 },
            TagRoute{ tag: "link", handler_id: 20 },
        ];
        expect_eq(dispatchTag("text", routes[0..2]), 10 as i32);
        expect_eq(dispatchTag("image", routes[0..2]), -1 as i32);
    });
}
