#pragma once

#include <memory>
#include <string>

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
 */

enum Stage {
   READY,         // pause before a life starts
   PLAYING,
   CAUGHT,        // a monster got the hero
   LEVEL_CLEAR,   // every item eaten
   GAME_OVER,     // no lives left; R restarts
};

const int MONSTER_COUNT = 4;

struct Game {
   Maze maze;
   std::unique_ptr<PathFind> pathFinder;

   Actor     hero;
   Direction wanted;                 // last arrow pressed
   Monster   monsters[MONSTER_COUNT];

   Stage stage {READY};
   float pause {0};                  // seconds left in the current stage

   int score     {0};
   int highScore {0};
   int lives     {0};
   int level     {1};

   bool  scatter {true};             // monsters heading to their corners?
   float modeTimer {0};              // until scatter <-> chase switches
   float frightenedTime {0};         // > 0 while the potion lasts
   int   eatenThisPotion {0};        // for the doubling score

   // Loads the maze and creates the A*. false if the file does not open.
   bool prepare (const std::string &mazeFile);

   void newGame ();                  // score reset, 3 lives, level 1
   void update (float dt);           // one frame
   void key (sf::Keyboard::Key k);

   std::string hudText () const;
   float potionDuration () const;

private:
   void startLife ();                // hero and monsters back at the start
   void playFrame (float dt);
   void moveHero (float dt);
   void moveMonster (Monster &m, float dt);
   void eatItem ();
   void checkEncounters ();
   void updateTints ();
   float monsterSpeed () const;
};
