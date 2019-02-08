/*
Every 8 seconds play a sound. Ideally dynamically check for the next numbered sound file.
sound00.wav, sound01.wav, etc.
*/

#include <SD.h>
#define SD_ChipSelectPin 4

#include <TMRpcm.h>

#define COMPLETE_PIN 2

#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

TMRpcm tmrpcm;
volatile int debounce = 0;
volatile int playSound = 0;
int nextSound = 0;
int complete = 0;

ISR(WDT_vect) {
  wdt_reset();
  playSound = 1;
}

void puzzleCompleteHandler() {
  // Check if we're complete
  if (digitalRead(COMPLETE_PIN) == HIGH) {
    // Start debouncing, this will prevent sleeping
    debounce = 1;
  } else {
    // Stop debouncing, this will make the main loop send us to sleep
    debounce = 0;
  }
}

void play(char *pname)
{
  /*Serial.print("Playing ");
  Serial.print(pname);
  Serial.print(" ... ");*/
  tmrpcm.play(pname);
  delay(100);
  while (tmrpcm.isPlaying())
  {
    delay(100);
  }
  //Serial.println("done");
}

void playNext() {
  char filename [16];
  sprintf(filename, "sound%02d.wav", nextSound++);
  if (!SD.exists(filename)) {
    nextSound = 1;
    strcpy (filename, "sound00.wav");
  }
  play(filename);
}

void setup()
{
  //Serial.begin(9600);

  SD.begin(SD_ChipSelectPin);

  tmrpcm.speakerPin = 9;
  pinMode(10, OUTPUT);
  tmrpcm.setVolume(5);
  tmrpcm.quality(1);

  play("start.wav");

  noInterrupts();

  // Enable interrupts when there is a hardware pin change
  attachInterrupt(digitalPinToInterrupt(COMPLETE_PIN), puzzleCompleteHandler, CHANGE);

  // Enable WDT interrupts every 8 seconds
  MCUSR = 0;
  WDTCSR |= 0b00011000;
  WDTCSR =  0b01000000 | 0b100001;

  interrupts();
}

void debugSounds()
{
  int n;

  while ((n = Serial.read()) != -1)
  {
    switch (n)
    {
    case 'w':
      play("herewego.wav");
      break;
    case 'b':
      play("burned.wav");
      break;
    case 'l':
      play("laugh.wav");
      break;
    case 'm':
      play("mamma.wav");
      break;
    case 's':
      play("start.wav");
      break;
    }
  }
}

void loop()
{
  if (debounce == 0) {
    // Set speaker output pins low so they stay low during sleep otherwise they are high impedance and the speaker picks up the PWM frequency.
    digitalWrite(9, LOW);
    digitalWrite(10, LOW);

    // Sleep, only an interrupt will wake us
    power_all_disable();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_mode(); // BLOCKS

    // Back from sleep, play a sound if flagged
    power_all_enable();

    if (playSound) {
      if (complete) {
        play("herewego.wav");
      } else {
        playNext();
      }
      playSound = 0;
    }

    return;
  }

  // We're counting the debouncer, check if it's reached its limit
  if (++debounce == 50) {
    complete = 1;
    play("herewego.wav");
    debounce = 0;
  } else {
    delay(10);
  }
}
