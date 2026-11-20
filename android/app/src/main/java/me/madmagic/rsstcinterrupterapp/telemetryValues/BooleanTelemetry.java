package me.madmagic.rsstcinterrupterapp.telemetryValues;

import org.json.JSONException;
import org.json.JSONObject;

public class BooleanTelemetry extends TelemetryEntry {
    public boolean value;

    public BooleanTelemetry(JSONObject definition) throws JSONException {
        super(definition, TYPE_BOOLEAN);
    }

    @Override
    protected void updateValue(JSONObject data) throws JSONException {
        value = data.getBoolean("value");
    }

    @Override
    public String getDisplayValue() {
        return String.valueOf(value);
    }
}
