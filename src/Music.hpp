#pragma once

#include <mutex>
#include <string>
#include <vector>

#include <SFML/Audio/SoundStream.hpp>

#include "Synth.hpp"

/**
 * The game's soundtrack: plays the Synth through SFML's audio output and
 * picks the song for each moment.
 *
 *   menu        "Title Screen", looping
 *   game        "Dungeon", "In the Royal Court", "Fountain of the Fairies",
 *               one after the other, around and around
 *   game over   "Game Over", once
 *
 * During a game the music follows the danger (0..1): bass and drums grow
 * as a monster gets closer. Pause muffles it; being caught stops it like a
 * tape, and the next life spins it back up.
 *
 * Sound effects go through the same output, with their own volume.
 */
class Music : private sf::SoundStream {
public:
   ~Music () override;

   // Loads the five MIDI files from a folder; false if any is missing.
   bool load (const std::string &folder);

   void menu ();
   void game ();        // from the first song of the playlist
   void gameOver ();

   void setDanger (float danger);
   void setMusicVolume (float volume);   // 0..1
   void setSoundVolume (float volume);   // 0..1, sound effects
   void play (SoundEffect effect);
   void setPaused (bool paused);
   void caught ();      // tape stop
   void resume ();      // after being caught

private:
   enum Mode { MENU_MUSIC, GAME_MUSIC, GAME_OVER_MUSIC };

   std::mutex m_lock;           // SFML calls onGetData from its own thread
   Synth m_synth;
   Mode  m_mode {MENU_MUSIC};
   Song  m_title, m_gameOver;
   std::vector<Song> m_playlist;
   std::size_t m_next {0};      // next song of the playlist
   std::vector<std::int16_t> m_buffer;

   void start ();
   bool onGetData (Chunk &data) override;
   void onSeek (sf::Time) override {}
};
