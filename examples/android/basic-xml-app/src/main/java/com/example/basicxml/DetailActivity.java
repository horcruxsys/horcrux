package com.example.basicxml;

import android.os.Bundle;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

public class DetailActivity extends AppCompatActivity {

    public static final String EXTRA_MESSAGE = "com.example.basicxml.MESSAGE";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_detail);

        String message = getIntent().getStringExtra(EXTRA_MESSAGE);
        TextView messageText = findViewById(R.id.text_message);
        if (message != null) {
            messageText.setText(message);
        }
    }
}
