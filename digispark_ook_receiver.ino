// Digispark OOK Receiver connects a little 433.92mhz OOK receiver to pin 2
// of a digispark and uses interrupts to receive button codes wirelessly.
// These button codes are fingerprinted in to unique 32-bit numbers and sent
// over DigiUSB to a connected computer which can do neat home automation
// stuff with them. It works with doorbell buttons and other cheap 433mhz
// remote control gadgets easily found on ebay.
//
// The inlcuded ruby script requires the digiusb rubygem, libusb 1.x, and
// ruby 1.9.x, libusb 1.x which can be easily installed on Mac's via
// Mac Homebrew: http://mxcl.github.com/homebrew/
//   brew install ruby libusb libusb-compat
//   gem install digiusb
// on Linux (like Raspbian/Debian) install via apt-get:
//   sudo apt-get install ruby1.9.3 libusb-1.0-0-dev libusb-1.0-0
//   sudo gem install digiusb
// and either run the ruby script. If you have trouble connecting on linux
// try running it as root to work around udev permissions problems.
#include "DigiUSB.h"

// OOK sequences arrive as a long LOW pulse to the digital pin
// this long pulse is the Automatic Gain Calibration pulse, which calibrates
// the radio receiver to the signal strength of the transmitter
// It signals to our program that some data is about to arrive
// Immediately after AGC pulse we recieve pulses of varying lengths
// where data is transmitted by the duration of the pulses and pulse gaps
// This code fingerprints those pulses by cycling over a 32-bit number xoring
// over it with each pulse length thresholded by about 0.6ms, so it can
// accept more than 32 pulses and antipulses, but is effectively a one-way
// hash of the button not suitable for retransmission.

// Minimum duration of AGC pulse:
#define AGC_PULSE 4000
// Minimum duration of a pulse which causes a 1-bit to be added:
#define PULSE_THRESHOLD 600
// How many times to receive this code before transmitting it to the computer:
#define REPEATS 1

// pin which connects to 433.92mhz ook receiver data connection
// don't change this - it's tightly coupled to the hardware interrupt used
// and digisparks only have one hardware interrupt. We can't use PCINT because
// DigiUSB is using that.
#define DATA_IN 2

void setup() {
  // pin 0 is our receiver data line
  pinMode(DATA_IN, INPUT);
  digitalWrite(DATA_IN, LOW);
  DigiUSB.begin();
  
  attachInterrupt(0, on_pin_change, CHANGE);
}

uint32_t last_code = 0;
boolean code_received = false;
uint32_t code = 0;
unsigned long last_change_time;
unsigned long time_now;
unsigned int pulse_duration; // duration of most recent pulse in microseconds
boolean busy = false;
void on_pin_change() {
  if (!busy && !code_received) {
    busy = true; // make sure we don't burst the stack
    
    time_now = micros();
    pulse_duration = time_now - last_change_time;
    
    if (pulse_duration > AGC_PULSE) { // reset - this is gain control pulse
      last_code = code;
      code_received = true;
      code = 0;
    } else {
      boolean spare_bit = code >> 31;
      code <<= 1; // shift across one for the next bit
      code |= spare_bit; // recycle bit around to the other end.
      
      if (pulse_duration > PULSE_THRESHOLD) {
        code ^= 1; // xor a bit in for long pulses but not short ones
      }
    }
    
    last_change_time = time_now;
    busy = false;
  }
}

uint32_t counted_code;
byte repeat_counter = 0;
void loop() {
  if (code_received) {
    if (last_code == counted_code) {
      repeat_counter++;
      
      if (repeat_counter == REPEATS && DigiUSB.tx_remaining() > 8) {
        DigiUSB.println(counted_code, HEX);
        DigiUSB.delay(1);
      }
    } else {
      counted_code = last_code;
      repeat_counter = 0;
    }
    
    code_received = false;
  }
  
  DigiUSB.refresh();  
}

