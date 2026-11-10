#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

/**
 * A small 8-bit style synthesizer that plays MIDI songs.
 *
 * Songs are read with TinyMidiLoader (third_party/tml.h). Each MIDI channel
 * becomes a Part that plays one note at a time with a simple waveform
 * (pulse, saw or triangle); the drum channel becomes noise and pitch-swept
 * hits. Output is mono, 16-bit, at SAMPLE_RATE.
 *
 * Effects:
 *   intensity   0..1. Bass and drums grow with it; at 0 they are almost
 *               silent, at 1 the song plays in full.
 *   muffled     low-pass filter and lower volume (used while paused).
 *   tape stop   speed and pitch slide down to a halt, like a tape deck
 *               losing power; resume() spins it back up.
 *
 * Every parameter moves smoothly towards its target, so nothing jumps.
 * No audio device is used here: render() just fills a buffer.
 *
 * Sound effects are short pitch slides played on top of the music, with
 * their own volume; they are not muffled and not slowed by a tape stop.
 */

enum SoundEffect {
   SFX_COIN, SFX_POTION, SFX_MONSTER, SFX_CAUGHT, SFX_LEVEL,
   SFX_MENU_MOVE, SFX_MENU_CHOOSE
};

struct Note {
   double start;      // seconds
   double end;        // seconds
   int    key;        // MIDI note number (69 = A4 = 440 Hz)
   float  velocity;   // 0..1
};

enum Role { LEAD, HARMONY, BASS, DRUMS };

struct Part {
   int  channel {0};
   int  program {0};          // General MIDI instrument
   Role role {LEAD};
   std::vector<Note> notes;   // sorted by start
};

struct Song {
   std::string name;
   double bpm {120};
   double length {0};         // seconds, rounded up to a whole 4/4 bar
   std::vector<Part> parts;

   bool load (const std::string &file);
};

class Synth {
public:
   static constexpr int SAMPLE_RATE = 44100;

   // Starts a song from the beginning. With loop = false, finished() turns
   // true at its end and the synth falls silent.
   void play (const Song *song, bool loop);
   void stop () { m_song = nullptr; }
   bool finished () const { return m_song == nullptr; }
   const Song *song () const { return m_song; }

   void setIntensity (float target) { m_intensityTarget = target; }
   void setMuffled (bool on) { m_muffledTarget = on ? 1.0f : 0.0f; }
   void tapeStop () { m_stopping = true; }
   void resume ()   { m_stopping = false; }

   // Volumes from 0 to 1.
   void setMusicVolume   (float v) { m_musicVolume = v; }
   void setEffectsVolume (float v) { m_effectsVolume = v; }

   void playEffect (SoundEffect effect);

   void render (std::int16_t *out, std::size_t count);

   // A piece of a sound effect: a slide from one pitch to another.
   enum Wave { SQUARE, THIN_PULSE, TRIANGLE };
   struct Slide {
      float from, to;     // Hz
      float length;       // seconds
      Wave  wave;
      float gain;
   };

private:
   struct Effect {
      const std::vector<Slide> *slides;
      std::size_t index {0};
      double time {0};             // within the current slide
      double phase {0};
   };
   std::vector<Effect> m_effects;
   float m_musicVolume {1}, m_effectsVolume {1};

   float effectsSample (double dt);
   struct Voice {                  // one per part
      std::size_t cursor {0};      // first note that may still sound
      double phase {0};
   };
   struct Hit {                    // a drum hit in progress
      int    key;
      double age {0};              // seconds since it started
      double phase {0};
      float  velocity;
   };

   const Song *m_song {nullptr};
   bool   m_loop {false};
   double m_time {0};              // position in the song, seconds
   std::vector<Voice> m_voices;
   std::vector<Hit>   m_hits;

   float m_intensity {1}, m_intensityTarget {1};
   float m_muffled {0},   m_muffledTarget {0};
   float m_speed {1};
   bool  m_stopping {false};
   float m_lowPass {0};            // filter state

   std::minstd_rand m_noise {12345};
   float m_previousNoise {0};

   void  restart ();
   float partSample (const Part &part, Voice &voice, double dt);
   float drumSample (double dt);
   void  startHits (const Part &part, Voice &voice);
};
