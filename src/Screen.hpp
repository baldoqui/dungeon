#pragma once

#include <functional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "NPC.hpp"
#include "TileMap.hpp"

/**
 * The game window: draws the whole maze, the items, the characters and
 * the HUD, and calls the game every frame and on every key press.
 *
 * Units: character positions (npc.body.position) are in TILES, top-left
 * corner; each tile is TILE_SIZE x TILE_SIZE in the world, and any image
 * is stretched to fit one tile.
 *
 * The camera stays still showing the whole map, without distortion: bars
 * are left on the sides (or above and below) depending on the window shape.
 */
class Screen {
public:
   using FrameFunction = std::function<void (float dt)>;
   using KeyFunction   = std::function<void (sf::Keyboard::Key key)>;

   // The map must outlive the screen.
   explicit Screen (const TileMap &map);

   // Free space above the map, in tiles, so the HUD covers nothing.
   void setTopMargin (float tiles);

   bool open (unsigned width, unsigned height, const std::string &title);

   // Called once per frame, with the time since the previous frame.
   void onFrame (FrameFunction f) { m_frame = std::move(f); }

   // Called for every key press, in order, without missing any.
   void onKey (KeyFunction f) { m_key = std::move(f); }

   // How the maze looks.
   void setBackgroundColor (sf::Color c) { m_background = c; }
   void setWallColor       (sf::Color c) { m_wallColor = c; }
   bool setWallImage  (const std::string &file);
   bool setFloorImage (const std::string &file);

   // Items: one int per tile (same size as the map). 0 = nothing; other
   // values are drawn with their image or, without one, as a circle.
   void setItems     (const std::vector<int> *grid) { m_items = grid; }
   void setItemColor (int item, sf::Color color, float radiusInTiles);
   bool setItemImage (int item, const std::string &file);

   // Characters, drawn in the order they were added.
   void add (NPC *npc) { m_npcs.push_back(npc); }

   // HUD at the top-left corner of the window.
   bool setHudFont (const std::string &file);
   void setHudText (const std::string &utf8Text);
   // Bar under the text, from 0 to 1; negative hides it.
   void setHudBar (float fraction) { m_bar = fraction; }

   // Main loop; returns when the window closes.
   void run ();

private:
   struct ItemStyle {
      sf::Color   color {255, 255, 255};
      float       radius {0.1f};
      sf::Texture image;
      bool        hasImage {false};
   };

   const TileMap &m_map;
   sf::RenderWindow m_window;
   sf::View m_camera;       // world (maze)
   sf::View m_fixed;        // window pixels (HUD)
   float m_topMargin {0};

   FrameFunction m_frame;
   KeyFunction   m_key;

   sf::Color   m_background {20, 16, 24};
   sf::Color   m_wallColor {120, 120, 130};
   sf::Texture m_wallImage, m_floorImage;
   bool        m_hasWall {false}, m_hasFloor {false};

   const std::vector<int> *m_items {nullptr};
   std::vector<ItemStyle> m_styles;   // index = item value

   std::vector<NPC *> m_npcs;

   sf::Font m_font;
   sf::Text m_text;
   bool     m_hasFont {false};
   float    m_bar {-1};

   void fitCamera (unsigned width, unsigned height);
   ItemStyle &style (int item);

   void drawMaze ();
   void drawItems ();
   void drawCharacters ();
   void drawHud ();
};
