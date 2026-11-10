#include "Game.hpp"

#include <algorithm>

#include "Collision2D.hpp"
#include "Global.hpp"


//--------------------------------------------------------------
// Rule numbers

const int   STARTING_LIVES        = 3;
const int   COIN_POINTS           = 10;
const int   POTION_POINTS         = 50;
const int   FIRST_MONSTER_POINTS  = 200;

const float HERO_SPEED            = 4.0f;   // tiles per second
const float MONSTER_SPEED         = 3.6f;   // on level 1
const float MAX_MONSTER_SPEED     = 4.0f;
const float FRIGHTENED_SPEED      = 2.0f;
const float LEAVING_PEN_SPEED     = 2.0f;

const float SCATTER_TIME          = 7.0f;   // seconds
const float CHASE_TIME            = 20.0f;
const float READY_PAUSE           = 2.0f;
const float CAUGHT_PAUSE          = 1.5f;
const float LEVEL_CLEAR_PAUSE     = 2.0f;
const float WAIT_AFTER_EATEN      = 3.0f;

// Collision box margin: hero and monster only touch when their centres
// are less than half a tile apart on both axes.
const float MARGIN = 0.25f;


//--------------------------------------------------------------
// Setup

bool Game::prepare (const std::string &mazeFile) {

   if (!maze.load(mazeFile))
      return false;

   pathFinder = std::make_unique<PathFind>(maze.map.data(), maze.map.width(),
                                           maze.map.height());

   // Corners are one tile in from the edges.
   const float right  = maze.width()  - 2;
   const float bottom = maze.height() - 2;

   monsters[BRUTE].personality    = BRUTE;
   monsters[BRUTE].name           = "Brute";
   monsters[BRUTE].image          = "demon.png";
   monsters[BRUTE].corner         = Vector2D(right, 1);

   monsters[AMBUSHER].personality = AMBUSHER;
   monsters[AMBUSHER].name        = "Ambusher";
   monsters[AMBUSHER].image       = "bat.png";
   monsters[AMBUSHER].corner      = Vector2D(1, 1);

   monsters[FLANKER].personality  = FLANKER;
   monsters[FLANKER].name         = "Flanker";
   monsters[FLANKER].image        = "spider.png";
   monsters[FLANKER].corner       = Vector2D(right, bottom);

   monsters[COWARD].personality   = COWARD;
   monsters[COWARD].name          = "Coward";
   monsters[COWARD].image         = "slime.png";
   monsters[COWARD].corner        = Vector2D(1, bottom);

   return true;
}

void Game::showMenu () {
   // A fresh maze behind the menu, with everyone in place.
   maze.resetItems();
   lives = STARTING_LIVES;
   startLife();
   stage = MENU;
   menuChoice = 0;
}

void Game::newGame () {
   score = 0;
   lives = STARTING_LIVES;
   level = 1;
   menuChoice = 0;
   maze.resetItems();
   startLife();
}

void Game::startLife () {

   hero.placeAt(maze.heroStart);
   hero.speed = HERO_SPEED;
   wanted = LEFT;

   // The Brute starts outside; the others leave the pen one at a time.
   const float waits[MONSTER_COUNT] = {0, 2, 5, 8};

   for (int i = 0; i < MONSTER_COUNT; ++i) {
      Monster &m = monsters[i];
      m.frightened = false;
      m.wait = waits[i] / (1 + 0.2f * (level - 1));

      if (i == BRUTE) {
         m.actor.placeAt(maze.penExit);
         m.state = ROAMING;
      } else {
         // Middle tiles of the pen.
         size_t middle = maze.pen.size() / 2;
         size_t tile = std::min(maze.pen.size() - 1, middle - 1 + (i - 1));
         m.actor.placeAt(maze.pen[tile]);
         m.state = IN_PEN;
      }
   }

   scatter = true;
   modeTimer = SCATTER_TIME;
   frightenedTime = 0;
   eatenThisPotion = 0;

   stage = READY;
   pause = READY_PAUSE;
   updateTints();
}


