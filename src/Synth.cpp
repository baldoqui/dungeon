#include "Synth.hpp"

#include <algorithm>
#include <cmath>
#include <map>

#define TML_IMPLEMENTATION
#include "tml.h"

static const double TWO_PI = 6.283185307179586;


//--------------------------------------------------------------
// Reading a song

bool Song::load (const std::string &file) {

   tml_message *first = tml_load_filename(file.c_str());
   if (first == nullptr)
      return false;

   name = file;
   bpm = 120;
   for (tml_message *m = first; m; m = m->next)
      if (m->type == TML_SET_TEMPO) {
         bpm = 60e6 / tml_get_tempo_value(m);
         break;
      }

   // Notes are built from note-on / note-off pairs, per channel and key.
   struct Open { double start; float velocity; };
   std::map<int, Part> byChannel;
   std::map<int, std::map<int, Open>> open;
   double lastEnd = 0;

   auto close = [&](int channel, int key, double time) {
      auto it = open[channel].find(key);
      if (it == open[channel].end())
         return;
      byChannel[channel].notes.push_back({it->second.start, time, key, it->second.velocity});
      lastEnd = std::max(lastEnd, time);
      open[channel].erase(it);
   };

   for (tml_message *m = first; m; m = m->next) {
      double t = m->time / 1000.0;
      int channel = m->channel;
      int key = (unsigned char) m->key;
      int velocity = (unsigned char) m->velocity;

      if (m->type == TML_PROGRAM_CHANGE) {
         byChannel[channel].program = (unsigned char) m->program;
      } else if (m->type == TML_NOTE_ON && velocity > 0) {
         close(channel, key, t);
         open[channel][key] = {t, velocity / 127.0f};
         byChannel[channel].channel = channel;
      } else if (m->type == TML_NOTE_OFF || m->type == TML_NOTE_ON) {
         close(channel, key, t);
      }
   }
   tml_free(first);

   for (auto &[channel, keys]: open)
      while (!keys.empty())
         close(channel, keys.begin()->first, lastEnd);

   // Length: up to the end of the last bar, so loops keep the beat.
   const double bar = 4 * 60.0 / bpm;
   length = std::max(bar, std::ceil(lastEnd / bar - 1e-3) * bar);

   // Roles: drums, bass, lead (the "lead" synth instruments) or harmony.
   parts.clear();
   for (auto &[channel, part]: byChannel) {
      if (part.notes.empty())
         continue;
      std::sort(part.notes.begin(), part.notes.end(),
                [](const Note &a, const Note &b) { return a.start < b.start; });
      part.channel = channel;
      if (channel == 9)                                part.role = DRUMS;
      else if (part.program >= 32 && part.program < 40) part.role = BASS;
      else if (part.program >= 80 && part.program < 88) part.role = LEAD;
      else                                             part.role = HARMONY;
      parts.push_back(part);
   }

   // Without a bass instrument, the lowest part plays the bass role.
   bool hasBass = std::any_of(parts.begin(), parts.end(),
                              [](const Part &p) { return p.role == BASS; });
   if (!hasBass) {
      Part *lowest = nullptr;
      double lowestAverage = 1e9;
      for (auto &p: parts) {
         if (p.role == DRUMS)
            continue;
         double sum = 0;
         for (auto &n: p.notes) sum += n.key;
         double average = sum / p.notes.size();
         if (average < lowestAverage) { lowestAverage = average; lowest = &p; }
      }
      if (lowest && lowestAverage < 55)   // below G3
         lowest->role = BASS;
   }

   return !parts.empty();
}


//--------------------------------------------------------------
// Playback

void Synth::play (const Song *song, bool loop) {
   m_song = song;
   m_loop = loop;
   m_stopping = false;
   m_speed = 1;
   m_hits.clear();
   restart();
}

void Synth::restart () {
   m_time = 0;
   m_voices.assign(m_song ? m_song->parts.size() : 0, Voice{});
}

// Waveform for a part, at a phase from 0 to 1.
static float wave (const Part &part, double phase) {
   if (part.role == BASS || part.role == HARMONY)
      return (float) (1 - 4 * std::abs(phase - 0.5));      // triangle
   if (part.program == 81)
      return (float) (2 * phase - 1);                       // saw
   if (part.program >= 82 && part.program < 88)
      return phase < 0.25 ? 1.0f : -1.0f;                   // thin pulse
   return phase < 0.5 ? 1.0f : -1.0f;                       // square
}

float Synth::partSample (const Part &part, Voice &voice, double dt) {

   const auto &notes = part.notes;

   // One note at a time: the latest one that has started.
   while (voice.cursor + 1 < notes.size() && notes[voice.cursor + 1].start <= m_time)
      ++voice.cursor;
   if (voice.cursor >= notes.size())
      return 0;

   const Note &note = notes[voice.cursor];
   if (m_time < note.start)
      return 0;

   // Envelope: 4 ms attack, 60 ms release after the note ends.
   const double attack = 0.004, release = 0.06;
   double envelope;
   if (m_time < note.end) {
      envelope = std::min(1.0, (m_time - note.start) / attack);
   } else {
      double after = m_time - note.end;
      if (after >= release)
         return 0;
      envelope = (1 - after / release) * std::min(1.0, (note.end - note.start) / attack);
   }

   // dt already includes the playback speed, so a tape stop lowers the pitch too.
   double frequency = 440.0 * std::pow(2.0, (note.key - 69) / 12.0);
   voice.phase += frequency * dt;
   voice.phase -= std::floor(voice.phase);

   return wave(part, voice.phase) * (float) envelope * note.velocity;
}

