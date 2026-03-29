package com.example.flavor;

import android.os.Bundle;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView flavorText = findViewById(R.id.text_flavor);
        flavorText.setText(getString(R.string.flavor_greeting, getString(R.string.flavor_name)));

        TextView featuresText = findViewById(R.id.text_features);
        featuresText.setText(getFlavorFeatures());
    }

    protected String getFlavorFeatures() {
        return getString(R.string.features_base);
    }
}
