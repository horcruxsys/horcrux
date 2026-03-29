package com.example.flavor;

public class FreeMainActivity extends MainActivity {

    @Override
    protected String getFlavorFeatures() {
        return getString(R.string.features_free);
    }
}
