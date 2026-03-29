package com.example.utils;

/**
 * General utility functions shared across app modules.
 * Demonstrates the utils library module used via Horcrux dependencies.
 */
public final class StringUtils {

    private StringUtils() {}

    /** Returns true if the string is null or empty. */
    public static boolean isNullOrEmpty(String value) {
        return value == null || value.isEmpty();
    }

    /** Capitalizes the first letter of the string. */
    public static String capitalize(String value) {
        if (isNullOrEmpty(value)) {
            return value;
        }
        return Character.toUpperCase(value.charAt(0)) + value.substring(1);
    }

    /** Truncates the string to maxLength, appending "..." if needed. */
    public static String truncate(String value, int maxLength) {
        if (isNullOrEmpty(value) || value.length() <= maxLength) {
            return value;
        }
        return value.substring(0, maxLength) + "...";
    }
}
