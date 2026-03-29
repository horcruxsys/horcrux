package com.example.flavor;

import org.junit.Test;

import static org.junit.Assert.assertNotNull;

public class MainActivityTest {

    @Test
    public void testMainActivityExists() throws ClassNotFoundException {
        assertNotNull(Class.forName("com.example.flavor.MainActivity"));
    }

    @Test
    public void testFlavorSubclassesExist() throws ClassNotFoundException {
        assertNotNull(Class.forName("com.example.flavor.FreeMainActivity"));
        assertNotNull(Class.forName("com.example.flavor.PaidMainActivity"));
    }
}
