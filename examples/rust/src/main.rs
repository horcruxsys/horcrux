// examples/rust/src/main.rs
// Entry point for the Rust example binary.

mod greet;

fn main() {
    let message = greet::greet("World");
    println!("{}", message);
}
