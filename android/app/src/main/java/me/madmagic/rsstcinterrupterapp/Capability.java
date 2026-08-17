package me.madmagic.rsstcinterrupterapp;

import org.json.JSONException;
import org.json.JSONObject;

public class Capability {
    public String name;
    public int value, min, max;

    public Capability (JSONObject o) throws JSONException {
        name = o.getString("name");
        value = o.getInt("value");
        min = o.getInt("min");
        max = o.getInt("max");
    }
}
