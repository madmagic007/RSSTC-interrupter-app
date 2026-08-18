package me.madmagic.rsstcinterrupterapp;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.le.ScanResult;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.ListView;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import org.json.JSONObject;

import java.util.UUID;

@SuppressLint("MissingPermission")
public class MainActivity extends AppCompatActivity {

    private BLEManager bleManager;
    private MyAdapter adapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ScanResultPopup popup = new ScanResultPopup(this) {
            @Override
            protected void onDeviceSelected(BluetoothDevice device) {
                bleManager.connect(device);
            }
        };

        bleManager = new BLEManager(this) {
            @Override
            protected void onDataReceived(UUID uuid, JSONObject data) {
                Log.d("BLE", data.toString());

                runOnUiThread(() -> {
                    try {
                        if (uuid.equals(BLEManager.CAPS_UUID)) {
                            adapter.addData(data);
                        }
                    } catch (Exception ignored) {}
                });
            }

            @Override
            protected void onScanResult(ScanResult scanResult) {
                BluetoothDevice device = scanResult.getDevice();
                String str = String.format("%s, (%s)", device.getName(), device.getAddress());
                popup.addDevice(str, device);
            }

            @Override
            protected void onDeviceDisconnected() {
                Log.d("BLE", "disconnected");

                runOnUiThread(() -> {
                    TextView tv = findViewById(R.id.conDevice);
                    tv.setText("No interrupter connected");

                    adapter.clear();
                });
            }

            @Override
            protected void onDeviceConnected(String name) {
                Log.d("BLE", "connected");

                runOnUiThread(() -> {
                    TextView tv = findViewById(R.id.conDevice);
                    tv.setText(name);
                });
            }
        };

        ListView listContainer = findViewById(R.id.valuesContainer);
        adapter = new MyAdapter(this, bleManager::writeValue);
        listContainer.setAdapter(adapter);

        findViewById(R.id.btnStartScan).setOnClickListener(l -> {
            bleManager.startScanning();
            popup.show();
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

            if (!granted) {
                requestBluetoothPermissionsIfNot();
            }
        }
    }
}