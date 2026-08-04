#pragma once

// SoftAP + captive portal WiFi configuration mode.

// Starts the config AP and web server. Safe to call repeatedly.
void portalStart();

// Call every loop() iteration while in portal mode. Handles DNS/web requests
// and performs the scheduled reboot after a save/clear.
void portalLoop();

// True when the portal has saved new settings (used to update the display).
bool portalDirty();
