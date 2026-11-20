package me.madmagic.rsstcinterrupterapp.telemetryValues;

import org.json.JSONException;
import org.json.JSONObject;

public abstract class TelemetryEntry {
    protected static final int TYPE_NUMBER = 0;
    protected static final int TYPE_BOOLEAN = 1;
    protected static final int TYPE_STRING = 2;

    public final String name;
    public final int type;

    protected TelemetryEntry(JSONObject definition, int type) throws JSONException {
        name = definition.getString("name");
        this.type = type;
        updateValue(definition);
    }

    public static TelemetryEntry create(JSONObject definition) throws JSONException {
        int type = definition.has("type") ? definition.getInt("type") : TYPE_NUMBER;
        switch (type) {
            case TYPE_NUMBER:
                return new NumberTelemetry(definition);
            case TYPE_BOOLEAN:
                return new BooleanTelemetry(definition);
            case TYPE_STRING:
                return new StringTelemetry(definition);
            default:
                throw new JSONException("Unknown telemetry type: " + type);
        }
    }

    public final void setValue(JSONObject update) throws JSONException {
        int updateType = update.has("type") ? update.getInt("type") : TYPE_NUMBER;
        if (updateType != type) {
            throw new JSONException("Telemetry update type does not match definition for " + name);
        }
        updateValue(update);
    }

    protected abstract void updateValue(JSONObject data) throws JSONException;

    public abstract String getDisplayValue();
}
