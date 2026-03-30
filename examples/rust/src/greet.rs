// examples/rust/src/greet.rs
// A simple Rust library exporting a greeting function.

pub fn greet(name: &str) -> String {
    format!("Hello, {}!", name)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_greet() {
        assert_eq!(greet("Horcrux"), "Hello, Horcrux!");
    }
}
