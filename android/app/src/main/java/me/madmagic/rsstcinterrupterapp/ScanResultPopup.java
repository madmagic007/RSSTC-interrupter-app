package me.madmagic.rsstcinterrupterapp;

import android.app.AlertDialog;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import androidx.constraintlayout.widget.ConstraintLayout;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class ScanResultPopup extends AlertDialog {

    private final ConstraintLayout layout;
    private final Map<String, BluetoothDevice> scannedDevices = new LinkedHashMap<>();
    private final List<String> deviceLabels = new ArrayList<>();
    private final List<String> deviceAddresses = new ArrayList<>();
    private final Spinner spinner;
    private final Button selectButton;
    private final ArrayAdapter<String> adapter;

    public ScanResultPopup(Context context) {
        super(context);

        LayoutInflater inflater = LayoutInflater.from(context);
        layout = (ConstraintLayout) inflater.inflate(R.layout.popup_scanresult, null);
        setView(layout);

        spinner = layout.findViewById(R.id.spinner);
        selectButton = layout.findViewById(R.id.btnSelect);
        adapter = new ArrayAdapter<>(context, android.R.layout.simple_spinner_item, deviceLabels);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);
        selectButton.setEnabled(false);

        layout.findViewById(R.id.btnCancel).setOnClickListener(l -> {
            dismiss();
        });

        selectButton.setOnClickListener(l -> {
            int position = spinner.getSelectedItemPosition();
            if (position < 0 || position >= deviceLabels.size()) return;

            BluetoothDevice device = scannedDevices.get(deviceAddresses.get(position));
            if (device == null) return;
            onDeviceSelected(device);
            dismiss();
        });
    }

    public void rePopulate() {
        adapter.notifyDataSetChanged();
        boolean hasDevices = !deviceLabels.isEmpty();
        selectButton.setEnabled(hasDevices);
        selectButton.setTextColor(hasDevices ? Color.rgb(0, 113, 253) : Color.GRAY);
    }

    public void clearDevices() {
        scannedDevices.clear();
        deviceLabels.clear();
        deviceAddresses.clear();
        rePopulate();
    }

    public void addDevice(String name, BluetoothDevice device) {
        String address = device.getAddress();
        if (scannedDevices.containsKey(address)) return;
        scannedDevices.put(address, device);
        deviceLabels.add(name);
        deviceAddresses.add(address);

        rePopulate();
    }

    protected void onDeviceSelected(BluetoothDevice device) {}
}
