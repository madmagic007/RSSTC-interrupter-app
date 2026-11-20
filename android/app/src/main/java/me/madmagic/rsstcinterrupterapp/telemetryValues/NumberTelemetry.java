package me.madmagic.rsstcinterrupterapp.telemetryValues;

import org.json.JSONException;
import org.json.JSONObject;

public class NumberTelemetry extends TelemetryEntry {
    public long value;

    public NumberTelemetry(JSONObject definition) throws JSONException {
        super(definition, TYPE_NUMBER);
    }

    @Override
    protected void updateValue(JSONObject data) throws JSONException {
        value = data.getLong("value");
    }

    @Override
    public String getDisplayValue() {
        return String.valueOf(value);
    }
}
