// Attempting to structurally mutate a borrowed collection inside for-loop should fail at compile-time
async fn main() {
    let list: i32[] = [1, 2, 3];
    for (x of list) {
        list.push(4);
    }
}
