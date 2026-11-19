import { Method } from "cap:http";

pub fn main()! {
    let m = Method.GET;
    if (m == Method.GET) {
        @print("PASS: on-demand cap:http Method resolution\n");
    }
}
