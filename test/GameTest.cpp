// Rule tests, without opening a window.

#include <gtest/gtest.h>

#include "Game.hpp"

namespace {

const float FRAME = 1.0f / 60;

// A new game, stopped right when "Get ready..." ends.
struct GameTest : ::testing::Test {
   Game game;

   void SetUp () override {
      ASSERT_TRUE(game.prepare(ASSETS_DIR "maze.txt"));
      game.newGame();
      for (int i = 0; i < 1000 && game.stage != PLAYING; ++i)
         game.update(FRAME);
      ASSERT_EQ(game.stage, PLAYING);
      ASSERT_EQ(game.score, 0);
   }

   void run (float seconds) {
      for (float t = 0; t < seconds; t += FRAME)
         game.update(FRAME);
   }

   // Gets the monsters out of the way: all of them parked in the pen.
   void lockMonsters () {
      for (auto &m: game.monsters) {
         m.actor.placeAt(game.maze.pen[0]);
         m.state = IN_PEN;
         m.wait = 1000;
      }
   }
};


// --- Maze ------------------------------------------------------------------

TEST_F(GameTest, MazeHasSizeAndPenFromFile) {
   EXPECT_EQ(game.maze.width(), 21);
   EXPECT_EQ(game.maze.height(), 23);
   EXPECT_EQ(game.maze.map.width(), 21);
   EXPECT_EQ(game.maze.pen.size(), 5u);
}

// With no open 2x2 block, A* never needs a diagonal step.
TEST_F(GameTest, MazeHasNoOpen2x2Block) {
   const Maze &maze = game.maze;
   for (int r = 0; r + 1 < maze.height(); ++r)
      for (int c = 0; c + 1 < maze.width(); ++c)
         EXPECT_FALSE(maze.isOpenForMonster(c, r) && maze.isOpenForMonster(c + 1, r) &&
                      maze.isOpenForMonster(c, r + 1) && maze.isOpenForMonster(c + 1, r + 1))
            << "open block at " << c << "," << r;
}

TEST_F(GameTest, HeroCannotEnterPenButMonstersCan) {
   Vector2D door = game.maze.penExit + Vector2D(0, 1);
   EXPECT_FALSE(game.maze.isOpenForHero((int) door.x, (int) door.y));
   EXPECT_TRUE(game.maze.isOpenForMonster((int) door.x, (int) door.y));
}


// --- Hero ------------------------------------------------------------------

TEST_F(GameTest, HeroEatsCoinsWhileMoving) {
   lockMonsters();
   int before = game.maze.itemsLeft();
   run(1.0f);   // starts heading left, along a corridor of coins
   int eaten = before - game.maze.itemsLeft();
   EXPECT_GE(eaten, 3);
   EXPECT_EQ(game.score, eaten * 10);
}

TEST_F(GameTest, HeroStopsAtWall) {
   lockMonsters();
   game.key(sf::Keyboard::Left);
   run(5.0f);
   // On the starting row, the wall to the left is column 5.
   EXPECT_TRUE(game.hero.atTileCenter());
   EXPECT_EQ(game.hero.col(), 6);
   EXPECT_TRUE(game.hero.dir.isStop());
}

TEST_F(GameTest, HeroTurnsBackRightAway) {
   lockMonsters();
   run(0.1f);
   ASSERT_EQ(game.hero.dir, LEFT);
   game.key(sf::Keyboard::Right);
   game.update(FRAME);
   EXPECT_EQ(game.hero.dir, RIGHT);
}

TEST_F(GameTest, TunnelLeadsToTheOtherSide) {
   lockMonsters();
   game.hero.placeAt(Vector2D(1, 11));
   game.key(sf::Keyboard::Left);
   run(0.6f);
   EXPECT_GT(game.hero.npc.body.position.x, game.maze.width() - 3);
}


// --- Potion and monsters ---------------------------------------------------

TEST_F(GameTest, PotionFrightensMonstersAndPointsDouble) {
   lockMonsters();
   game.hero.placeAt(Vector2D(1, 1));   // on a potion
   game.update(FRAME);
   EXPECT_GT(game.frightenedTime, 0);
   for (auto &m: game.monsters)
      EXPECT_TRUE(m.frightened);

   int before = game.score;
   for (int i = 0; i < 2; ++i) {
      Monster &m = game.monsters[i];
      m.state = ROAMING;
      m.actor.placeAt(game.hero.npc.body.position);
      game.update(FRAME);
      EXPECT_EQ(m.state, IN_PEN);
   }
   EXPECT_EQ(game.score - before, 200 + 400);
}

TEST_F(GameTest, MonsterCatchesHeroAndTakesALife) {
   Monster &brute = game.monsters[BRUTE];
   brute.actor.placeAt(game.hero.npc.body.position);
   game.update(FRAME);
   EXPECT_EQ(game.stage, CAUGHT);
   EXPECT_EQ(game.lives, 2);
   run(2.0f);
   EXPECT_EQ(game.stage, READY);
}

TEST_F(GameTest, NoLivesEndsTheGameAndPlayAgainRestarts) {
   game.lives = 1;
   game.score = 500;
   game.monsters[BRUTE].actor.placeAt(game.hero.npc.body.position);
   game.update(FRAME);
   run(2.0f);
   EXPECT_EQ(game.stage, GAME_OVER);
   EXPECT_TRUE(game.inMenu());
   EXPECT_EQ(game.menuOptions()[0], "PLAY AGAIN");

   game.key(sf::Keyboard::Enter);
   EXPECT_EQ(game.lives, 3);
   EXPECT_EQ(game.score, 0);
   EXPECT_EQ(game.highScore, 500);
   EXPECT_EQ(game.stage, READY);
}


// --- Danger (drives the music) ---------------------------------------------

TEST_F(GameTest, DangerFollowsTheNearestMonster) {
   lockMonsters();
   Monster &brute = game.monsters[BRUTE];
   brute.state = ROAMING;
   brute.wait = 0;

   // Far away, in the opposite corner.
   game.hero.placeAt(Vector2D(1, 21));
   brute.actor.placeAt(Vector2D(19, 1));
   game.wanted = STOP;
   game.update(FRAME);
   EXPECT_EQ(game.danger, 0);

   // Two steps away along the corridor.
   brute.actor.placeAt(Vector2D(3, 21));
   game.update(FRAME);
   EXPECT_GT(game.danger, 0.9f);

   // Frightened monsters are not a danger.
   game.hero.placeAt(Vector2D(1, 1));   // on a potion
   brute.actor.placeAt(Vector2D(3, 1));
   game.update(FRAME);
   EXPECT_TRUE(brute.frightened);
   EXPECT_EQ(game.danger, 0);
}

TEST_F(GameTest, NoDangerOutsidePlaying) {
   Monster &brute = game.monsters[BRUTE];
   brute.actor.placeAt(game.hero.npc.body.position + Vector2D(2, 0));
   game.update(FRAME);
   game.key(sf::Keyboard::Escape);
   game.update(FRAME);
   EXPECT_EQ(game.stage, PAUSED);
   EXPECT_EQ(game.danger, 0);
}


// --- Menus -----------------------------------------------------------------

TEST_F(GameTest, TitleMenuPlaysOrQuits) {
   game.showMenu();
   EXPECT_EQ(game.stage, MENU);
   ASSERT_EQ(game.menuOptions().size(), 3u);
   run(1.0f);   // nothing moves behind the menu
   EXPECT_EQ(game.stage, MENU);

   game.key(sf::Keyboard::Up);     // wraps around to QUIT
   game.key(sf::Keyboard::Enter);
   EXPECT_TRUE(game.quitRequested);

   // After a choice the selection goes back to the first option, PLAY.
   game.quitRequested = false;
   EXPECT_EQ(game.menuChoice, 0);
   game.key(sf::Keyboard::Enter);
   EXPECT_EQ(game.stage, READY);
   EXPECT_FALSE(game.quitRequested);
}

TEST_F(GameTest, MusicVolumeChangesInTheMenu) {
   game.showMenu();
   game.musicVolume = 8;
   game.key(sf::Keyboard::Down);   // MUSIC
   EXPECT_EQ(game.menuActions()[game.menuChoice], CHANGE_VOLUME);
   EXPECT_EQ(game.menuOptions()[game.menuChoice], "MUSIC 80%");

   game.key(sf::Keyboard::Left);
   EXPECT_EQ(game.musicVolume, 7);
   for (int i = 0; i < 20; ++i) game.key(sf::Keyboard::Right);
   EXPECT_EQ(game.musicVolume, 10);   // stops at 100%

   // Enter steps it up and wraps around, without leaving the option.
   game.key(sf::Keyboard::Enter);
   EXPECT_EQ(game.musicVolume, 0);
   EXPECT_EQ(game.menuActions()[game.menuChoice], CHANGE_VOLUME);
   EXPECT_EQ(game.stage, MENU);

   // Left/Right do nothing on other options.
   game.key(sf::Keyboard::Up);   // PLAY
   game.key(sf::Keyboard::Right);
   EXPECT_EQ(game.musicVolume, 0);
}

TEST_F(GameTest, EscPausesAndResumes) {
   lockMonsters();
   run(0.2f);
   Vector2D before = game.hero.npc.body.position;

   game.key(sf::Keyboard::Escape);
   EXPECT_EQ(game.stage, PAUSED);
   run(1.0f);
   EXPECT_EQ(game.hero.npc.body.position.x, before.x);

   // Arrows choose menu options while paused, not the hero's direction.
   game.key(sf::Keyboard::Down);
   EXPECT_EQ(game.menuChoice, 1);
   game.key(sf::Keyboard::Up);
   game.key(sf::Keyboard::Enter);
   EXPECT_EQ(game.stage, PLAYING);
   EXPECT_EQ(game.hero.dir, LEFT);
}

TEST_F(GameTest, PauseMenuCanGoBackToTitle) {
   game.score = 120;
   game.key(sf::Keyboard::Escape);
   EXPECT_EQ(game.menuOptions()[2], "MAIN MENU");
   game.key(sf::Keyboard::Up);   // wraps around to MAIN MENU
   game.key(sf::Keyboard::Enter);
   EXPECT_EQ(game.stage, MENU);
   EXPECT_EQ(game.highScore, 0);   // high score is only updated while playing
}

TEST_F(GameTest, MonstersLeaveThePen) {
   lockMonsters();
   for (auto &m: game.monsters)
      m.wait = 0;
   game.hero.placeAt(Vector2D(1, 21));   // far away, in a corner
   run(4.0f);
   for (auto &m: game.monsters)
      EXPECT_EQ(m.state, ROAMING) << m.name;
}

// Long game with the hero standing still: the monsters find him, and
// nobody walks into a wall or back into the pen after leaving it.
TEST_F(GameTest, MonstersReachTheHeroWithoutCrossingWalls) {
   game.hero.placeAt(Vector2D(1, 21));
   bool caught = false;
   for (float t = 0; t < 60 && !caught; t += FRAME) {
      game.wanted = STOP;
      game.update(FRAME);
      caught = game.stage == CAUGHT;
      for (auto &m: game.monsters) {
         ASSERT_TRUE(game.maze.isOpenForMonster(m.actor.col(), m.actor.row())) << m.name;
         if (m.state == ROAMING)
            ASSERT_FALSE(game.maze.isInPen(m.actor.col(), m.actor.row())) << m.name;
      }
   }
   EXPECT_TRUE(caught);
}

TEST_F(GameTest, EatingEverythingClearsTheLevel) {
   lockMonsters();
   // Leave a single coin: the one just left of the start.
   Maze &maze = game.maze;
   int col = (int) maze.heroStart.x - 1, row = (int) maze.heroStart.y;
   ASSERT_EQ(maze.item(col, row), COIN);
   for (int r = 0; r < maze.height(); ++r)
      for (int c = 0; c < maze.width(); ++c)
         if (c != col || r != row)
            maze.removeItem(c, r);
   ASSERT_EQ(maze.itemsLeft(), 1);

   run(0.5f);
   EXPECT_EQ(game.stage, LEVEL_CLEAR);
   run(2.5f);
   EXPECT_EQ(game.level, 2);
   EXPECT_GT(maze.itemsLeft(), 100);
}

}  // namespace
