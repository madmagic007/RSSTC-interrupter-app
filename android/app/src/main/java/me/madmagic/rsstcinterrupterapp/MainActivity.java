package me.madmagic.rsstcinterrupterapp;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.le.ScanResult;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.UUID;

@SuppressLint("MissingPermission")
public class MainActivity extends AppCompatActivity {

    private BLEManager bleManager;

    private final HashMap<String, BluetoothDevice> scannedDevices = new HashMap<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        bleManager = new BLEManager(this) {
            @Override
            protected void onDataReceived(UUID uuid, JSONObject data) {
                Log.d("BLE", data.toString());
            }

            @Override
            protected void onScanResult(ScanResult scanResult) {
                Log.d("BLE", scanResult.getDevice().getName() + ", " + scanResult.getDevice().getAddress());

                BluetoothDevice device = scanResult.getDevice();
                String str = String.format("%s, (%s)", device.getName(), device.getAddress());

                if (scannedDevices.containsKey(str)) return;
                scannedDevices.put(str, device);
            }

            @Override
            protected void onDeviceDisconnected() {
                Log.d("BLE", "disconnected");
            }

            @Override
            protected void onDeviceConnected() {
                Log.d("BLE", "connected");
            }
        };

        findViewById(R.id.btnStartScan).setOnClickListener(l -> {
            new ScanResultPopup(MainActivity.this).show();
        });

        requestBluetoothPermissionsIfNot();
    }

    private static final int REQUEST_BLE_PERMISSIONS = 1;

    private void requestBluetoothPermissionsIfNot() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT)
                == PackageManager.PERMISSION_GRANTED) {

            bleManager.startScanning();
            return;
        };

        ActivityCompat.requestPermissions(this,
                new String[]{
                        Manifest.permission.BLUETOOTH_SCAN,
                        Manifest.permission.BLUETOOTH_CONNECT
                },
                REQUEST_BLE_PERMISSIONS);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_BLE_PERMISSIONS) {
            boolean granted = true;
            for (int result : grantResults) {
                if (result != PackageManager.PERMISSION_GRANTED) {
                    granted = false;
                    break;
                }
            }

            if (granted) {
                bleManager.startScanning();
            } else {} //denied
        }
    }
}