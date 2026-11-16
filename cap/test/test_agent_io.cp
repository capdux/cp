import { Agent, SystemAction } from "cap:beam";

class IoAgent extends Agent {
    async fn onMessage(action_id: u32, payload: string) {
        @print("IoAgent: Requesting file read...");
        let data = await @fs_read_async("./hello.txt");
        @print("IoAgent: File read complete. Content: {s}", data);
    }
}

fn main() {}
