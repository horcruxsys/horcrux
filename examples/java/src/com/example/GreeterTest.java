// examples/java/src/com/example/GreeterTest.java
// Unit test for the Greeter class.
package com.example;

/**
 * Simple test harness (no JUnit dependency) for the Greeter class.
 *
 * Run with: java -cp <jar> com.example.GreeterTest
 */
public class GreeterTest {
    public static void main(final String[] args) {
        Greeter greeter = new Greeter();

        assertEquals("Hello, World!", greeter.greet("World"));
        assertEquals("Hello, Horcrux!", greeter.greet("Horcrux"));
        assertEquals("Hello, !", greeter.greet(""));

        System.out.println("All tests passed.");
    }

    private static void assertEquals(final String expected, final String actual) {
        if (!expected.equals(actual)) {
            throw new AssertionError(
                "Expected: " + expected + " but got: " + actual);
        }
    }
}
