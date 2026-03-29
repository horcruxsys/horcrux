// examples/rust/src/greet_test.rs
// Integration test for the greet library.

mod greet;

#[cfg(test)]
mod tests {
    use super::greet;

    #[test]
    fn test_greet_world() {
        assert_eq!(greet::greet("World"), "Hello, World!");
    }

    #[test]
    fn test_greet_empty() {
        assert_eq!(greet::greet(""), "Hello, !");
    }
}
