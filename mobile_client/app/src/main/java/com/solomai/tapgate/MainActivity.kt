package com.solomai.tapgate

import android.os.Bundle
import android.widget.Toast
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import com.google.android.material.button.MaterialButton
import android.content.Context
import android.media.AudioManager
import android.widget.TextView
import android.widget.ImageView

class MainActivity : AppCompatActivity() {
    // Handler called when the app is starting (before UI is shown)
    private fun onAppStart() {
        // TODO: Add logic to be executed when the app starts (before UI is visible)
    }

    // Handler called when the UI is fully drawn and visible to the user
    private fun onUiReady() {
        // TODO: Add logic to be executed when the UI is ready and visible
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        onAppStart()
        enableEdgeToEdge()
        setContentView(R.layout.activity_main)
        // Set initial status texts and icons after setContentView
        findViewById<TextView>(R.id.wifi_status_text).text = getString(R.string.check_online_status)
        findViewById<TextView>(R.id.bt_status_text).text = getString(R.string.check_bluetooth_status)
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main)) { v, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }
        findViewById<MaterialButton>(R.id.button_doAction).setOnClickListener {
            // Play click sound
            val audioManager = getSystemService(Context.AUDIO_SERVICE) as AudioManager
            audioManager.playSoundEffect(AudioManager.FX_KEY_CLICK)

            // doAction handler
            Toast.makeText(this, "Button pressed!", Toast.LENGTH_SHORT).show()
        }
    }

    override fun onResume() {
        super.onResume()
        onUiReady()
    }
}