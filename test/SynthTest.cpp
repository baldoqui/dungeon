// Synthesizer tests: render into a buffer, no sound card needed.

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "Synth.hpp"

namespace {

const int RATE = Synth::SAMPLE_RATE;

std::vector<std::int16_t> render (Synth &synth, double seconds) {
   std::vector<std::int16_t> out((size_t) (seconds * RATE));
   synth.render(out.data(), out.size());
   return out;
}

double energy (const std::vector<std::int16_t> &s, size_t from, size_t to) {
   double sum = 0;
   for (size_t i = from; i < to && i < s.size(); ++i)
      sum += (double) s[i] * s[i];
   return sum / std::max<size_t>(1, to - from);
}

// A song with a single part playing one long note.
Song oneNote (Role role, int key, int program = 80) {
   Song song;
   song.length = 4;
   Part part;
   part.role = role;
   part.program = program;
   if (role == DRUMS) {
      // A kick every quarter second.
      for (int i = 0; i < 16; ++i)
         part.notes.push_back({i * 0.25, i * 0.25 + 0.1, 36, 1.0f});
   } else {
      part.notes.push_back({0.0, 4.0, key, 1.0f});
   }
   song.parts.push_back(part);
   return song;
}


TEST(Song, LoadsTheDungeonMidi) {
   Song song;
   ASSERT_TRUE(song.load(ASSETS_DIR "music/dungeon.mid"));
   EXPECT_NEAR(song.bpm, 108, 0.5);
   EXPECT_EQ(song.parts.size(), 6u);
   // 32 bars of 4/4 at 108 bpm.
   EXPECT_NEAR(song.length, 32 * 4 * 60.0 / 108, 0.01);

   int drums = 0, bass = 0, lead = 0, melodyNotes = 0;
   for (auto &p: song.parts) {
      drums += p.role == DRUMS;
      bass  += p.role == BASS;
      lead  += p.role == LEAD;
      if (p.program == 80) melodyNotes = (int) p.notes.size();
   }
   EXPECT_EQ(drums, 1);
   EXPECT_EQ(bass, 1);
   EXPECT_EQ(lead, 2);
   EXPECT_EQ(melodyNotes, 240);
}

TEST(Song, AllGameSongsLoad) {
   for (const char *name: {"title_screen", "dungeon", "in_the_royal_court",
                           "fountain_of_the_fairies", "game_over"}) {
      Song song;
      EXPECT_TRUE(song.load(std::string(ASSETS_DIR "music/") + name + ".mid")) << name;
      EXPECT_GT(song.length, 10) << name;
   }
}

TEST(Synth, PlaysTheRightPitch) {
   Song song = oneNote(LEAD, 69);   // A4 = 440 Hz
   Synth synth;
   synth.play(&song, false);
   auto out = render(synth, 1.0);

   // A square wave crosses zero twice per cycle.
   int crossings = 0;
   for (size_t i = 1; i < out.size(); ++i)
      crossings += (out[i - 1] < 0) != (out[i] < 0);
   EXPECT_NEAR(crossings / 2.0, 440, 440 * 0.02);
}

TEST(Synth, DrumsGrowWithIntensity) {
   Song song = oneNote(DRUMS, 0);

   Synth calm;
   calm.setIntensity(0);
   calm.play(&song, true);
   render(calm, 2.0);                       // let the smoothing settle
   double quiet = energy(render(calm, 1.0), 0, RATE);

   Synth tense;
   tense.setIntensity(1);
   tense.play(&song, true);
   render(tense, 2.0);
   double loud = energy(render(tense, 1.0), 0, RATE);

   EXPECT_GT(loud, quiet * 20);
}

TEST(Synth, TapeStopEndsInSilence) {
   Song song = oneNote(LEAD, 60);
   Synth synth;
   synth.play(&song, true);
   render(synth, 0.2);
   synth.tapeStop();
   auto out = render(synth, 1.0);
   EXPECT_GT(energy(out, 0, RATE / 10), 0);
   EXPECT_EQ(energy(out, RATE * 9 / 10, RATE), 0);

   synth.resume();
   auto back = render(synth, 0.5);
   EXPECT_GT(energy(back, RATE / 4, RATE / 2), 0);
}

TEST(Synth, MuffledIsQuieter) {
   Song song = oneNote(LEAD, 72);
   Synth open, muffled;
   open.play(&song, true);
   muffled.setMuffled(true);
   muffled.play(&song, true);
   double a = energy(render(open, 2.0), RATE, 2 * RATE);
   double b = energy(render(muffled, 2.0), RATE, 2 * RATE);
   EXPECT_LT(b, a * 0.5);
}

TEST(Synth, EffectsPlayWithoutMusicAndEnd) {
   Synth synth;   // no song
   synth.playEffect(SFX_POTION);
   auto out = render(synth, 0.5);
   EXPECT_GT(energy(out, 0, RATE / 4), 0);              // 4 notes, 0.3 s
   EXPECT_EQ(energy(out, RATE * 2 / 5, RATE / 2), 0);   // and then silence
}

TEST(Synth, EffectsHaveTheirOwnVolumeAndAreNotMuffled) {
   auto effectEnergy = [](float effects, float music, bool muffled) {
      Synth s;
      s.setMusicVolume(music);
      s.setEffectsVolume(effects);
      s.setMuffled(muffled);
      std::vector<std::int16_t> warm(RATE);
      s.render(warm.data(), warm.size());   // let the muffling settle
      s.playEffect(SFX_CAUGHT);
      return energy(render(s, 0.5), 0, RATE / 2);
   };
   double normal = effectEnergy(1, 1, false);
   EXPECT_GT(normal, 0);
   EXPECT_EQ(effectEnergy(0, 1, false), 0);                 // effects muted
   EXPECT_NEAR(effectEnergy(1, 0, true), normal, normal * 0.01);   // music off, paused
}

TEST(Synth, SongWithoutLoopFinishes) {
   Song song = oneNote(LEAD, 60);
   Synth synth;
   synth.play(&song, false);
   render(synth, 4.1);
   EXPECT_TRUE(synth.finished());

   synth.play(&song, true);
   render(synth, 4.1);
   EXPECT_FALSE(synth.finished());
}

}  // namespace
