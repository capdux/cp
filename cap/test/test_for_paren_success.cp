import { print } from "cap:io";

fn main() {
    let sum = 0;
    for (i of 0..10) {
        sum += i;
    }
    print(sum);
}
