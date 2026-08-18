package me.madmagic.rsstcinterrupterapp;

import android.annotation.SuppressLint;
import android.bluetooth.*;
import android.bluetooth.le.*;
import android.content.Context;
import android.os.ParcelUuid;
import android.util.Log;
import androidx.annotation.NonNull;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.*;

@SuppressLint("MissingPermission")
public class BLEManager {

    private static final UUID SERVICE_UUID = UUID.fromString("00000000-0000-0000-0000-000000000000");
    public static final UUID CAPS_UUID    = UUID.fromString("11111111-1111-1111-1111-111111111111");
    public static final UUID REPORT_UUID  = UUID.fromString("22222222-2222-2222-2222-222222222222");
    public static final UUID VALUES_UUID  = UUID.fromString("33333333-3333-3333-3333-333333333333");
    private static final UUID CCCD_UUID    = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

    private final Context context;
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic capsChar, reportChar, valuesChar;

    private boolean connected = false;
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
        sendtimer.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() {
                try {
                    writeString("ping");
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
        if (gatt != null) gatt.close();

        gatt = device.connectGatt(context, false, gattCallback);
    }

    private final Queue<BluetoothGattCharacteristic> notificationQueue = new ArrayDeque<>();
    private boolean descriptorWritePending = false;
    private int processNotificationQueue() {
        if (gatt == null || descriptorWritePending) return 1;

        BluetoothGattCharacteristic characteristic = notificationQueue.peek();
        if (characteristic == null) return 0;

        BluetoothGattDescriptor descriptor = characteristic.getDescriptor(CCCD_UUID);

        if (descriptor == null) {
            Log.e("BLE", "CCCD missing: " + characteristic.getUuid());
            notificationQueue.poll();
            processNotificationQueue();
            return 1;
        }

        if (!gatt.setCharacteristicNotification(characteristic, true)) {
            Log.e("BLE", "setCharacteristicNotification failed: " + characteristic.getUuid());
            notificationQueue.poll();
            processNotificationQueue();
            return 1;
        }

        descriptorWritePending = true;

        int started = gatt.writeDescriptor(descriptor, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
        Log.d("BLE","writeDescriptor " +characteristic.getUuid() + " started=" + (started == 1));

        return 1;
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt g, int status, int newState) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                g.discoverServices();

                connected = true;

                stopScanning();
                onDeviceConnected(g.getDevice().getName());
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                connected = false;

                stopSendTimer();
                stopCancelTimer();
                startScanning();
                onDeviceDisconnected();
            }
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
            UUID uuid = descriptor.getCharacteristic().getUuid();
            Log.d("BLE", "CCCD write: " + uuid + " status=" + status);

            descriptorWritePending = false;

            if (status != BluetoothGatt.GATT_SUCCESS) {
                Log.e("BLE", "Failed CCCD write: " + uuid + " status=" + status);
            }

            notificationQueue.poll();
            if (processNotificationQueue() == 0) {
                resetSendTimer();
                resetCancelTimer();

                Log.d("BLE", "all notifications enabled, requesting capabilities");
                writeString("caps");
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
        public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
            Log.d("BLE", "MTU: " + mtu + " status: " + status);

            notificationQueue.offer(capsChar);
            notificationQueue.offer(reportChar);
            processNotificationQueue();
        }

        @Override
        public void onCharacteristicChanged(@NonNull BluetoothGatt gatt, @NonNull BluetoothGattCharacteristic characteristic, @NonNull byte[] value) {
            resetCancelTimer();
            String str = new String(value);

            if (str.equals("pong")) return;

            try {
                JSONObject obj = new JSONObject(str);
                onDataReceived(characteristic.getUuid(), obj);
            } catch (JSONException e) {
                Log.e("BLE", "Bad JSON: " + str, e);
            }
        }
    };

    public void writeValue(String key, int value) {
        try {
            JSONObject obj = new JSONObject()
                    .put("msg", "value")
                    .put("k", key)
                    .put("v", value);

            writeString(obj.toString());
        } catch (JSONException e) {
            Log.e("BLE", "Failed to build value JSON", e);
        }
    }

    public void writeString(String str) {
        if (valuesChar == null || gatt == null || !connected) return;
        gatt.writeCharacteristic(valuesChar, str.getBytes(),  BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
    }

    protected void onDataReceived(UUID uuid, JSONObject data) {}
    protected void onScanResult(ScanResult scanResult) {}
    protected void onDeviceConnected(String name) {}
    protected void onDeviceDisconnected() {}
}
