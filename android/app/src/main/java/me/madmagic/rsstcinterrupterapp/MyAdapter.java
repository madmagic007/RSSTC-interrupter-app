package me.madmagic.rsstcinterrupterapp;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;
import com.google.android.material.slider.Slider;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.BiConsumer;

public class MyAdapter extends BaseAdapter {

    public final Context context;
    public final List<Capability> items = new ArrayList<>();
    public final Map<String, Capability> map = new HashMap<>();

    public final BiConsumer<String, Integer> callback;

    public MyAdapter(Context context, BiConsumer<String, Integer> callback) {
        this.context = context;
        this.callback = callback;
    }

    public void addData(JSONObject data) throws JSONException {
        String name = data.optString("name");
        if (map.containsKey(name)) return;

        Capability cap = new Capability(data);
        map.put(name, cap);
        items.add(cap);

        notifyDataSetChanged();
    }

    public void updateData(String name, int value) {
        Capability cap = map.get(name);
        if (cap == null) return;

        cap.setValue(value);
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

    private final Handler handler = new Handler(Looper.getMainLooper());
    private Integer pendingValue;
    private boolean sendScheduled;

    private void queueSliderValue(String name, int value) {
        pendingValue = value;

        if (sendScheduled) return;
        sendScheduled = true;

        handler.postDelayed(() -> {
            sendScheduled = false;

            if (pendingValue == null) return;

            int valueToSend = pendingValue;
            pendingValue = null;

            callback.accept(name, valueToSend);

            if (pendingValue != null) {
                queueSliderValue(name, pendingValue);
            }
        }, 100);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        if (convertView == null) {
            convertView = LayoutInflater.from(context).inflate(R.layout.item_slider, parent, false);
        }

        Capability cap = items.get(position);
        if (cap == null) return convertView;

        TextView itemName = convertView.findViewById(R.id.itemName);
        itemName.setText(cap.name);

        Slider slider = convertView.findViewById(R.id.slider);
        slider.setValueFrom(cap.min);
        slider.setValueTo(cap.max);
        slider.setValue(cap.value);
        cap.slider = slider;

        slider.addOnChangeListener((slider1, v, b) -> {
            if (!b) return;

            queueSliderValue(cap.name, (int) v);
        });

        slider.addOnSliderTouchListener(new Slider.OnSliderTouchListener() {
            @Override
            public void onStartTrackingTouch(Slider slider) {}

            @Override
            public void onStopTrackingTouch(Slider slider) {
                int value = (int) slider.getValue();

                queueSliderValue(cap.name, value);
            }
        });

        return convertView;
    }

    public void clear() {
        map.clear();
        items.clear();
        notifyDataSetChanged();
    }
}
