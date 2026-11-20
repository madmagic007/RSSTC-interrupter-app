package me.madmagic.rsstcinterrupterapp;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import me.madmagic.rsstcinterrupterapp.controlEntries.ControlEntry;

import java.util.List;
import java.util.Map;
import java.util.function.BiConsumer;

public class MyAdapter extends BaseAdapter {

    public final Context context;
    public final List<ControlEntry> items;
    public final Map<String, ControlEntry> map;

    public final BiConsumer<String, Integer> callback;

    public MyAdapter(Context context, List<ControlEntry> items,
                     Map<String, ControlEntry> map, BiConsumer<String, Integer> callback) {
        this.context = context;
        this.items = items;
        this.map = map;
        this.callback = callback;
    }

    @Override
    public int getViewTypeCount() {
        return ControlEntry.TYPES_COUNT;
    }

    @Override
    public int getItemViewType(int position) {
        return items.get(position).getViewType();
    }

    @Override
    public int getCount() {
        return items.size();
    }

    @Override
    public Object getItem(int position) {
        return items.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ControlEntry entry = items.get(position);

        if (convertView == null) {
            convertView = LayoutInflater.from(context)
                    .inflate(entry.getLayoutId(), parent, false);
        }

        entry.bindView(convertView, callback);

        return convertView;
    }

    public void refreshVisibleEntry(String name, ListView listView) {
        ControlEntry entry = map.get(name);
        if (entry == null) return;

        int firstVisiblePosition = listView.getFirstVisiblePosition();
        int lastVisiblePosition = listView.getLastVisiblePosition();
        for (int position = firstVisiblePosition;
             position <= lastVisiblePosition && position < items.size();
             position++) {
            if (items.get(position) != entry) continue;

            View row = listView.getChildAt(position - firstVisiblePosition);
            if (row != null) {
                entry.bindView(row, callback);
            }
            return;
        }
    }
}
