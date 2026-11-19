import { Method } from "cap:http";
import { AddressFamily, Ipv4Addr } from "cap:net";

pub fn main()! {
    let m = Method.GET;
    if (m == Method.GET) {
        @print("PASS: simplified cap.json opt & bin on-demand compilation\n");
    }

    let af = AddressFamily.INET;
    if (af == AddressFamily.INET) {
        @print("PASS: simplified cap.json net usockets on-demand compilation\n");
    }

    let ip = Ipv4Addr.init(127, 0, 0, 1);
    if (ip.octets[0] == 127) {
        @print("PASS: Ipv4Addr initialized\n");
    }
}
