// Regression test for TypeScript-style export alias syntax:
//   export { Orig as Alias } from "./mod.cp";
//   export { Orig as Alias };

fn origAdd(a: i32, b: i32): i32 {
    return a + b;
}

const ORIGIN_VAL: i32 = 42;

export { origAdd as aliasedAdd, origAdd, ORIGIN_VAL as ALIASED_VAL };
export { Foo as Bar, Baz, type MyType as AliasedType } from "./mod.cp";

fn main() {
    let sum = aliasedAdd(1, 2);
    let _v = ALIASED_VAL + sum;
}
