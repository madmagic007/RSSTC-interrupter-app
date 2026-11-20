package me.madmagic.rsstcinterrupterapp.controlEntries;

import android.view.View;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.function.BiConsumer;

public abstract class ControlEntry {

    protected static final int TYPE_NUMBER = 0;
    protected static final int TYPE_TOGGLE = 1;
    protected static final int TYPE_BUTTON = 2;
    protected static final int TYPE_ENUM = 3;
    public static final int TYPES_COUNT = 4;

    public String name;
    public int value;

    public static ControlEntry create(JSONObject o) throws JSONException {
        switch (o.getInt("type")) {
            case TYPE_NUMBER: return new NumberCapability(o);
            case TYPE_TOGGLE: return new ToggleCapability(o);
            case TYPE_BUTTON: return new ButtonCapability(o);
            case TYPE_ENUM: return new EnumCapability(o);
            default: return null;
        }
    }

    public ControlEntry(JSONObject o) throws JSONException {
        name = o.getString("name");
        value = o.getInt("value");
    }

    public void setValue(int value) {
        this.value = value;
    }

    public boolean isUserValuePersistable(int value) {
        return true;
    }

    public abstract int getViewType();
    public abstract int getLayoutId();
    public abstract void bindView(View view, BiConsumer<String, Integer> callback);
}
