package me.madmagic.rsstcinterrupterapp.telemetryValues;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;
import me.madmagic.rsstcinterrupterapp.R;

import java.util.List;
import java.util.Map;

public class TelemetryAdapter extends BaseAdapter {

    private final Context context;
    private final List<TelemetryEntry> items;
    private final Map<String, TelemetryEntry> entriesByName;

    public TelemetryAdapter(Context context, List<TelemetryEntry> items, Map<String, TelemetryEntry> entriesByName) {
        this.context = context;
        this.items = items;
        this.entriesByName = entriesByName;
    }

    @Override
    public int getCount() {
        return items.size();
    }

    @Override
    public TelemetryEntry getItem(int position) {
        return items.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View row = convertView;
        if (row == null) {
            row = LayoutInflater.from(context)
                    .inflate(R.layout.item_telemetry, parent, false);
        }

        TelemetryEntry entry = getItem(position);
        ((TextView) row.findViewById(R.id.telemetryName)).setText(entry.name);
        ((TextView) row.findViewById(R.id.telemetryValue)).setText(entry.getDisplayValue());
        return row;
    }

    public void refreshVisibleEntry(String name, ListView listView) {
        TelemetryEntry entry = entriesByName.get(name);
        if (entry == null) return;

        int firstVisiblePosition = listView.getFirstVisiblePosition();
        int lastVisiblePosition = listView.getLastVisiblePosition();
        for (int position = firstVisiblePosition;
             position <= lastVisiblePosition && position < items.size();
             position++) {
            if (items.get(position) != entry) continue;

            View row = listView.getChildAt(position - firstVisiblePosition);
            if (row != null) {
                ((TextView) row.findViewById(R.id.telemetryValue))
                        .setText(entry.getDisplayValue());
            }
            return;
        }
    }
}
