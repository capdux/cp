fn extern_any_ptr_sym(comptime name: []const u8): *! {
    return @extern(*!, { name: name });
}

fn extern_const_any_ptr_sym(comptime name: []const u8): *const ! {
    return @extern(*const !, { name: name });
}

fn extern_opt_any_ptr_sym(comptime name: []const u8): *!? {
    return @extern(*!?, { name: name });
}

fn main() {
    const ptr_size: usize = @sizeOf(*!);
    @expect_eq(ptr_size, @sizeOf(usize));

    const opt_ptr_size: usize = @sizeOf(*!?);
    @expect_eq(opt_ptr_size, @sizeOf(usize));

    * = extern_any_ptr_sym;
    * = extern_const_any_ptr_sym;
    * = extern_opt_any_ptr_sym;
}
