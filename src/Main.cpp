// Dungeon: a maze-chase game.
//
// This file only connects the rules (Game) to the window (Screen). The
// rules live in Game.cpp, movement in Actor.cpp, the monsters in
// Monsters.cpp and the maze in Maze.cpp.

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "Game.hpp"
#include "Music.hpp"
#include "Screen.hpp"

// Art by Kenney (CC0); see README.
#define TILES ASSETS_DIR "kenney/tiny-dungeon/"
#define UI    ASSETS_DIR "kenney/pixel-ui/"
#define FONTS ASSETS_DIR "kenney/fonts/"
// Songs by Aureolus_Omicron (CC0); see README.
#define SONGS ASSETS_DIR "music/"


// Changes the music when the game moves from one stage to another.
static void followStage (Music &music, Stage from, Stage to) {
   if (to == PAUSED)  { music.setPaused(true); return; }
   if (from == PAUSED) music.setPaused(false);

   switch (to) {
      case MENU:      if (from != MENU) music.menu(); break;
      case CAUGHT:    music.caught(); break;
      case GAME_OVER: music.gameOver(); break;
      case READY:
         if (from == MENU || from == GAME_OVER) music.game();   // a new game
         else if (from == CAUGHT)               music.resume(); // next life
         break;
      default: break;
   }
}


// Settings file: ~/.config/dungeon/settings.txt (or under $XDG_CONFIG_HOME).
static std::filesystem::path settingsFile () {
   const char *xdg  = std::getenv("XDG_CONFIG_HOME");
   const char *home = std::getenv("HOME");
   std::filesystem::path base = xdg && *xdg ? xdg : std::string(home ? home : ".") + "/.config";
   return base / "dungeon" / "settings.txt";
}

static void loadSettings (Game &game) {
   std::ifstream in(settingsFile());
   std::string key;
   int value;
   while (in >> key >> value) {
      if (key == "music_volume") game.musicVolume = std::clamp(value, 0, MAX_VOLUME);
      if (key == "sound_volume") game.soundVolume = std::clamp(value, 0, MAX_VOLUME);
      if (key == "high_score")   game.highScore   = std::max(value, 0);
   }
}

static void saveSettings (const Game &game) {
   std::error_code ignored;
   std::filesystem::create_directories(settingsFile().parent_path(), ignored);
   std::ofstream out(settingsFile());
   out << "music_volume " << game.musicVolume << "\n"
       << "sound_volume " << game.soundVolume << "\n"
       << "high_score "   << game.highScore   << "\n";
}

// The sound for each thing that happens in the game.
static SoundEffect soundFor (GameEvent e) {
   switch (e) {
      case COIN_EATEN:    return SFX_COIN;
      case POTION_TAKEN:  return SFX_POTION;
      case MONSTER_EATEN: return SFX_MONSTER;
      case HERO_CAUGHT:   return SFX_CAUGHT;
      case LEVEL_CLEARED: return SFX_LEVEL;
      case MENU_MOVED:    return SFX_MENU_MOVE;
      case MENU_CHOSEN:   return SFX_MENU_CHOOSE;
   }
   return SFX_MENU_MOVE;
}


// What the menu says for the current stage.
static Screen::Menu menuFor (const Game &game) {
   Screen::Menu menu;
   menu.visible  = game.inMenu();
   menu.options  = game.menuOptions();
   menu.selected = game.menuChoice;

   // Arrows around a volume while it is selected: Left/Right change it.
   auto actions = game.menuActions();
   if (menu.selected < (int) actions.size() &&
       (actions[menu.selected] == CHANGE_MUSIC || actions[menu.selected] == CHANGE_SOUND))
      menu.options[menu.selected] = "< " + menu.options[menu.selected] + " >";

   switch (game.stage) {
      case MENU:
         menu.title    = "DUNGEON";
         menu.subtitle = game.highScore > 0
                         ? "HIGH SCORE " + std::to_string(game.highScore)
                         : "GRAB THE COINS, DODGE THE MONSTERS";
         break;
      case PAUSED:
         menu.title    = "PAUSED";
         menu.subtitle = "SCORE " + std::to_string(game.score);
         break;
      case GAME_OVER:
         menu.title    = "GAME OVER";
         menu.subtitle = "SCORE " + std::to_string(game.score);
         break;
      default:
         break;
   }
   return menu;
}


