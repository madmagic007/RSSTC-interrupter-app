package me.madmagic.rsstcinterrupterapp.telemetryValues;

import org.json.JSONException;
import org.json.JSONObject;

public class StringTelemetry extends TelemetryEntry {
    public String value;

    public StringTelemetry(JSONObject definition) throws JSONException {
        super(definition, TYPE_STRING);
    }

    @Override
    protected void updateValue(JSONObject data) throws JSONException {
        value = data.getString("value");
    }

    @Override
    public String getDisplayValue() {
        return value;
    }
}
