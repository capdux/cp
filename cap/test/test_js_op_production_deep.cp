import { test, expect_eq } from "cap:test";
import { AssignTarget, Code, Level, opInfo } from "cap:js";

test("1. Prefix, postfix, and unary classification", () => {
    expect_eq(Code.un_pos.isPrefix(), true);
    expect_eq(Code.un_typeof.isPrefix(), true);
    expect_eq(Code.un_pre_inc.isPrefix(), true);
    expect_eq(Code.un_post_inc.isPrefix(), false);
    expect_eq(Code.un_post_inc.isPostfix(), true);
    expect_eq(Code.un_post_dec.isUnary(), true);
    expect_eq(Code.bin_add.isUnary(), false);
    expect_eq(Code.bin_add.isBinary(), true);
});

test("2. Left-associative vs right-associative vs non-associative", () => {
    expect_eq(Code.bin_add.isLeftAssociative(), true);
    expect_eq(Code.bin_mul.isLeftAssociative(), true);
    expect_eq(Code.bin_logical_and.isLeftAssociative(), true);

    // Exponentiation (**) and all assignments are right-associative
    expect_eq(Code.bin_pow.isLeftAssociative(), false);
    expect_eq(Code.bin_pow.isRightAssociative(), true);
    expect_eq(Code.bin_assign.isRightAssociative(), true);
    expect_eq(Code.bin_nullish_coalescing_assign.isRightAssociative(), true);

    // Comma is non-associative in this table
    expect_eq(Code.bin_comma.isLeftAssociative(), false);
    expect_eq(Code.bin_comma.isRightAssociative(), false);
});

test("3. Assignment target classification", () => {
    expect_eq(Code.un_pre_inc.unaryAssignTarget() == AssignTarget.update, true);
    expect_eq(Code.un_post_dec.unaryAssignTarget() == AssignTarget.update, true);
    expect_eq(Code.un_not.unaryAssignTarget() == AssignTarget.none, true);

    expect_eq(Code.bin_assign.binaryAssignTarget() == AssignTarget.replace, true);
    expect_eq(Code.bin_add_assign.binaryAssignTarget() == AssignTarget.update, true);
    expect_eq(Code.bin_logical_or_assign.binaryAssignTarget() == AssignTarget.update, true);
    expect_eq(Code.bin_add.binaryAssignTarget() == AssignTarget.none, true);
});

test("4. Precedence Level ordering", () => {
    expect_eq(Level.lowest.lt(.comma), true);
    expect_eq(Level.assign.lt(.conditional), true);
    expect_eq(Level.logical_or.lt(.logical_and), true);
    expect_eq(Level.add.lt(.multiply), true);
    expect_eq(Level.multiply.lt(.exponentiation), true);
    expect_eq(Level.prefix.lt(.postfix), true);
    expect_eq(Level.member.gt(.call), true);
    expect_eq(Level.equals.eql(.equals), true);
    expect_eq(Level.shift.lte(.add), true);
    expect_eq(Level.add.gte(.shift), true);
});

test("5. Operator text and keyword metadata lookup", () => {
    const t_typeof = Code.un_typeof.info();
    expect_eq(std.mem.eql(u8, t_typeof.text, "typeof"), true);
    expect_eq(t_typeof.is_keyword, true);
    expect_eq(t_typeof.level.eql(.prefix), true);

    const t_in = opInfo(.bin_in);
    expect_eq(std.mem.eql(u8, t_in.text, "in"), true);
    expect_eq(t_in.is_keyword, true);
    expect_eq(t_in.level.eql(.compare), true);

    const t_nc = opInfo(.bin_nullish_coalescing_assign);
    expect_eq(std.mem.eql(u8, t_nc.text, "??="), true);
    expect_eq(t_nc.is_keyword, false);
    expect_eq(t_nc.level.eql(.assign), true);
});
