package com.example.ndk;

import android.os.Bundle;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native_lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView greetingView = findViewById(R.id.text_greeting);
        greetingView.setText(getNativeGreeting());

        TextView resultView = findViewById(R.id.text_result);
        int a = 6;
        int b = 7;
        resultView.setText(getString(R.string.multiply_result, a, b, multiplyNative(a, b)));
    }

    public native String getNativeGreeting();

    public native int multiplyNative(int a, int b);
}
