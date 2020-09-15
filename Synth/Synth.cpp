#include <iostream>

#include "olcNoiseMaker.h"

// General purpose oscillator
enum class OSC { SINE, SQUARE, TRIANGLE, SAW_ANALOG, SAW_DIGITAL, NOISE };

// Amplitude (Attack, Decay, Sustain, Release) Envelope
struct sEnvelopeADSR {
  double dAttackTime;
  double dDecayTime;
  double dSustainAmplitude;
  double dReleaseTime;
  double dStartAmplitude;
  double dTriggerOffTime;
  double dTriggerOnTime;
  bool bNoteOn;

  sEnvelopeADSR() {
    dAttackTime = 0.10;
    dDecayTime = 0.01;
    dStartAmplitude = 1.0;
    dSustainAmplitude = 0.8;
    dReleaseTime = 0.20;
    bNoteOn = false;
    dTriggerOffTime = 0.0;
    dTriggerOnTime = 0.0;
  }

  sEnvelopeADSR(double attack, double decay, double sustain, double realease)
      : dAttackTime(attack),
        dDecayTime(decay),
        dSustainAmplitude(sustain),
        dReleaseTime(realease) {
    dStartAmplitude = 1.0;
    bNoteOn = false;
    dTriggerOffTime = 0.0;
    dTriggerOnTime = 0.0;
  }

  // Call when key is pressed
  void NoteOn(double dTimeOn) {
    dTriggerOnTime = dTimeOn;
    bNoteOn = true;
  }

  // Call when key is released
  void NoteOff(double dTimeOff) {
    dTriggerOffTime = dTimeOff;
    bNoteOn = false;
  }

  // Get the correct amplitude at the requested point in time
  double GetAmplitude(double dTime) {
    double dAmplitude = 0.0;
    double dLifeTime = dTime - dTriggerOnTime;

    if (bNoteOn) {
      if (dLifeTime <= dAttackTime) {
        // In attack Phase - approach max amplitude
        dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;
      }

      if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime)) {
        // In decay phase - reduce to sustained amplitude
        dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) *
                         (dSustainAmplitude - dStartAmplitude) +
                     dStartAmplitude;
      }

      if (dLifeTime > (dAttackTime + dDecayTime)) {
        // In sustain phase - dont change until note released
        dAmplitude = dSustainAmplitude;
      }
    } else {
      // Note has been released, so in release phase
      dAmplitude = ((dTime - dTriggerOffTime) / dReleaseTime) *
                       (0.0 - dSustainAmplitude) +
                   dSustainAmplitude;
    }

    // Amplitude should not be negative
    if (dAmplitude <= 0.0001) dAmplitude = 0.0;

    return dAmplitude;
  }
};

atomic<double> dFrequencyOutput = 0.0;
double dOctaveBaseFrequency = 110.0;
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);

sEnvelopeADSR envelope(0.0, 0.1, 1.0, 0.1);

double makeNoise(double dTime);
double w(double dHertz);
double oscilator(double dHertz, double dTime, OSC nType);

int main() {
  std::wcout << "Michael Henrique - Synthesizer" << std::endl;

  std::vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

  for (auto device : devices)
    std::wcout << " Found Output Device " << device << std::endl;

  wcout << endl
        << "|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |" << endl
        << "|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |" << endl
        << "|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__"
        << endl
        << "|     |     |     |     |     |     |     |     |     |     |"
        << endl
        << "|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  |"
        << endl
        << "|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|"
        << endl
        << endl;

  olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

  sound.SetUserFunction(makeNoise);

  int nCurrentKey = -1;
  bool bKeyPressed = false;
  while (1) {
    bKeyPressed = false;
    for (int k = 0; k < 16; k++) {
      if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe\xbf"[k])) &
          0x8000) {
        if (nCurrentKey != k) {
          dFrequencyOutput = dOctaveBaseFrequency * pow(d12thRootOf2, k);
          envelope.NoteOn(sound.GetTime());
          wcout << "\rNote On : " << sound.GetTime() << "s " << dFrequencyOutput
                << "Hz";
          nCurrentKey = k;
        }

        bKeyPressed = true;
      }
    }

    if (!bKeyPressed) {
      if (nCurrentKey != -1) {
        wcout << "\rNote Off: " << sound.GetTime()
              << "s                        ";

        envelope.NoteOff(sound.GetTime());
        nCurrentKey = -1;
      }
    }
  }

  return 0;
}

double w(double dHertz) { return 2.0 * PI * dHertz; }

double oscilator(double dHertz, double dTime, OSC nType) {
  switch (nType) {
    case OSC::SINE:  // Sine wave bewteen -1 and +1
      return sin(w(dHertz) * dTime);

    case OSC::SQUARE:  // Square wave between -1 and +1
      return sin(w(dHertz) * dTime) > 0 ? 1.0 : -1.0;

    case OSC::TRIANGLE:  // Triangle wave between -1 and +1
      return asin(sin(w(dHertz) * dTime)) * (2.0 / PI);

    case OSC::SAW_ANALOG:  // Saw wave (analogue / warm / slow)
    {
      double dOutput = 0.0;

      for (double n = 1.0; n < 100.0; n++)
        dOutput += (sin(n * w(dHertz) * dTime)) / n;

      return dOutput * (2.0 / PI);
    }

    case OSC::SAW_DIGITAL:  // Saw Wave (optimised / harsh / fast)
      return (2.0 / PI) *
             (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));

    case OSC::NOISE:  // Pseudorandom noise
      return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;

    default:
      return 0.0;
  }
}

double makeNoise(double dTime) {
  return envelope.GetAmplitude(dTime) *
         oscilator(dFrequencyOutput, dTime, OSC::SAW_ANALOG);
}
