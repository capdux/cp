import { print } from "cap:io";

struct BufferHelper {
    len: usize = 0;

    @inline
    fn inc(*_, amount: usize) {
        len = len + amount;
    }

    @inline
    fn getLen(): usize {
        return len;
    }
}

fn main() {
    let buf = BufferHelper{ len: 5 };
    buf.inc(10);
    if (buf.getLen() != 15) {
        print("Root inline test failed\n");
        return;
    }
    print("Root and inline feature tests passed successfully!\n");
}