void Synth::startHits (const Part &part, Voice &voice) {
   while (voice.cursor < part.notes.size() && part.notes[voice.cursor].start <= m_time) {
      const Note &n = part.notes[voice.cursor];
      m_hits.push_back({n.key, 0, 0, n.velocity});
      ++voice.cursor;
   }
   if (m_hits.size() > 16)
      m_hits.erase(m_hits.begin(), m_hits.end() - 16);
}

float Synth::drumSample (double dt) {

   // One noise value per sample, shared by all hits; "bright" is high-passed.
   float noise  = (float) m_noise() / (float) m_noise.max() * 2 - 1;
   float bright = (noise - m_previousNoise) * 0.5f;
   m_previousNoise = noise;

   float sum = 0;
   for (auto &h: m_hits) {
      double a = h.age;
      float v = 0;
      double life;

      if (h.key == 35 || h.key == 36) {                       // kick
         h.phase += (45 + 110 * std::exp(-a * 30)) * dt;
         v = (float) (std::sin(TWO_PI * h.phase) * std::exp(-a * 12));
         life = 0.3;
      } else if (h.key >= 37 && h.key <= 40) {               // snare
         h.phase += 190 * dt;
         v = (float) (noise * 0.8 * std::exp(-a * 22) +
                      std::sin(TWO_PI * h.phase) * 0.4 * std::exp(-a * 30));
         life = 0.25;
      } else if (h.key == 42 || h.key == 44) {               // closed hi-hat
         v = (float) (bright * std::exp(-a * 60));
         life = 0.08;
      } else if (h.key == 46) {                              // open hi-hat
         v = (float) (bright * std::exp(-a * 12));
         life = 0.35;
      } else if (h.key == 41 || h.key == 43 || h.key == 45 ||
                 h.key == 47 || h.key == 48 || h.key == 50) { // toms
         double f = (80 + (h.key - 41) * 12) * (1 + 0.5 * std::exp(-a * 20));
         h.phase += f * dt;
         v = (float) (std::sin(TWO_PI * h.phase) * std::exp(-a * 10));
         life = 0.35;
      } else if (h.key >= 49 && h.key <= 59) {               // cymbals
         v = (float) (bright * 0.5 * std::exp(-a * 5));
         life = 1.0;
      } else {
         v = (float) (noise * std::exp(-a * 30));
         life = 0.15;
      }

      sum += v * h.velocity;
      h.age += dt;
      if (h.age > life)
         h.age = 1e9;   // marks it for removal
   }

   m_hits.erase(std::remove_if(m_hits.begin(), m_hits.end(),
                               [](const Hit &h) { return h.age >= 1e9; }),
                m_hits.end());
   return sum;
}

void Synth::render (std::int16_t *out, std::size_t count) {

   // Smoothing: parameters reach ~63% of their target in 0.35 s.
   static const float smooth = 1.0f - (float) std::exp(-1.0 / (0.35 * SAMPLE_RATE));
   // Muffled = one-pole low-pass at about 700 Hz.
   static const float closed = 1.0f - (float) std::exp(-TWO_PI * 700 / SAMPLE_RATE);

   for (std::size_t i = 0; i < count; ++i) {

      m_intensity += (m_intensityTarget - m_intensity) * smooth;
      m_muffled   += (m_muffledTarget   - m_muffled)   * smooth;

      // Tape stop: down to zero in 0.7 s; back up to speed in 0.25 s.
      if (m_stopping) m_speed = std::max(0.0f, m_speed - 1.0f / (0.7f * SAMPLE_RATE));
      else            m_speed = std::min(1.0f, m_speed + 1.0f / (0.25f * SAMPLE_RATE));

      float sample = 0;

      if (m_song != nullptr && m_speed > 0) {
         const double dt = m_speed / (double) SAMPLE_RATE;
         const float bassGain  = 0.22f * (0.3f + 0.7f * m_intensity);
         const float drumsGain = 0.30f * (0.1f + 0.9f * m_intensity);

         for (std::size_t p = 0; p < m_song->parts.size(); ++p) {
            const Part &part = m_song->parts[p];
            switch (part.role) {
               case DRUMS:   startHits(part, m_voices[p]); break;
               case LEAD:    sample += 0.20f * partSample(part, m_voices[p], dt); break;
               case HARMONY: sample += 0.13f * partSample(part, m_voices[p], dt); break;
               case BASS:    sample += bassGain * partSample(part, m_voices[p], dt); break;
            }
         }
         sample += drumsGain * drumSample(dt);

         m_time += dt;
         if (m_time >= m_song->length) {
            if (m_loop) {
               double over = m_time - m_song->length;
               restart();
               m_time = over;
            } else {
               m_song = nullptr;
            }
         }
      }

      // Pause: low-pass and quieter. Master volume grows a little with intensity.
      float alpha = 1.0f + (closed - 1.0f) * m_muffled;
      m_lowPass += alpha * (sample - m_lowPass);
      float x = m_lowPass * (1.0f - 0.6f * m_muffled) * (0.75f + 0.25f * m_intensity);

      x = std::tanh(x * 1.2f);   // soft limit instead of clipping
      out[i] = (std::int16_t) (x * 30000);
   }
}
