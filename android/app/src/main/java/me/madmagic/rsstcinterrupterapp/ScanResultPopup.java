package me.madmagic.rsstcinterrupterapp;

import android.app.AlertDialog;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.view.LayoutInflater;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import androidx.constraintlayout.widget.ConstraintLayout;

import java.util.ArrayList;
import java.util.HashMap;

public class ScanResultPopup extends AlertDialog {

    private final ConstraintLayout layout;
    private final HashMap<String, BluetoothDevice> scannedDevices = new HashMap<>();
    private final Spinner spinner;

    public ScanResultPopup(Context context) {
        super(context);

        LayoutInflater inflater = LayoutInflater.from(context);
        layout = (ConstraintLayout) inflater.inflate(R.layout.popup_scanresult, null);
        setView(layout);

        spinner = layout.findViewById(R.id.spinner);

        layout.findViewById(R.id.btnCancel).setOnClickListener(l -> {
            dismiss();
        });

        layout.findViewById(R.id.btnSelect).setOnClickListener(l -> {
            String name = spinner.getSelectedItem().toString();
            BluetoothDevice device = scannedDevices.getOrDefault(name, null);
            if (device != null) onDeviceSelected(device);

            dismiss();
        });
    }

    public void rePopulate() {
        ArrayAdapter<String> adapter = new ArrayAdapter<>(getContext(), android.R.layout.simple_spinner_item, new ArrayList<>(scannedDevices.keySet()));
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);
    }

    public void addDevice(String name, BluetoothDevice device) {
        if (scannedDevices.containsKey(name)) return;
        scannedDevices.put(name, device);

        rePopulate();
    }

    protected void onDeviceSelected(BluetoothDevice device) {}
}
