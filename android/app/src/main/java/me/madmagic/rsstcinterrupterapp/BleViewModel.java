package me.madmagic.rsstcinterrupterapp;

import android.app.Application;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.le.ScanResult;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.lifecycle.AndroidViewModel;

import me.madmagic.rsstcinterrupterapp.controlEntries.ControlEntry;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

public class BleViewModel extends AndroidViewModel implements BLEManager.Listener {

    public interface UiListener {
        void onCapabilitiesChanged();
        void onScanResult(ScanResult scanResult);
        void onConnectionStateChanged(boolean connected, String name);
    }

    private final BLEManager bleManager;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final Map<String, ControlEntry> capabilities = new LinkedHashMap<>();
    private final List<ControlEntry> capabilityEntries = new ArrayList<>();
    private volatile UiListener uiListener;
    private volatile String connectedDeviceName;
    private volatile boolean connected;

    public BleViewModel(@NonNull Application application) {
        super(application);
        bleManager = new BLEManager(application);
        bleManager.setListener(this);
    }

    public void setUiListener(UiListener listener) {
        uiListener = listener;
        if (listener == null) return;

        listener.onConnectionStateChanged(connected, connectedDeviceName);
        listener.onCapabilitiesChanged();
    }

    public Map<String, ControlEntry> getCapabilities() {
        return capabilities;
    }

    public List<ControlEntry> getCapabilityEntries() {
        return capabilityEntries;
    }

    public void startScanning() {
        bleManager.startScanning();
    }

    public void stopScanning() {
        bleManager.stopScanning();
    }

    public void connect(BluetoothDevice device) {
        bleManager.connect(device);
    }

    public void writeCapabilityValue(String name, int value) {
        bleManager.queueControlValue(name, value);
    }

    @Override
    public void onDataReceived(UUID uuid, JSONObject data) {
        mainHandler.post(() -> {
            if (BLEManager.CAPS_DEFS_UUID.equals(uuid)) {

                String name = data.optString("name");
                if (capabilities.containsKey(name)) return;

                try {
                    ControlEntry capability = ControlEntry.create(data);
                    if (capability == null) return;

                    capabilities.put(name, capability);
                    capabilityEntries.add(capability);
                } catch (JSONException e) {
                    Log.e("BLE", "Failed to parse capability definition", e);
                    return;
                }
            } else if (BLEManager.DEVICE_REPOTS_UUID.equals(uuid)) {

                ControlEntry capability = capabilities.get(data.optString("name"));
                if (capability == null) return;

                capability.setValue(data.optInt("value"));
            }

            if (uiListener != null) {
                uiListener.onCapabilitiesChanged();
            }
        });
    }

    @Override
    public void onScanResult(ScanResult scanResult) {
        UiListener listener = uiListener;
        if (listener != null) {
            listener.onScanResult(scanResult);
        }
    }

    @Override
    public void onConnectionStateChanged(boolean connected, String name) {
        mainHandler.post(() -> {
            this.connected = connected;
            connectedDeviceName = connected ? name : null;
            if (!connected) {
                capabilities.clear();
                capabilityEntries.clear();
            }

            UiListener listener = uiListener;
            if (listener != null) {
                listener.onConnectionStateChanged(connected, connectedDeviceName);
                if (!connected) {
                    listener.onCapabilitiesChanged();
                }
            }
        });
    }

    @Override
    protected void onCleared() {
        uiListener = null;
        bleManager.setListener(null);
        bleManager.shutdown();
    }
}
