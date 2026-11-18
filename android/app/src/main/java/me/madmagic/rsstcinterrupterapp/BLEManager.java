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

import java.nio.charset.StandardCharsets;
import java.util.*;

@SuppressLint("MissingPermission")
public class BLEManager {

    public interface Listener {
        void onDataReceived(UUID uuid, JSONObject data);
        void onScanResult(ScanResult scanResult);
        void onConnectionStateChanged(boolean connected, String name);
    }

    private static final UUID SERVICE_UUID =            UUID.fromString("00000000-0000-0000-0000-000000000000");
    public static final UUID  CAPS_DEFS_UUID =          UUID.fromString("11111111-1111-1111-1111-111111111111");
    public static final UUID  DEVICE_REPOTS_UUID =      UUID.fromString("22222222-2222-2222-2222-222222222222");
    public static final UUID  CONTROL_WRITE_UUID =      UUID.fromString("33333333-3333-3333-3333-333333333333");
    public static final UUID  TELEMETRY_UPDATES_UUID =  UUID.fromString("44444444-4444-4444-4444-444444444444");
    public static final UUID  TELEMETRY_DEFS_UUIDS =    UUID.fromString("55555555-5555-5555-5555-555555555555");
    public static final UUID  HOST_REQUESTS_UUID =      UUID.fromString("66666666-6666-6666-6666-666666666666");
    private static final UUID CCCD_UUID =               UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

    private final Context context;
    private BluetoothGatt gatt;
    private volatile Listener listener;
    private BluetoothGattCharacteristic capsDefsChar, deviceReportsChar, telUpdatesChar, telDefsChar;

    private boolean connected = false;
    private boolean scanning = false;
    private boolean writeQueueReady = false;
    private long timeoutDelay = 10000;
    private Timer cancelTimer;

    private static final class PendingWrite {
        final UUID characteristicUuid;
        final byte[] value;
        final String capabilityName;
        final int writeType;

        PendingWrite(UUID characteristicUuid, byte[] value, String capabilityName, int writeType) {
            this.characteristicUuid = characteristicUuid;
            this.value = value;
            this.capabilityName = capabilityName;
            this.writeType = writeType;
        }
    }

