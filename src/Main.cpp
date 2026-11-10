// Dungeon: a maze-chase game.
//
// This file only connects the rules (Game) to the window (Screen). The
// rules live in Game.cpp, movement in Actor.cpp, the monsters in
// Monsters.cpp and the maze in Maze.cpp.

#include <algorithm>
#include <iostream>

#include "Game.hpp"
#include "Screen.hpp"

// Art: Tiny Dungeon by Kenney (CC0); see README.
#define ART ASSETS_DIR "kenney/"


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
      m.actor.npc.sprite (ART + m.image);
      screen.add (&m.actor.npc);
   }
   game.hero.npc.sprite (ART "knight.png");
   screen.add (&game.hero.npc);

   // A frightened monster turns into a ghost.
   sf::Texture ghost;
   if (!ghost.loadFromFile (ART "ghost.png")) {
      std::cerr << "Could not load ghost.png\n";
      return 1;
   }

   screen.setWallImage  (ART "wall.png");
   screen.setFloorImage (ART "floor.png");
   screen.setItems      (&game.maze.items());
   screen.setItemColor  (COIN, sf::Color(255, 210, 90), 0.1f);
   screen.setItemImage  (POTION, ART "potion.png");

   screen.onKey ([&](sf::Keyboard::Key k) { game.key(k); });

   screen.onFrame ([&](float dt) {
      // A very long frame (window dragged, first frame...) would make
      // everyone jump several tiles at once.
      game.update (std::min(dt, 0.05f));

      for (auto &m: game.monsters)
         m.actor.npc.m_sprite.setTexture (m.frightened ? ghost : m.actor.npc.m_texture);

      screen.setHudText (game.hudText());

      // Bar = potion time left.
      screen.setHudBar (game.frightenedTime > 0
                        ? game.frightenedTime / game.potionDuration() : -1);
   });

   game.newGame();

   // Room above the maze for the HUD.
   screen.setTopMargin (3.5f);
   if (!screen.open (900, 1000, "Dungeon"))
      return 1;

   if (!screen.setHudFont (ASSETS_DIR "DejaVuSans-Bold.ttf"))
      std::cerr << "Could not load the font; playing without a HUD.\n";

   screen.run();
   return 0;
}
