package me.madmagic.rsstcinterrupterapp.controlEntries;

import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.TextView;
import com.google.android.material.slider.Slider;
import me.madmagic.rsstcinterrupterapp.R;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.function.BiConsumer;

public class NumberCapability extends ControlEntry {

    private final int min, max;

    public NumberCapability(JSONObject o) throws JSONException {
        super(o);
        min = o.getInt("min");
        max = o.getInt("max");
    }

    @Override
    public int getViewType() {
        return TYPE_NUMBER;
    }

    @Override
    public int getLayoutId() {
        return R.layout.item_slider;
    }

    static class ViewHolder {
        TextView itemName;
        Slider slider;
        NumberCapability entry;
        BiConsumer<String, Integer> callback;
        final Handler handler = new Handler(Looper.getMainLooper());
        Integer lastSentValue;
        Runnable sampleSliderValue;
        boolean trackingTouch;
    }

    @Override
    public void bindView(View view, BiConsumer<String, Integer> callback) {
        ViewHolder holder = (ViewHolder) view.getTag();

        if (holder == null) {
            holder = new ViewHolder();

            holder.itemName = view.findViewById(R.id.itemName);
            holder.slider = view.findViewById(R.id.slider);
            ViewHolder sliderHolder = holder;
            sliderHolder.sampleSliderValue = () -> {
                sendSliderValue(sliderHolder);
                sliderHolder.handler.postDelayed(
                        sliderHolder.sampleSliderValue, 50);
            };
            holder.slider.addOnSliderTouchListener(new Slider.OnSliderTouchListener() {
                @Override
                public void onStartTrackingTouch(Slider slider) {
                    sliderHolder.trackingTouch = true;
                    sliderHolder.handler.removeCallbacks(sliderHolder.sampleSliderValue);
                    sliderHolder.handler.postDelayed(
                            sliderHolder.sampleSliderValue, 20);
                }

                @Override
                public void onStopTrackingTouch(Slider slider) {
                    sliderHolder.trackingTouch = false;
                    sliderHolder.handler.removeCallbacks(sliderHolder.sampleSliderValue);
                    sendSliderValue(sliderHolder);
                }
            });

            view.setTag(holder);
        }

        if (holder.entry != this) {
            holder.handler.removeCallbacks(holder.sampleSliderValue);
            holder.trackingTouch = false;
            holder.lastSentValue = value;
        }

        holder.entry = this;
        holder.callback = callback;
        holder.itemName.setText(name);
        holder.slider.setValueFrom(min);
        holder.slider.setValueTo(max);
        if (!holder.trackingTouch) {
            holder.slider.setValue(value);
        }
    }

    private static void sendSliderValue(ViewHolder holder) {
        if (holder.entry == null) {
            return;
        }

        int value = (int) holder.slider.getValue();
        holder.entry.setValue(value);
        if (holder.lastSentValue == null || holder.lastSentValue != value) {
            holder.lastSentValue = value;
            holder.callback.accept(holder.entry.name, value);
        }
    }
}