    private final Object writeQueueLock = new Object();
    private final Queue<PendingWrite> writeQueue = new ArrayDeque<>();
    private PendingWrite activeWrite;
    private long nextControlMessageId = 1;

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
                Listener currentListener = BLEManager.this.listener;
                if (currentListener != null) {
                    currentListener.onScanResult(result);
                }
            }

            @Override
            public void onScanFailed(int errorCode) {
                scanning = false;
                Log.e("BLE", "Scan failed: " + errorCode);
            }
        };
    }

    public void setListener(Listener listener) {
        this.listener = listener;
    }

    public void startScanning() {
        if (connected || scanning) return;

        try {
            scanner.startScan(Collections.singletonList(scanFilter), scanSettings, scanCb);
            scanning = true;
        } catch (IllegalStateException e) {
            Log.e("BLE", "Failed to start scan", e);
        }
    }

    public void stopScanning() {
        if (!scanning) return;

        scanner.stopScan(scanCb);
        scanning = false;
    }

    public void shutdown() {
        stopScanning();
        stopCancelTimer();

        BluetoothGatt oldGatt;
        synchronized (writeQueueLock) {
            connected = false;
            writeQueueReady = false;
            writeQueue.clear();
            activeWrite = null;
            oldGatt = gatt;
            gatt = null;
        }

        if (oldGatt != null) {
            oldGatt.disconnect();
            oldGatt.close();
        }
    }

    private void stopCancelTimer() {
        try {
            cancelTimer.cancel();
        } catch (Exception ignored) {}
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
        synchronized (writeQueueLock) {
            logDroppedWritesLocked("reconnecting");
            connected = false;
            writeQueueReady = false;
            writeQueue.clear();
            activeWrite = null;
        }

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
            if (g != BLEManager.this.gatt) return;

            if (newState == BluetoothProfile.STATE_CONNECTED) {
                g.discoverServices();
                if (!g.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH)) {
                    Log.w("BLE", "Failed to request high-priority BLE connection");
                }

                synchronized (writeQueueLock) {
                    connected = true;
                }

                stopScanning();
                Listener currentListener = listener;
                if (currentListener != null) {
                    currentListener.onConnectionStateChanged(true, g.getDevice().getName());
                }
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                synchronized (writeQueueLock) {
                    logDroppedWritesLocked("disconnected");
                    connected = false;
                    writeQueueReady = false;
                    writeQueue.clear();
                    activeWrite = null;
                }

                stopCancelTimer();
                startScanning();
                Listener currentListener = listener;
                if (currentListener != null) {
                    currentListener.onConnectionStateChanged(false, null);
                }
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
                resetCancelTimer();

                Log.d("BLE", "all notifications enabled, requesting definitions");
                synchronized (writeQueueLock) {
                    writeQueueReady = true;
                    enqueueWriteLocked(HOST_REQUESTS_UUID, "defs");
                    processWriteQueueLocked();
                }
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt g, int status) {
            BluetoothGattService service = g.getService(SERVICE_UUID);
            capsDefsChar = service.getCharacteristic(CAPS_DEFS_UUID);
            deviceReportsChar = service.getCharacteristic(DEVICE_REPOTS_UUID);
            telUpdatesChar = service.getCharacteristic(TELEMETRY_UPDATES_UUID);
            telDefsChar = service.getCharacteristic(TELEMETRY_DEFS_UUIDS);

            g.requestMtu(247);
        }

        @Override
        public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
            Log.d("BLE", "MTU: " + mtu + " status: " + status);

            notificationQueue.offer(capsDefsChar);
            notificationQueue.offer(deviceReportsChar);
            notificationQueue.offer(telDefsChar);
            notificationQueue.offer(telUpdatesChar);
            processNotificationQueue();
        }

        @Override
        public void onCharacteristicChanged(@NonNull BluetoothGatt gatt, @NonNull BluetoothGattCharacteristic characteristic, @NonNull byte[] value) {
            resetCancelTimer();

            String str = new String(value);
            if (str.equals("hb")) return;

            try {
                JSONObject obj = new JSONObject(str);
                Listener currentListener = listener;
                if (currentListener != null) {
                    currentListener.onDataReceived(characteristic.getUuid(), obj);
                }
            } catch (JSONException e) {
                Log.e("BLE", "Bad JSON: " + str, e);
            }
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt callbackGatt, BluetoothGattCharacteristic characteristic, int status) {
            if (callbackGatt != BLEManager.this.gatt) return;

            synchronized (writeQueueLock) {
                if (activeWrite == null) {
                    if (status != BluetoothGatt.GATT_SUCCESS) {
                        Log.w("BLE", "Unacknowledged characteristic write status: "
                                + characteristic.getUuid() + " status=" + status);
                    }
                    processWriteQueueLocked();
                    return;
                }

                if (!activeWrite.characteristicUuid.equals(characteristic.getUuid())) {
                    Log.e("BLE", "Unexpected characteristic write callback: "
                            + characteristic.getUuid() + " status=" + status);
                    return;
                }

                if (status != BluetoothGatt.GATT_SUCCESS) {
                    Log.e("BLE", "Characteristic write failed: "
                            + characteristic.getUuid() + " status=" + status);
                }

                activeWrite = null;
                processWriteQueueLocked();
            }
        }
    };

    public void queueControlValue(String key, int value) {
        synchronized (writeQueueLock) {
            if (!canQueueWriteLocked()) return;

            try {
                JSONObject values = new JSONObject();
                values.put(key, value);
                JSONObject message = new JSONObject()
                        .put("id", nextControlMessageId++)
                        .put("values", values);
                enqueueWriteLocked(
                        CONTROL_WRITE_UUID,
                        message.toString(),
                        key,
                        BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
                processWriteQueueLocked();
            } catch (JSONException e) {
                Log.e("BLE", "Failed to build control message", e);
            }
        }
    }

    public void queueJson(UUID characteristicUuid, JSONObject json) {
        if (json == null) {
            Log.e("BLE", "Cannot queue null JSON");
            return;
        }
        queueWrite(characteristicUuid, json.toString());
    }

    public void queueWrite(UUID characteristicUuid, String value) {
        if (characteristicUuid == null || value == null) {
            Log.e("BLE", "Cannot queue write with null characteristic or value");
            return;
        }

        synchronized (writeQueueLock) {
            if (!canQueueWriteLocked()) return;
            enqueueWriteLocked(characteristicUuid, value);
            processWriteQueueLocked();
        }
    }

    private boolean canQueueWriteLocked() {
        if (connected && gatt != null) return true;
        Log.e("BLE", "Write rejected: not connected");
        return false;
    }

    private void enqueueWriteLocked(UUID characteristicUuid, String value) {
        enqueueWriteLocked(characteristicUuid, value, null, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
    }

    private void enqueueWriteLocked(UUID characteristicUuid, String value, String capabilityName, int writeType) {
        if (capabilityName != null) {
            writeQueue.removeIf(pending -> capabilityName.equals(pending.capabilityName));
        }
        writeQueue.offer(new PendingWrite(
                characteristicUuid,
                value.getBytes(StandardCharsets.UTF_8),
                capabilityName,
                writeType));
    }

    private void logDroppedWritesLocked(String reason) {
        int droppedCount = writeQueue.size() + (activeWrite == null ? 0 : 1);
        if (droppedCount > 0) {
            Log.w("BLE", "Dropping " + droppedCount + " queued write(s): " + reason);
        }
    }

    private void processWriteQueueLocked() {
        if (!writeQueueReady || !connected || gatt == null || activeWrite != null) return;

        while (!writeQueue.isEmpty()) {
            PendingWrite next = writeQueue.peek();
            BluetoothGattService service = gatt.getService(SERVICE_UUID);
            BluetoothGattCharacteristic characteristic = service == null
                    ? null
                    : service.getCharacteristic(next.characteristicUuid);
            if (characteristic == null) {
                writeQueue.poll();
                Log.e("BLE", "Write characteristic missing: " + next.characteristicUuid);
                continue;
            }

            int result = gatt.writeCharacteristic(
                    characteristic, next.value, next.writeType);
            if (result != BluetoothStatusCodes.SUCCESS) {
                writeQueue.poll();
                Log.e("BLE", "Failed to queue write to " + next.characteristicUuid
                        + " status=" + result);
                continue;
            }

            writeQueue.poll();
            activeWrite = next;
            return;
        }
    }

}
