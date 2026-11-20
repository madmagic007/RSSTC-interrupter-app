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
import me.madmagic.rsstcinterrupterapp.controlEntries.TelemetryEntry;
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
        void onCapabilityValueChanged(String name);
        void onTelemetryChanged();
        void onTelemetryValueChanged(String name);
        void onScanResult(ScanResult scanResult);
        void onConnectionStateChanged(boolean connected, String name);
    }

    private final BLEManager bleManager;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final Map<String, ControlEntry> capabilities = new LinkedHashMap<>();
    private final List<ControlEntry> capabilityEntries = new ArrayList<>();
    private final Map<String, TelemetryEntry> telemetry = new LinkedHashMap<>();
    private final List<TelemetryEntry> telemetryEntries = new ArrayList<>();
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
        listener.onTelemetryChanged();
    }

    public Map<String, ControlEntry> getCapabilities() {
        return capabilities;
    }

    public List<ControlEntry> getCapabilityEntries() {
        return capabilityEntries;
    }

    public Map<String, TelemetryEntry> getTelemetry() {
        return telemetry;
    }

    public List<TelemetryEntry> getTelemetryEntries() {
        return telemetryEntries;
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

        try {
            Log.d("BLE", uuid.toString() + " " + data.toString(4));
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }

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
                UiListener listener = uiListener;
                if (listener != null) {
                    listener.onCapabilitiesChanged();
                }
            } else if (BLEManager.DEVICE_REPOTS_UUID.equals(uuid)) {
                String name = data.optString("name");
                ControlEntry capability = capabilities.get(name);
                if (capability == null) return;

                capability.setValue(data.optInt("value"));
                UiListener listener = uiListener;
                if (listener != null) {
                    listener.onCapabilityValueChanged(name);
                }
            } else if (BLEManager.TELEMETRY_DEFS_UUIDS.equals(uuid)) {
                String name = data.optString("name");
                if (telemetry.containsKey(name)) return;

                TelemetryEntry entry = new TelemetryEntry(name, data.optLong("value"));
                telemetry.put(name, entry);
                telemetryEntries.add(entry);

                UiListener listener = uiListener;
                if (listener != null) {
                    listener.onTelemetryChanged();
                }
            } else if (BLEManager.TELEMETRY_UPDATES_UUID.equals(uuid)) {
                String name = data.optString("name");
                TelemetryEntry entry = telemetry.get(name);
                if (entry == null) return;

                entry.setValue(data.optLong("value"));
                UiListener listener = uiListener;
                if (listener != null) {
                    listener.onTelemetryValueChanged(name);
                }
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
                telemetry.clear();
                telemetryEntries.clear();
            }

            UiListener listener = uiListener;
            if (listener != null) {
                listener.onConnectionStateChanged(connected, connectedDeviceName);
                if (!connected) {
                    listener.onCapabilitiesChanged();
                    listener.onTelemetryChanged();
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
