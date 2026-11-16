import { Agent, SystemAction } from "cap:beam";
import { print } from "cap:io";

class MyAgent extends Agent {
    fn onMessage(action_id: u32, payload: auto) {
        @print("MyAgent onMessage: {d}", action_id);
    }
}

fn main() {}
