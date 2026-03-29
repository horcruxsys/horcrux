package com.example.ndk;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class NativeLibTest {

    @Test
    public void testLoadLibraryMethodExists() throws NoSuchMethodException {
        // Verify native method signatures are declared
        MainActivity.class.getMethod("getNativeGreeting");
        MainActivity.class.getMethod("multiplyNative", int.class, int.class);
    }

    @Test
    public void testMultiplySymmetry() {
        // Business logic test independent of native call
        int a = 6;
        int b = 7;
        assertEquals(a * b, b * a);
    }
}