int main () {

   Game game;
   if (!game.prepare (ASSETS_DIR "maze.txt")) {
      std::cerr << "Could not load maze.txt\n";
      return 1;
   }

   Screen screen (game.maze.map);

   // Images. Characters are drawn in the order they are added:
   // the hero last, on top of the monsters.
   for (auto &m: game.monsters) {
      m.actor.npc.sprite (TILES + m.image);
      screen.add (&m.actor.npc);
   }
   game.hero.npc.sprite (TILES "knight.png");
   screen.add (&game.hero.npc);

   // A frightened monster turns into a ghost.
   sf::Texture ghost;
   if (!ghost.loadFromFile (TILES "ghost.png")) {
      std::cerr << "Could not load ghost.png\n";
      return 1;
   }

   screen.setWallImage  (TILES "wall.png");
   screen.setFloorImage (TILES "floor.png");
   screen.setItems      (&game.maze.items());
   screen.setItemColor  (COIN, sf::Color(255, 210, 90), 0.1f);
   screen.setItemImage  (POTION, TILES "potion.png");

   loadSettings (game);

   Music music;
   bool hasMusic = music.load (SONGS);
   if (!hasMusic)
      std::cerr << "Could not load the songs; playing without music.\n";

   // Volumes and high score: applied right away, saved for next time.
   int savedMusic = -1, savedSound = -1, savedHigh = game.highScore;
   auto followSettings = [&]() {
      if (game.musicVolume != savedMusic || game.soundVolume != savedSound) {
         music.setMusicVolume ((float) game.musicVolume / MAX_VOLUME);
         music.setSoundVolume ((float) game.soundVolume / MAX_VOLUME);
         if (savedMusic >= 0)
            saveSettings (game);
         savedMusic = game.musicVolume;
         savedSound = game.soundVolume;
      }
      // The high score is saved when a game ends or is left, not every frame.
      if (game.highScore != savedHigh && game.stage != PLAYING) {
         saveSettings (game);
         savedHigh = game.highScore;
      }
   };
   followSettings();

   // Sound effects for what just happened.
   auto playEvents = [&]() {
      if (hasMusic)
         for (GameEvent e: game.events)
            music.play (soundFor(e));
      game.events.clear();
   };

   // Stage changes come from keys (menus) and from the game itself.
   Stage lastStage = MENU;
   auto followGame = [&]() {
      if (hasMusic && game.stage != lastStage)
         followStage(music, lastStage, game.stage);
      lastStage = game.stage;
   };

   screen.onKey ([&](sf::Keyboard::Key k) {
      game.key(k);
      followGame();
      followSettings();
      playEvents();
      if (game.quitRequested)
         screen.close();
   });

   screen.onFrame ([&](float dt) {
      // A very long frame (window dragged, first frame...) would make
      // everyone jump several tiles at once.
      game.update (std::min(dt, 0.05f));
      followGame();
      followSettings();
      playEvents();
      if (hasMusic)
         music.setDanger (game.danger);

      for (auto &m: game.monsters)
         m.actor.npc.m_sprite.setTexture (m.frightened ? ghost : m.actor.npc.m_texture);

      Screen::Hud hud;
      hud.score     = game.score;
      hud.highScore = game.highScore;
      hud.level     = game.level;
      hud.lives     = game.lives;
      hud.bar       = game.frightenedTime > 0
                      ? game.frightenedTime / game.potionDuration() : -1;
      hud.banner    = game.banner();
      screen.setHud (hud);
      screen.setMenu (menuFor(game));
   });

   game.showMenu();
   if (hasMusic)
      music.menu();

   if (!screen.open (900, 1000, "Dungeon"))
      return 1;

   if (!screen.loadInterface (UI, FONTS "kenpixel.ttf", FONTS "kenpixel_blocks.ttf"))
      std::cerr << "Could not load the interface art; playing without HUD and menus.\n";
   screen.setLifeIcon (TILES "knight.png");

   screen.run();

   // Closing the window in the middle of a game keeps the high score too.
   if (game.highScore != savedHigh)
      saveSettings (game);
   return 0;
}
