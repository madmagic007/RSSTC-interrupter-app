package me.madmagic.rsstcinterrupterapp;

import android.view.View;
import org.json.JSONException;
import org.json.JSONObject;

public abstract class ControlEntry {
    public String name;
    public int value;
    
    public View viewEntry;

    public ControlEntry(JSONObject o) throws JSONException {
        name = o.getString("name");
        value = o.getInt("value");
    }

    public void setValue(int value) {
        if (viewEntry == null) return;

        this.value = value;
        handleValueUpdate(value);
    }

    protected abstract void handleValueUpdate(int value);
}
