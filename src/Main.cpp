// Dungeon: a maze-chase game.
//
// This file only connects the rules (Game) to the window (Screen). The
// rules live in Game.cpp, movement in Actor.cpp, the monsters in
// Monsters.cpp and the maze in Maze.cpp.

#include <algorithm>
#include <iostream>

#include "Game.hpp"
#include "Screen.hpp"

// Art by Kenney (CC0); see README.
#define TILES ASSETS_DIR "kenney/tiny-dungeon/"
#define UI    ASSETS_DIR "kenney/pixel-ui/"
#define FONTS ASSETS_DIR "kenney/fonts/"


// What the menu says for the current stage.
static Screen::Menu menuFor (const Game &game) {
   Screen::Menu menu;
   menu.visible  = game.inMenu();
   menu.options  = game.menuOptions();
   menu.selected = game.menuChoice;

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

   screen.onKey ([&](sf::Keyboard::Key k) {
      game.key(k);
      if (game.quitRequested)
         screen.close();
   });

   screen.onFrame ([&](float dt) {
      // A very long frame (window dragged, first frame...) would make
      // everyone jump several tiles at once.
      game.update (std::min(dt, 0.05f));

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

   if (!screen.open (900, 1000, "Dungeon"))
      return 1;

   if (!screen.loadInterface (UI, FONTS "kenpixel.ttf", FONTS "kenpixel_blocks.ttf"))
      std::cerr << "Could not load the interface art; playing without HUD and menus.\n";
   screen.setLifeIcon (TILES "knight.png");

   screen.run();
   return 0;
}
