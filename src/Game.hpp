#pragma once

#include <memory>
#include <string>
#include <vector>

#include <SFML/Window/Keyboard.hpp>

#include "Actor.hpp"
#include "Maze.hpp"
#include "Monsters.hpp"
#include "PathFind.hpp"

/**
 * The game rules. Opens no window and draws nothing: Main.cpp connects
 * this to the Screen, so the rules can be tested without a display
 * (test/GameTest.cpp).
 *
 * Score: coin 10, potion 50, frightened monster 200, 400, 800, 1600
 * (doubling for each monster eaten on the same potion).
 *
 * Menus: the title menu (PLAY / MUSIC / SOUND / QUIT), Esc during a game
 * (RESUME / MUSIC / SOUND / MAIN MENU) and game over (PLAY AGAIN /
 * MAIN MENU). Up and Down pick an option, Enter or Space confirms it; on
 * MUSIC and SOUND, Left and Right change the volume and Enter steps it up.
 *
 * Sounds: the rules play nothing; they push GameEvents to "events" and
 * Main.cpp turns them into sound effects.
 */

enum Stage {
   MENU,          // title menu
   READY,         // pause before a life starts
   PLAYING,
   PAUSED,        // Esc during a game
   CAUGHT,        // a monster got the hero
   LEVEL_CLEAR,   // every item eaten
   GAME_OVER,     // no lives left
};

const int MONSTER_COUNT = 4;
const int MAX_VOLUME = 10;           // volume steps (10 = 100%)

// What a menu option does.
enum MenuAction {
   START_GAME, RESUME_GAME, CHANGE_MUSIC, CHANGE_SOUND, BACK_TO_TITLE, QUIT_GAME
};

// Something that happened, for the sound effects.
enum GameEvent {
   COIN_EATEN, POTION_TAKEN, MONSTER_EATEN, HERO_CAUGHT, LEVEL_CLEARED,
   MENU_MOVED, MENU_CHOSEN
};

struct Game {
   Maze maze;
   std::unique_ptr<PathFind> pathFinder;

   Actor     hero;
   Direction wanted;                 // last arrow pressed
   Monster   monsters[MONSTER_COUNT];

   Stage stage {MENU};
   Stage pausedFrom {PLAYING};       // where Esc came from
   float pause {0};                  // seconds left in the current stage

   int  menuChoice {0};              // selected option in the current menu
   bool quitRequested {false};       // "QUIT" chosen in the title menu
   int  musicVolume {8};             // 0..MAX_VOLUME, changed in the menus
   int  soundVolume {8};             // 0..MAX_VOLUME, sound effects

   // Events since the last time someone cleared this list.
   std::vector<GameEvent> events;

   int score     {0};
   int highScore {0};
   int lives     {0};
   int level     {1};

   bool  scatter {true};             // monsters heading to their corners?
   float modeTimer {0};              // until scatter <-> chase switches
   float frightenedTime {0};         // > 0 while the potion lasts
   int   eatenThisPotion {0};        // for the doubling score

   // How close the nearest dangerous monster is, from 0 (10 or more steps
   // away along the maze) to 1 (2 steps or less). 0 outside PLAYING and for
   // frightened monsters. Drives the music.
   float danger {0};

   // Loads the maze and creates the A*. false if the file does not open.
   bool prepare (const std::string &mazeFile);

   void showMenu ();                 // title menu, maze reset behind it
   void newGame ();                  // score reset, 3 lives, level 1
   void update (float dt);           // one frame
   void key (sf::Keyboard::Key k);

   // Menu shown now: empty when there is none.
   bool inMenu () const;
   std::vector<MenuAction>  menuActions () const;
   std::vector<std::string> menuOptions () const;   // labels, same order

   // Short message over the maze ("GET READY"...); empty when there is none.
   std::string banner () const;

   float potionDuration () const;

private:
   void chooseMenuOption ();
   int *volumeOf (MenuAction action);   // nullptr if not a volume option
   void startLife ();                // hero and monsters back at the start
   void playFrame (float dt);
   void moveHero (float dt);
   void moveMonster (Monster &m, float dt);
   void eatItem ();
   void checkEncounters ();
   void updateDanger ();
   void updateTints ();
   float monsterSpeed () const;
};