//--------------------------------------------------------------
// Keyboard

// WASD and Vim keys (H J K L) work as the arrows.
static sf::Keyboard::Key asArrow (sf::Keyboard::Key k) {
   switch (k) {
      case sf::Keyboard::W: case sf::Keyboard::K: return sf::Keyboard::Up;
      case sf::Keyboard::S: case sf::Keyboard::J: return sf::Keyboard::Down;
      case sf::Keyboard::A: case sf::Keyboard::H: return sf::Keyboard::Left;
      case sf::Keyboard::D: case sf::Keyboard::L: return sf::Keyboard::Right;
      default: return k;
   }
}

void Game::key (sf::Keyboard::Key k) {

   k = asArrow(k);

   if (inMenu()) {
      int count = (int) menuActions().size();
      bool onVolume = menuActions()[menuChoice] == CHANGE_VOLUME;
      switch (k) {
         case sf::Keyboard::Up:    menuChoice = (menuChoice + count - 1) % count; break;
         case sf::Keyboard::Down:  menuChoice = (menuChoice + 1) % count; break;
         case sf::Keyboard::Left:
            if (onVolume) musicVolume = std::max(0, musicVolume - 1);
            break;
         case sf::Keyboard::Right:
            if (onVolume) musicVolume = std::min(MAX_VOLUME, musicVolume + 1);
            break;
         case sf::Keyboard::Enter:
         case sf::Keyboard::Space: chooseMenuOption(); break;
         case sf::Keyboard::Escape:
            if (stage == PAUSED)
               stage = pausedFrom;
            break;
         default: break;
      }
      return;
   }

   switch (k) {
      case sf::Keyboard::Left:  wanted = LEFT;  break;
      case sf::Keyboard::Right: wanted = RIGHT; break;
      case sf::Keyboard::Up:    wanted = UP;    break;
      case sf::Keyboard::Down:  wanted = DOWN;  break;
      case sf::Keyboard::Escape:
         pausedFrom = stage;
         stage = PAUSED;
         menuChoice = 0;
         break;
      default:
         break;
   }
}

bool Game::inMenu () const {
   return stage == MENU || stage == PAUSED || stage == GAME_OVER;
}

std::vector<MenuAction> Game::menuActions () const {
   switch (stage) {
      case MENU:      return {START_GAME, CHANGE_VOLUME, QUIT_GAME};
      case PAUSED:    return {RESUME_GAME, CHANGE_VOLUME, BACK_TO_TITLE};
      case GAME_OVER: return {START_GAME, BACK_TO_TITLE};
      default:        return {};
   }
}

std::vector<std::string> Game::menuOptions () const {
   std::vector<std::string> labels;
   for (MenuAction a: menuActions())
      switch (a) {
         case START_GAME:    labels.push_back(stage == GAME_OVER ? "PLAY AGAIN" : "PLAY"); break;
         case RESUME_GAME:   labels.push_back("RESUME"); break;
         case CHANGE_VOLUME: labels.push_back("MUSIC " + std::to_string(musicVolume * 10) + "%"); break;
         case BACK_TO_TITLE: labels.push_back("MAIN MENU"); break;
         case QUIT_GAME:     labels.push_back("QUIT"); break;
      }
   return labels;
}

void Game::chooseMenuOption () {

   MenuAction action = menuActions()[menuChoice];

   // Enter on the volume steps it up (and around), keeping the selection.
   if (action == CHANGE_VOLUME) {
      musicVolume = (musicVolume + 1) % (MAX_VOLUME + 1);
      return;
   }

   menuChoice = 0;
   switch (action) {
      case START_GAME:    newGame(); break;
      case RESUME_GAME:   stage = pausedFrom; break;
      case BACK_TO_TITLE: showMenu(); break;
      case QUIT_GAME:     quitRequested = true; break;
      case CHANGE_VOLUME: break;
   }
}


