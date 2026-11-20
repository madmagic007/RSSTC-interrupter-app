package me.madmagic.rsstcinterrupterapp.controlEntries;

import android.view.View;
import android.widget.Button;
import me.madmagic.rsstcinterrupterapp.R;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.function.BiConsumer;

public class ButtonCapability extends ControlEntry {

    public ButtonCapability(JSONObject o) throws JSONException {
        super(o);
    }

    @Override
    public boolean isUserValuePersistable(int value) {
        return false;
    }

    @Override
    public int getViewType() {
        return TYPE_BUTTON;
    }

    @Override
    public int getLayoutId() {
        return R.layout.item_button;
    }

    static class ViewHolder {
        Button btn;
        ButtonCapability entry;
        BiConsumer<String, Integer> callback;
    }

    @Override
    public void bindView(View view, BiConsumer<String, Integer> callback) {
        ViewHolder holder = (ViewHolder) view.getTag();

        if (holder == null) {
            holder = new ViewHolder();

            holder.btn = view.findViewById(R.id.itemBtn);

            ViewHolder buttonHolder = holder;
            holder.btn.setOnClickListener(button -> {
                if (buttonHolder.entry != null) {
                    buttonHolder.callback.accept(buttonHolder.entry.name, 1);
                }
            });

            view.setTag(holder);
        }

        holder.entry = this;
        holder.callback = callback;
        holder.btn.setText(name);
    }
}
