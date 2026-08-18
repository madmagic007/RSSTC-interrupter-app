package me.madmagic.rsstcinterrupterapp;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;
import com.google.android.material.slider.Slider;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.BiConsumer;

public class MyAdapter extends BaseAdapter {

    public final Context context;
    public final List<JSONObject> items = new ArrayList<>();
    public final Map<String, JSONObject> map = new HashMap<>();

    public final BiConsumer<String, Integer> callback;

    public MyAdapter(Context context, BiConsumer<String, Integer> callback) {
        this.context = context;
        this.callback = callback;
    }

    public void addData(JSONObject data) {
        String name = data.optString("name");

        if (map.containsKey(name)) return;

        map.put(name, data);
        items.add(data);
        notifyDataSetChanged();
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
        if (convertView == null) {
            convertView = LayoutInflater.from(context).inflate(R.layout.item_slider, parent, false);
        }

        JSONObject data = items.get(position);
        String name = data.optString("name");

        TextView itemName = convertView.findViewById(R.id.itemName);
        itemName.setText(name);

        Slider slider = convertView.findViewById(R.id.slider);
        slider.setValueFrom(data.optInt("min"));
        slider.setValueTo(data.optInt("max"));
        slider.setValue(data.optInt("value"));

        slider.addOnChangeListener((slider1, v, b) -> {
            if (!b) return;

            callback.accept(name, (int) v);
        });

        return convertView;
    }

    public void clear() {
        map.clear();
        items.clear();
        notifyDataSetChanged();
    }
}