//--------------------------------------------------------------
// One frame

void Game::update (float dt) {

   if (stage != PLAYING)
      danger = 0;

   switch (stage) {
      case MENU:
      case PAUSED:
         break;

      case READY:
         pause -= dt;
         if (pause <= 0)
            stage = PLAYING;
         break;

      case PLAYING:
         playFrame(dt);
         break;

      case CAUGHT:
         pause -= dt;
         if (pause <= 0) {
            if (lives <= 0) {
               stage = GAME_OVER;
               menuChoice = 0;
            } else {
               startLife();
            }
         }
         break;

      case LEVEL_CLEAR:
         pause -= dt;
         if (pause <= 0) {
            ++level;
            maze.resetItems();
            startLife();
         }
         break;

      case GAME_OVER:
         break;
   }

   highScore = std::max(highScore, score);
   updateTints();
}

void Game::playFrame (float dt) {

   // Scatter and chase alternate; the clock stops while a potion lasts.
   if (frightenedTime > 0) {
      frightenedTime -= dt;
      if (frightenedTime <= 0)
         for (auto &m: monsters)
            m.frightened = false;
   } else {
      modeTimer -= dt;
      if (modeTimer <= 0) {
         scatter = !scatter;
         modeTimer = scatter ? SCATTER_TIME : CHASE_TIME;
      }
   }

   moveHero(dt);
   eatItem();

   if (maze.itemsLeft() == 0) {
      stage = LEVEL_CLEAR;
      pause = LEVEL_CLEAR_PAUSE;
      return;
   }

   for (auto &m: monsters)
      moveMonster(m, dt);

   checkEncounters();
   updateDanger();
}

void Game::updateDanger () {

   auto centre = [](int col, int row) {
      return Vector2D((col + 0.5f) * TILE_SIZE, (row + 0.5f) * TILE_SIZE);
   };

   // Steps along the maze (A*), not a straight line: a monster behind a
   // wall is not close.
   int nearest = 1000;
   for (auto &m: monsters) {
      if (m.state != ROAMING || m.frightened)
         continue;
      auto path = pathFinder->findPath(centre(hero.col(), hero.row()),
                                       centre(m.actor.col(), m.actor.row()));
      if (!path.empty())
         nearest = std::min(nearest, (int) path.size() - 1);
   }

   const float closeSteps = 2, farSteps = 10;
   danger = std::clamp((farSteps - nearest) / (farSteps - closeSteps), 0.0f, 1.0f);
}


//--------------------------------------------------------------
// Hero

void Game::moveHero (float dt) {

   IsOpen isOpen = [this](int col, int row) { return maze.isOpenForHero(col, row); };

   // Turning back works right away, even between two tiles.
   if (!hero.dir.isStop() && wanted == hero.dir.opposite())
      hero.dir = wanted;

   // At a tile centre: turn where the arrow says, if possible;
   // otherwise keep going (and stop at a wall).
   ChooseDirection choose = [&](const Actor &a) {
      if (isOpen(a.col() + wanted.dx, a.row() + wanted.dy))
         return wanted;
      return a.dir;
   };

   move(hero, dt, maze, isOpen, choose);
}

void Game::eatItem () {

   int col = hero.col(), row = hero.row();
   int item = maze.item(col, row);

   if (item == NO_ITEM)
      return;

   maze.removeItem(col, row);

   if (item == COIN)
      score += COIN_POINTS;

   if (item == POTION) {
      score += POTION_POINTS;
      frightenedTime = potionDuration();
      eatenThisPotion = 0;
      for (auto &m: monsters) {
         m.frightened = true;
         // Monsters out in the maze turn back when they get frightened.
         if (m.state == ROAMING && !m.actor.dir.isStop())
            m.actor.dir = m.actor.dir.opposite();
      }
   }
}


//--------------------------------------------------------------
// Monsters

