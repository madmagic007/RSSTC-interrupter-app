package me.madmagic.rsstcinterrupterapp;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.le.ScanResult;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ListView;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.Guideline;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.lifecycle.ViewModelProvider;
import me.madmagic.rsstcinterrupterapp.controlEntries.ControlAdapter;
import me.madmagic.rsstcinterrupterapp.telemetryValues.TelemetryAdapter;

@SuppressLint("MissingPermission")
public class MainActivity extends AppCompatActivity {

    private static final String TELEMETRY_SPLIT_PREFERENCE = "telemetry_split";
    private static final float DEFAULT_TELEMETRY_SPLIT = 0.68f;
    private static final float MIN_TELEMETRY_SPLIT = 0.42f;
    private static final float MAX_TELEMETRY_SPLIT = 0.82f;

    private BLEViewModel bleViewModel;
    private ControlAdapter controlAdapter;
    private TelemetryAdapter telemetryAdapter;
    private float telemetrySplit;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Guideline telemetryStart = findViewById(R.id.telemetryStart);
        View divider = findViewById(R.id.telemetryDivider);
        View telemetryBoundary = findViewById(R.id.telemetryBoundary);
        telemetrySplit = getPreferences(MODE_PRIVATE)
                .getFloat(TELEMETRY_SPLIT_PREFERENCE, DEFAULT_TELEMETRY_SPLIT);
        telemetrySplit = clampTelemetrySplit(telemetrySplit);
        telemetryStart.setGuidelinePercent(telemetrySplit);
        configureDraggableDivider(telemetryStart, divider);

        bleViewModel = new ViewModelProvider(this).get(BLEViewModel.class);

        ScanResultPopup popup = new ScanResultPopup(this) {
            @Override
            protected void onDeviceSelected(BluetoothDevice device) {
                bleViewModel.connect(device);
            }
        };

        ListView listContainer = findViewById(R.id.valuesContainer);
        ListView telemetryList = findViewById(R.id.telemetryContainer);
        listContainer.setVisibility(View.GONE);
        telemetryList.setVisibility(View.GONE);
        divider.setVisibility(View.GONE);
        telemetryBoundary.setVisibility(View.GONE);

        controlAdapter = new ControlAdapter(this, bleViewModel.getCapabilityEntries(), bleViewModel.getCapabilities(), bleViewModel::writeCapabilityValue);
        listContainer.setAdapter(controlAdapter);

        telemetryAdapter = new TelemetryAdapter(this, bleViewModel.getTelemetryEntries(), bleViewModel.getTelemetry());
        telemetryList.setAdapter(telemetryAdapter);

        bleViewModel.setUiListener(new BLEViewModel.UiListener() {
            @Override
            public void onCapabilitiesChanged() {
                controlAdapter.notifyDataSetChanged();
            }

            @Override
            public void onCapabilityValueChanged(String name) {
                controlAdapter.refreshVisibleEntry(name, listContainer);
            }

            @Override
            public void onTelemetryChanged() {
                telemetryAdapter.notifyDataSetChanged();
            }

            @Override
            public void onTelemetryValueChanged(String name) {
                telemetryAdapter.refreshVisibleEntry(name, telemetryList);
            }

            @Override
            public void onDefinitionsComplete(boolean hasCapabilities, boolean hasTelemetry) {
                runOnUiThread(() -> {
                    listContainer.setVisibility(hasCapabilities ? View.VISIBLE : View.GONE);
                    telemetryList.setVisibility(hasTelemetry ? View.VISIBLE : View.GONE);

                    boolean showDivider = hasCapabilities && hasTelemetry;
                    divider.setVisibility(showDivider ? View.VISIBLE : View.GONE);
                    telemetryBoundary.setVisibility(showDivider ? View.VISIBLE : View.GONE);

                    if (showDivider) {
                        telemetryStart.setGuidelinePercent(telemetrySplit);
                    } else if (hasCapabilities) {
                        telemetryStart.setGuidelinePercent(1f);
                    } else if (hasTelemetry) {
                        telemetryStart.setGuidelinePercent(0f);
                    }
                });
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
                    listContainer.setVisibility(View.GONE);
                    telemetryList.setVisibility(View.GONE);
                    divider.setVisibility(View.GONE);
                    telemetryBoundary.setVisibility(View.GONE);
                    if (connected) {
                        telemetryStart.setGuidelinePercent(telemetrySplit);
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

    private void configureDraggableDivider(Guideline guideline, View divider) {
        View root = findViewById(R.id.main);
        divider.setOnClickListener(view -> {});
        divider.setOnTouchListener(new View.OnTouchListener() {
            private final int[] rootLocation = new int[2];

            @Override
            public boolean onTouch(View view, MotionEvent event) {
                switch (event.getActionMasked()) {
                    case MotionEvent.ACTION_DOWN:
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        int height = root.getHeight();
                        if (height > 0) {
                            root.getLocationOnScreen(rootLocation);
                            telemetrySplit = clampTelemetrySplit(
                                    (event.getRawY() - rootLocation[1]) / height);
                            guideline.setGuidelinePercent(telemetrySplit);
                        }
                        return true;
                    case MotionEvent.ACTION_UP:
                        getPreferences(MODE_PRIVATE).edit()
                                .putFloat(TELEMETRY_SPLIT_PREFERENCE, telemetrySplit)
                                .apply();
                        view.performClick();
                        return true;
                    case MotionEvent.ACTION_CANCEL:
                        getPreferences(MODE_PRIVATE).edit()
                                .putFloat(TELEMETRY_SPLIT_PREFERENCE, telemetrySplit)
                                .apply();
                        return true;
                    default:
                        return false;
                }
            }
        });
    }

    private static float clampTelemetrySplit(float split) {
        return Math.max(MIN_TELEMETRY_SPLIT, Math.min(MAX_TELEMETRY_SPLIT, split));
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