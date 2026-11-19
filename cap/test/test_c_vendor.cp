import { println } from "cap:io";
import { HttpServer } from "cap:http";

pub fn main()! {
    println("Testing vendored C library linkage via cap:http...");
    let server = HttpServer.init();
    println("Successfully instantiated HttpServer with vendored picohttpparser.");
}
