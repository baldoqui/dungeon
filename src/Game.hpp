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
 * Menus: the title menu (PLAY / QUIT), Esc during a game (RESUME /
 * MAIN MENU) and game over (PLAY AGAIN / MAIN MENU). Up and Down pick an
 * option, Enter or Space confirms it.
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
   std::vector<std::string> menuOptions () const;

   // Short message over the maze ("GET READY"...); empty when there is none.
   std::string banner () const;

   float potionDuration () const;

private:
   void chooseMenuOption ();
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
