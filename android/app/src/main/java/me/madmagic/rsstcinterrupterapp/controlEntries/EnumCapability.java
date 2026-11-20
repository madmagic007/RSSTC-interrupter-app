package me.madmagic.rsstcinterrupterapp.controlEntries;

import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.TextView;
import androidx.appcompat.widget.AppCompatSpinner;
import me.madmagic.rsstcinterrupterapp.R;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.function.BiConsumer;

public class EnumCapability extends ControlEntry {

    private final List<String> values = new ArrayList<>();

    public EnumCapability(JSONObject o) throws JSONException {
        super(o);
        JSONArray options = o.getJSONArray("options");
        for (int i = 0; i < options.length(); i++) {
            values.add(options.getString(i));
        }
        if (values.isEmpty()) {
            throw new JSONException("Enum capability must have at least one option");
        }
    }

    @Override
    public boolean isUserValuePersistable(int value) {
        return value >= 0 && value < values.size();
    }

    @Override
    public int getViewType() {
        return TYPE_ENUM;
    }

    @Override
    public int getLayoutId() {
        return R.layout.item_spinner;
    }

    static class ViewHolder {
        TextView itemName;
        AppCompatSpinner spinner;
        EnumCapability entry;
        ArrayAdapter<String> optionsAdapter;
        BiConsumer<String, Integer> callback;
        boolean suppressSelectionCallback;
    }

    @Override
    public void bindView(View view, BiConsumer<String, Integer> callback) {
        ViewHolder holder = (ViewHolder) view.getTag();

        if (holder == null) {
            holder = new ViewHolder();

            holder.itemName = view.findViewById(R.id.itemName);
            holder.spinner = view.findViewById(R.id.itemSpinner);

            holder.optionsAdapter = new ArrayAdapter<>(
                    view.getContext(),
                    android.R.layout.simple_spinner_item,
                    new ArrayList<>(values));
            holder.optionsAdapter.setDropDownViewResource(
                    android.R.layout.simple_spinner_dropdown_item);
            holder.spinner.setAdapter(holder.optionsAdapter);

            ViewHolder spinnerHolder = holder;
            holder.spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent, View itemView, int position, long id) {
                    if (!spinnerHolder.suppressSelectionCallback && spinnerHolder.entry != null && spinnerHolder.entry.value != position) {
                        spinnerHolder.entry.setValue(position);
                        spinnerHolder.callback.accept(spinnerHolder.entry.name, position);
                    }
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {}
            });

            view.setTag(holder);
        }

        holder.callback = callback;
        holder.suppressSelectionCallback = true;
        if (holder.entry != this) {
            holder.optionsAdapter.clear();
            holder.optionsAdapter.addAll(values);
            holder.entry = this;
        }

        holder.itemName.setText(name);
        holder.spinner.setSelection(Math.max(0, Math.min(value, values.size() - 1)));
        holder.suppressSelectionCallback = false;
    }
}
