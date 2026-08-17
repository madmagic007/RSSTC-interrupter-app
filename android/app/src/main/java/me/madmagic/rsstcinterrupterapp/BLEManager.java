package me.madmagic.rsstcinterrupterapp;

import android.annotation.SuppressLint;
import android.bluetooth.*;
import android.bluetooth.le.*;
import android.content.Context;
import android.os.Build;
import android.os.ParcelUuid;
import android.util.Log;
import org.json.JSONException;
import org.json.JSONObject;

import java.nio.charset.StandardCharsets;
import java.util.Collections;
import java.util.Timer;
import java.util.TimerTask;
import java.util.UUID;

@SuppressLint("MissingPermission")
public class BLEManager {

    private static final UUID SERVICE_UUID = UUID.fromString("00000000-0000-0000-0000-000000000000");
    public static final UUID CAPS_UUID    = UUID.fromString("11111111-1111-1111-1111-111111111111");
    public static final UUID REPORT_UUID  = UUID.fromString("22222222-2222-2222-2222-222222222222");
    public static final UUID VALUES_UUID  = UUID.fromString("33333333-3333-3333-3333-333333332222");
    private static final UUID CCCD_UUID    = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

    private final Context context;
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic capsChar, reportChar, valuesChar;

    private boolean connected = false;
    private long lastMessage = System.currentTimeMillis();
    private long timeoutDelay = 10000;
    private Timer sendtimer;
    private Timer cancelTimer;

    private final ScanFilter scanFilter;
    private final ScanSettings scanSettings;
    private final ScanCallback scanCb;
    private final BluetoothLeScanner scanner;

    public BLEManager(Context context) {
        this.context = context;

        BluetoothManager btManager = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
        BluetoothAdapter btAdapter = btManager.getAdapter();
        scanner = btAdapter.getBluetoothLeScanner();

        scanFilter = new ScanFilter.Builder()
                .setServiceUuid(new ParcelUuid(SERVICE_UUID))
                .build();

        scanSettings = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .build();

        scanCb = new ScanCallback() {
            @Override
            public void onScanResult(int callbackType, ScanResult result) {
                BLEManager.this.onScanResult(result);
            }
        };
    }

    public void startScanning() {
        scanner.startScan(Collections.singletonList(scanFilter), scanSettings, scanCb);
    }

    public void stopScanning() {
        scanner.stopScan(scanCb);
    }

    private void stopSendTimer() {
        try {
            sendtimer.cancel();
        } catch (Exception ignored) {}
    }

    private void stopCancelTimer() {
        try {
            cancelTimer.cancel();
        } catch (Exception ignored) {}
    }

    private void resetSendTimer() {
        stopSendTimer();

        sendtimer = new Timer();
        sendtimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    Log.d("BLE", "sending ping");
                    writeJson(new JSONObject().put("msg", "ping"));
                } catch (Exception ignored) {}
            }
        }, 1000, 1000);
    }

    private void resetCancelTimer() {
        stopCancelTimer();

        cancelTimer = new Timer();
        cancelTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                gatt.disconnect();
            }
        }, timeoutDelay);
    }


    public void connect(BluetoothDevice device) {
        gatt = device.connectGatt(context, false, gattCallback);
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt g, int status, int newState) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                g.discoverServices();

                connected = true;
                lastMessage = System.currentTimeMillis();

                resetSendTimer();
                resetCancelTimer();

                stopScanning();

                onDeviceConnected();
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                connected = false;

                stopSendTimer();
                stopCancelTimer();

                startScanning();

                onDeviceDisconnected();
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt g, int status) {
            BluetoothGattService service = g.getService(SERVICE_UUID);
            capsChar   = service.getCharacteristic(CAPS_UUID);
            reportChar = service.getCharacteristic(REPORT_UUID);
            valuesChar = service.getCharacteristic(VALUES_UUID);

            g.requestMtu(247);
        }

        @Override
        public void onMtuChanged(BluetoothGatt g, int mtu, int status) {
            Log.d("BLE", "MTU: " + mtu + " status: " + status);
            enableIndications(g, capsChar);
            enableIndications(g, reportChar);
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt g, BluetoothGattCharacteristic characteristic) {
            lastMessage = System.currentTimeMillis();

            byte[] data = characteristic.getValue();
            String json = new String(data, StandardCharsets.UTF_8);

            try {
                JSONObject obj = new JSONObject(json);
                onDataReceived(characteristic.getUuid(), obj);
            } catch (JSONException e) {
                Log.e("BLE", "Bad JSON: " + json, e);
            }
        }
    };

    private void enableIndications(BluetoothGatt g, BluetoothGattCharacteristic characteristic) {
        if (characteristic == null) return;

        g.setCharacteristicNotification(characteristic, true);
        BluetoothGattDescriptor descriptor = characteristic.getDescriptor(CCCD_UUID);
        if (descriptor != null) {
            descriptor.setValue(BluetoothGattDescriptor.ENABLE_INDICATION_VALUE);
            g.writeDescriptor(descriptor);
        }
    }

    public void writeValue(String key, int value) {
        if (valuesChar == null || gatt == null) return;

        try {
            JSONObject obj = new JSONObject()
                    .put("msg", "value")
                    .put("k", key)
                    .put("v", value);

            writeJson(obj);
        } catch (JSONException e) {
            Log.e("BLE", "Failed to build value JSON", e);
        }
    }

    public void writeJson(JSONObject o) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            gatt.writeCharacteristic(valuesChar, o.toString().getBytes(),  BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
        } else {
            valuesChar.setValue(o.toString().getBytes());
            valuesChar.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
            boolean success = gatt.writeCharacteristic(valuesChar);
            if (!success) {
                Log.e("BLE", "Write failed (legacy API)");
            }
        }
    }

    protected void onDataReceived(UUID uuid, JSONObject data) {}
    protected void onScanResult(ScanResult scanResult) {}
    protected void onDeviceConnected() {}
    protected void onDeviceDisconnected() {}
}
