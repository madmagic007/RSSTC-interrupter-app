package me.madmagic.rsstcinterrupterapp.controlEntries;

import android.view.View;
import androidx.appcompat.widget.SwitchCompat;
import me.madmagic.rsstcinterrupterapp.R;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.function.BiConsumer;

public class ToggleCapability extends ControlEntry {

    public ToggleCapability(JSONObject o) throws JSONException {
        super(o);
    }

    @Override
    public int getViewType() {
        return TYPE_TOGGLE;
    }

    @Override
    public int getLayoutId() {
        return R.layout.item_toggle;
    }

    static class ViewHolder {
        SwitchCompat toggle;
        ToggleCapability entry;
        BiConsumer<String, Integer> callback;
        boolean binding;
    }

    @Override
    public void bindView(View view, BiConsumer<String, Integer> callback) {
        ViewHolder holder = (ViewHolder) view.getTag();

        if (holder == null) {
            holder = new ViewHolder();

            holder.toggle = view.findViewById(R.id.itemToggle);

            ViewHolder toggleHolder = holder;
            holder.toggle.setOnCheckedChangeListener((button, checked) -> {
                if (toggleHolder.binding || toggleHolder.entry == null) return;

                int newValue = checked ? 1 : 0;
                if (toggleHolder.entry.value != newValue) {
                    toggleHolder.entry.setValue(newValue);
                    toggleHolder.callback.accept(toggleHolder.entry.name, newValue);
                }
            });

            view.setTag(holder);
        }

        holder.entry = this;
        holder.callback = callback;
        holder.toggle.setText(name);
        holder.binding = true;
        holder.toggle.setChecked(value == 1);
        holder.binding = false;
    }
}
