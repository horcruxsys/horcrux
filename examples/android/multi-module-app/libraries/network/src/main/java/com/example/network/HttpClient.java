package com.example.network;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

/**
 * Simple HTTP client demonstrating the network library module.
 * Used by the main app module via Horcrux module dependency.
 */
public class HttpClient {

    private static final int TIMEOUT_MS = 5000;

    public String get(String urlString) throws IOException {
        URL url = new URL(urlString);
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setRequestMethod("GET");
        connection.setConnectTimeout(TIMEOUT_MS);
        connection.setReadTimeout(TIMEOUT_MS);

        StringBuilder response = new StringBuilder();
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(connection.getInputStream()))) {
            String line;
            while ((line = reader.readLine()) != null) {
                response.append(line).append('\n');
            }
        } finally {
            connection.disconnect();
        }
        return response.toString();
    }
}
