package com.example.basicxml;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

public class MainActivityTest {

    @Test
    public void testExtraMessageKey() {
        assertNotNull(DetailActivity.EXTRA_MESSAGE);
        assertEquals("com.example.basicxml.MESSAGE", DetailActivity.EXTRA_MESSAGE);
    }
}
