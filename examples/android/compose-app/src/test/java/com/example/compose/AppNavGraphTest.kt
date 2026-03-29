package com.example.compose

import org.junit.Assert.assertNotNull
import org.junit.Test

class AppNavGraphTest {

    @Test
    fun testAppNavGraphFunctionExists() {
        // Verify AppNavGraph composable function is accessible
        val method = Class.forName("com.example.compose.AppNavGraphKt")
            .methods
            .firstOrNull { it.name == "AppNavGraph" }
        assertNotNull("AppNavGraph composable function must exist", method)
    }
}
