package me.madmagic.rsstcinterrupterapp;

import android.app.AlertDialog;
import android.content.Context;
import android.view.LayoutInflater;
import androidx.constraintlayout.widget.ConstraintLayout;

public class ScanResultPopup extends AlertDialog {

    private final ConstraintLayout layout;

    public ScanResultPopup(Context context) {
        super(context);
//        setTitle("Title");

        LayoutInflater inflater = LayoutInflater.from(context);
        layout = (ConstraintLayout) inflater.inflate(R.layout.popup_scanresult, null);
        setView(layout);
    }
}
