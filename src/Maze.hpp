#pragma once

#include <string>
#include <vector>

#include "TileMap.hpp"
#include "Vector2D.hpp"

/**
 * The maze, read from a text file (assets/maze.txt):
 *
 *    #  wall
 *    .  corridor with a coin
 *    o  corridor with a potion
 *       (space) empty corridor
 *    H  where the hero starts
 *    P  monster pen (the hero cannot enter)
 *    -  pen door (only monsters go through)
 *
 * A row open at both ends becomes a tunnel: leaving on one side means
 * entering on the other.
 *
 * Positions are in TILES (col, row), like everywhere else in the game.
 */

// What can lie on the floor of a tile.
enum Item { NO_ITEM = 0, COIN = 1, POTION = 2 };

class Maze {
public:
   bool load (const std::string &file);

   int width  () const { return m_width; }
   int height () const { return m_height; }

   // Tunnel: column -1 is the last one, and column "width" is column 0.
   int wrapColumn (int col) const;

   bool isOpenForHero    (int col, int row) const;
   bool isOpenForMonster (int col, int row) const;
   bool isInPen          (int col, int row) const;

   int  item       (int col, int row) const;
   void removeItem (int col, int row);
   int  itemsLeft  () const { return m_itemsLeft; }
   void resetItems ();

   // One int per tile (Item), for the Screen to draw.
   const std::vector<int> &items () const { return m_items; }

   Vector2D heroStart;
   Vector2D penExit;              // tile right above the door
   std::vector<Vector2D> pen;     // tiles inside the pen

   // Walls (WALL_TILE) and corridors (EMPTY_TILE), for drawing and for A*.
   // Pen and door are corridors here: monsters must get through them.
   TileMap map;

private:
   std::vector<std::string> m_rows;
   std::vector<int> m_startItems;
   std::vector<int> m_items;
   int m_itemsLeft {0};
   int m_width  {0};
   int m_height {0};

   char at (int col, int row) const;
};
