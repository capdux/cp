import { compress2, uncompress, compressBound } from "cap:pack";

pub fn main()! {
    let original = "Hello CP! High-performance embedded zlib static compression from vendor/zlib.";
    let orig_len = original.len;

    let max_comp_len = compressBound(orig_len);
    let comp_buf: [256]u8 = undefined;
    let comp_len: usize = 256;

    let res = compress2(&comp_buf[0], &comp_len, original.ptr, orig_len, 6);
    if (res != 0) {
        @print("compress2 failed with code\n");
        return;
    }

    let decomp_buf: [256]u8 = undefined;
    let decomp_len: usize = 256;
    let un_res = uncompress(&decomp_buf[0], &decomp_len, &comp_buf[0], comp_len);
    if (un_res != 0) {
        @print("uncompress failed with code\n");
        return;
    }

    if (decomp_len == orig_len) {
        @print("PASS: zlib static compress & uncompress roundtrip successful!\n");
    }
}
