package com.example.basicxml;

import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView titleText = findViewById(R.id.text_title);
        titleText.setText(R.string.app_name);

        Button detailButton = findViewById(R.id.button_detail);
        detailButton.setOnClickListener(v -> {
            Intent intent = new Intent(this, DetailActivity.class);
            intent.putExtra(DetailActivity.EXTRA_MESSAGE, getString(R.string.detail_message));
            startActivity(intent);
        });
    }
}
