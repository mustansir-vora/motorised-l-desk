#pragma once
// =====================================================================
//  DIY L-Desk controller — configuration
//  Pins, mechanics and tuning. Secrets go in secrets.h (not committed).
// =====================================================================

// ---------- Wi-Fi ----------
// Passwords live in secrets.h, which git ignores.
// First time: copy secrets.example.h to secrets.h and fill it in.
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #error "secrets.h missing: copy secrets.example.h to secrets.h and add your Wi-Fi details"
#endif
#define HOSTNAME       "desk"          // phone app at http://desk.local
#define AP_SSID        "Desk-Setup"    // fallback hotspot if home Wi-Fi fails

// ---------- Pin map (ESP32 DevKit V1, 38-pin) ----------
#define PIN_EN         13   // R_EN + L_EN of all three BTS7960, 10k pull-down to GND
const int PIN_RPWM[3] = {25, 27, 32};   // leg A, B, C
const int PIN_LPWM[3] = {26, 14, 33};
const int PIN_HALL[3] = {18, 19, 23};   // Hall channel A of each column
const bool LEG_INVERT[3] = {false, false, false}; // set true if a leg runs backwards

#define PIN_BTN_UP     4
#define PIN_BTN_DOWN   16
#define PIN_BTN_P1     17
#define PIN_BTN_P2     35   // input-only pin: add external 10k pull-up to 3.3V
#define PIN_BTN_P3     36   // input-only pin: add external 10k pull-up to 3.3V
#define PIN_BTN_M      39   // input-only pin: add external 10k pull-up to 3.3V
#define PIN_DISP_CLK   21   // TM1637
#define PIN_DISP_DIO   22
#define PIN_BUZZER     5    // active 5V buzzer via BC547 (1k base resistor)
#define PIN_LED        2    // on-board LED

// ---------- Mechanics (calibrate from the app's Settings) ----------
#define DEFAULT_COUNTS_PER_MM  10.0f  // Hall pulses per mm of travel (placeholder!)
#define DEFAULT_MIN_HEIGHT_MM  645    // desktop surface height at the bottom limit
#define DEFAULT_MAX_HEIGHT_MM  1290   // soft upper limit, below the column's own limit
#define DEFAULT_PRESETS        {730, 1100, 920}

// ---------- Motion tuning ----------
#define PWM_FREQ        20000  // 20 kHz: inaudible
#define PWM_BITS        8
#define DUTY_MAX        255
#define DUTY_CRUISE     225    // leaves headroom so lagging legs can be helped
#define DUTY_MIN_MOVE   80     // lowest duty that still moves a loaded column
#define DUTY_HOMING     150    // slower when finding the bottom
#define RAMP_UP         7      // duty steps per tick (soft start ~0.65 s)
#define RAMP_DOWN       28     // duty steps per tick (soft stop ~0.16 s)
#define TICK_MS         20     // control loop period
#define SLOW_ZONE_MM    25.0f  // decelerate over the last 25 mm before a target
#define STOP_EARLY_MM   1.5f   // coast compensation
#define KP_SYNC         8.0f   // duty removed per mm a leg is ahead of the average
#define SKEW_LIMIT_MM   8.0f   // legs further apart than this = fault, stop
#define STALL_MS        250    // a driven leg with no Hall pulses this long = stalled
#define COLLISION_DROP  0.55f  // speed below 55% of cruise = obstacle
#define BACKOFF_MM      20.0f  // reverse this far after an obstacle
#define DEADMAN_MS      400    // app must re-send "move" within this time
#define HOLD_HOME_MS    3000   // hold M + DOWN this long to reset (home)
#define SAVE_WINDOW_MS  5000   // after pressing M, press 1/2/3 within this time
