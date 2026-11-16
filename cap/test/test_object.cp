interface trait {
    fn log(msg: string);
}

class MyLogger implements Logger {
    prefix: string,
    fn log(msg: string) {
        @print("LOG: {s}", msg);
    }
}

fn main(): void? {
    let logger: Logger = new MyLogger();
    logger.log("Object model works!");
}