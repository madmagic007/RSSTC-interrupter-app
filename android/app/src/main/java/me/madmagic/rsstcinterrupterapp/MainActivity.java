package me.madmagic.rsstcinterrupterapp;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.le.ScanResult;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.widget.ListView;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.lifecycle.ViewModelProvider;

@SuppressLint("MissingPermission")
public class MainActivity extends AppCompatActivity {

    private BleViewModel bleViewModel;
    private MyAdapter adapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        bleViewModel = new ViewModelProvider(this).get(BleViewModel.class);

        ScanResultPopup popup = new ScanResultPopup(this) {
            @Override
            protected void onDeviceSelected(BluetoothDevice device) {
                bleViewModel.connect(device);
            }
        };

        ListView listContainer = findViewById(R.id.valuesContainer);

        adapter = new MyAdapter(this, bleViewModel.getCapabilityEntries(), bleViewModel.getCapabilities(), bleViewModel::writeCapabilityValue);
        listContainer.setAdapter(adapter);

        bleViewModel.setUiListener(new BleViewModel.UiListener() {
            @Override
            public void onCapabilitiesChanged() {
                adapter.notifyDataSetChanged();
            }

            @Override
            public void onScanResult(ScanResult scanResult) {
                BluetoothDevice device = scanResult.getDevice();
                String str = String.format("%s, (%s)", device.getName(), device.getAddress());
                runOnUiThread(() -> popup.addDevice(str, device));
            }

            @Override
            public void onConnectionStateChanged(boolean connected, String name) {
                runOnUiThread(() -> {
                    TextView tv = findViewById(R.id.conDevice);
                    tv.setText(connected ? name : "No interrupter connected");
                    if (connected) {
                        popup.clearDevices();
                    }
                });
            }
        });

        findViewById(R.id.btnStartScan).setOnClickListener(l -> {
            popup.clearDevices();
            bleViewModel.startScanning();
            popup.show();
        });

        requestBluetoothPermissionsIfNot();
    }

    @Override
    protected void onDestroy() {
        if (bleViewModel != null) {
            bleViewModel.setUiListener(null);
        }
        super.onDestroy();
    }

    private static final int REQUEST_BLE_PERMISSIONS = 1;

    private void requestBluetoothPermissionsIfNot() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT)
                == PackageManager.PERMISSION_GRANTED) {

            bleViewModel.startScanning();
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