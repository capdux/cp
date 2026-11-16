import { spawn } from "cap:parallel";

async fn main() {
    let x = 100;
    let b = &mut x;
    let task = spawn(async () => {
        b.* = 200;
    });
}