float Game::monsterSpeed () const {
   return std::min(MAX_MONSTER_SPEED, MONSTER_SPEED * (1 + 0.05f * (level - 1)));
}

float Game::potionDuration () const {
   // The potion lasts less on each level, down to 2 seconds.
   return std::max(2.0f, 7.0f - (level - 1));
}

void Game::moveMonster (Monster &m, float dt) {

   IsOpen openInMaze = [this](int col, int row) {
      return maze.isOpenForHero(col, row);
   };
   IsOpen openWithPen = [this](int col, int row) {
      return maze.isOpenForMonster(col, row);
   };

   switch (m.state) {

      case IN_PEN:
         m.wait -= dt;
         if (m.wait <= 0)
            m.state = LEAVING_PEN;
         return;

      case LEAVING_PEN: {
         m.actor.speed = LEAVING_PEN_SPEED;
         ChooseDirection choose = [&](const Actor &a) {
            if (a.col() == (int) maze.penExit.x && a.row() == (int) maze.penExit.y)
               return STOP;
            return directionTowards(a, maze.penExit, *pathFinder, openWithPen);
         };
         move(m.actor, dt, maze, openWithPen, choose);

         if (m.actor.atTileCenter() &&
             m.actor.col() == (int) maze.penExit.x &&
             m.actor.row() == (int) maze.penExit.y)
            m.state = ROAMING;
         return;
      }

      case ROAMING: {
         m.actor.speed = m.frightened ? FRIGHTENED_SPEED : monsterSpeed();

         Situation s;
         s.hero    = Vector2D(hero.col(), hero.row());
         s.heroDir = hero.dir.isStop() ? wanted : hero.dir;
         s.brute   = monsters[BRUTE].actor.npc.body.position;
         s.scatter = scatter;

         ChooseDirection choose = [&](const Actor &a) {
            if (m.frightened)
               return fleeDirection(a, s.hero, openInMaze);
            Vector2D target = validTarget(targetOf(m, s), maze);
            return directionTowards(a, target, *pathFinder, openInMaze);
         };
         move(m.actor, dt, maze, openInMaze, choose);
         return;
      }
   }
}

void Game::checkEncounters () {

   auto box = [](const Actor &a) {
      const Vector2D &p = a.npc.body.position;
      return AABB{ p + Vector2D(MARGIN, MARGIN), p + Vector2D(1 - MARGIN, 1 - MARGIN) };
   };

   for (auto &m: monsters) {
      if (m.state == IN_PEN)
         continue;
      if (!box(hero).intersects(box(m.actor)))
         continue;

      if (m.frightened) {
         // Ate the monster: points double each time, and it goes back to the pen.
         score += FIRST_MONSTER_POINTS << eatenThisPotion;
         ++eatenThisPotion;
         m.frightened = false;
         m.actor.placeAt(maze.pen[maze.pen.size() / 2]);
         m.state = IN_PEN;
         m.wait = WAIT_AFTER_EATEN;
      } else {
         --lives;
         stage = CAUGHT;
         pause = CAUGHT_PAUSE;
         return;
      }
   }
}


//--------------------------------------------------------------
// Looks and HUD

void Game::updateTints () {
   // Frightened monsters (drawn as a ghost by Main.cpp) are tinted blue,
   // and flash white during the last 2 seconds.
   bool flashing = frightenedTime < 2.0f && (int) (frightenedTime * 6) % 2 == 0;

   for (auto &m: monsters) {
      if (m.frightened && !flashing)
         m.actor.npc.m_sprite.setColor(sf::Color(130, 150, 255));
      else
         m.actor.npc.m_sprite.setColor(sf::Color::White);
   }
}

std::string Game::banner () const {
   switch (stage) {
      case READY:       return level == 1 ? "GET READY" : "LEVEL " + std::to_string(level);
      case CAUGHT:      return "CAUGHT!";
      case LEVEL_CLEAR: return "LEVEL CLEAR!";
      default:          return "";
   }
}
