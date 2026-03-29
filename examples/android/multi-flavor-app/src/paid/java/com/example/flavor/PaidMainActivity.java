package com.example.flavor;

public class PaidMainActivity extends MainActivity {

    @Override
    protected String getFlavorFeatures() {
        return getString(R.string.features_paid);
    }
}
