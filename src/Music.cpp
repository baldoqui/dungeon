#include "Music.hpp"

Music::~Music () {
   // SFML requires derived streams to stop before their members go away.
   stop();
}

bool Music::load (const std::string &folder) {
   m_playlist.resize(3);
   bool ok = m_title.load(folder + "title_screen.mid") &&
             m_gameOver.load(folder + "game_over.mid") &&
             m_playlist[0].load(folder + "dungeon.mid") &&
             m_playlist[1].load(folder + "in_the_royal_court.mid") &&
             m_playlist[2].load(folder + "fountain_of_the_fairies.mid");
   if (ok)
      initialize(1, Synth::SAMPLE_RATE);
   return ok;
}

void Music::start () {
   if (getStatus() != Playing)
      play();
}

void Music::menu () {
   std::lock_guard<std::mutex> guard(m_lock);
   m_mode = MENU_MUSIC;
   m_synth.setMuffled(false);
   m_synth.setIntensity(1);
   m_synth.play(&m_title, true);
   start();
}

void Music::game () {
   std::lock_guard<std::mutex> guard(m_lock);
   m_mode = GAME_MUSIC;
   m_synth.setMuffled(false);
   m_synth.setIntensity(0);
   m_synth.play(&m_playlist[0], false);
   m_next = 1;
   start();
}

void Music::gameOver () {
   std::lock_guard<std::mutex> guard(m_lock);
   m_mode = GAME_OVER_MUSIC;
   m_synth.setMuffled(false);
   m_synth.setIntensity(1);
   m_synth.play(&m_gameOver, false);
   start();
}

void Music::setDanger (float danger) {
   std::lock_guard<std::mutex> guard(m_lock);
   if (m_mode == GAME_MUSIC)
      m_synth.setIntensity(danger);
}

void Music::setPaused (bool paused) {
   std::lock_guard<std::mutex> guard(m_lock);
   m_synth.setMuffled(paused);
}

void Music::caught () {
   std::lock_guard<std::mutex> guard(m_lock);
   m_synth.tapeStop();
}

void Music::resume () {
   std::lock_guard<std::mutex> guard(m_lock);
   m_synth.resume();
}

bool Music::onGetData (Chunk &data) {
   std::lock_guard<std::mutex> guard(m_lock);

   // During a game, when a song ends the next one starts.
   if (m_mode == GAME_MUSIC && m_synth.finished() && !m_playlist.empty()) {
      m_synth.play(&m_playlist[m_next], false);
      m_next = (m_next + 1) % m_playlist.size();
   }

   m_buffer.resize(2048);   // about 46 ms
   m_synth.render(m_buffer.data(), m_buffer.size());

   data.samples = m_buffer.data();
   data.sampleCount = m_buffer.size();
   return true;   // keep streaming (silence when nothing plays)
}
