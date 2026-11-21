import { test, expect_eq } from "cap:test";

enum Task {
    Idle,
    Text: string,
    Move: [i32, i32],
}

@refCount(cleanup)
class SharedResource {
    pub ref_count: u32;
    pub cleaned: bool;

    pub fn init() {
        _.ref_count = 1;
        _.cleaned = false;
    }

    pub fn cleanup(*) {
        this.cleaned = true;
    }
}

fn main() {
    test("tagged union payloads and match", () => {
        let t1 = Task.Idle;
        let t2 = Task.Text("hello tagged union");
        let t3 = Task.Move([10, 20]);

        match (t1) {
            Task.Idle => expect_eq(1, 1),
            * => expect_eq(0, 1),
        }

        match (t2) {
            Task.Text(s) => expect_eq(s, "hello tagged union"),
            * => expect_eq(0, 1),
        }

        match (t3) {
            Task.Move(pos) => {
                expect_eq(pos[0], 10);
                expect_eq(pos[1], 20);
            },
            * => expect_eq(0, 1),
        }
    });

    test("refCount decorator on class", () => {
        let res = new SharedResource();
        expect_eq(res.ref_count, 1);
        expect_eq(res.cleaned, false);

        * = res.ref();
        expect_eq(res.ref_count, 2);

        res.deref();
        expect_eq(res.ref_count, 1);
        expect_eq(res.cleaned, false);

        res.deref();
        expect_eq(res.cleaned, true);
    });
}
