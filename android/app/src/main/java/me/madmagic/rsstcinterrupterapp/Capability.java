package me.madmagic.rsstcinterrupterapp;

import com.google.android.material.slider.Slider;
import org.json.JSONException;
import org.json.JSONObject;

public class Capability {
    public String name;
    public int value, min, max;
    public Slider slider;

    public Capability (JSONObject o) throws JSONException {
        name = o.getString("name");
        value = o.getInt("value");
        min = o.getInt("min");
        max = o.getInt("max");
    }

    public void setValue(int value) {
        if (slider == null) return;

        slider.setValue(value);
    }
}
