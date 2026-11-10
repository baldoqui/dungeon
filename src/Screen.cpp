#include "Screen.hpp"

#include <algorithm>
#include <cmath>

#include "Global.hpp"


// Height of the HUD strip at the top of the window, in pixels.
static const float HUD_HEIGHT = 112;

static const sf::Color TEXT_COLOR    (240, 228, 200);
static const sf::Color LABEL_COLOR   (215, 190, 150);
static const sf::Color SHADOW_COLOR  (40, 26, 20, 200);
static const sf::Color DARK_TEXT     (70, 46, 30);


// Stretches a sprite to cover exactly one tile.
static void fitToTile (sf::Sprite &sprite) {
   const sf::Texture *t = sprite.getTexture();
   if (t == nullptr || t->getSize().x == 0 || t->getSize().y == 0)
      return;
   sprite.setScale(TILE_SIZE / t->getSize().x, TILE_SIZE / t->getSize().y);
}

// Pixel fonts look sharp only without smoothing (an SFML 2.6 option).
static void makeSharp (sf::Font &font) {
#if SFML_VERSION_MAJOR == 2 && SFML_VERSION_MINOR >= 6
   font.setSmooth(false);
#else
   (void) font;
#endif
}


Screen::Screen (const TileMap &map) : m_map(map) {}

bool Screen::open (unsigned width, unsigned height, const std::string &title) {
   m_window.create(sf::VideoMode(width, height), title);
   if (!m_window.isOpen())
      return false;

   m_window.setFramerateLimit(60);
   fitCamera(width, height);
   return true;
}

void Screen::fitCamera (unsigned width, unsigned height) {

   m_fixed = sf::View(sf::FloatRect(0, 0, (float) width, (float) height));

   const float worldW = m_map.width()  * TILE_SIZE;
   const float worldH = m_map.height() * TILE_SIZE;
   m_camera = sf::View(sf::FloatRect(0, 0, worldW, worldH));

   // The maze gets the area under the HUD, keeping its proportions.
   const float top   = std::min(HUD_HEIGHT, height * 0.5f) / height;
   const float areaW = (float) width;
   const float areaH = height * (1 - top);

   const float scale = std::min(areaW / worldW, areaH / worldH);
   const float w = worldW * scale / width;
   const float h = worldH * scale / height;

   m_camera.setViewport(sf::FloatRect((1 - w) / 2, top + ((1 - top) - h) / 2, w, h));
}


//--------------------------------------------------------------
// Looks

bool Screen::setWallImage (const std::string &file) {
   m_hasWall = m_wallImage.loadFromFile(file);
   return m_hasWall;
}

bool Screen::setFloorImage (const std::string &file) {
   m_hasFloor = m_floorImage.loadFromFile(file);
   return m_hasFloor;
}

Screen::ItemStyle &Screen::style (int item) {
   if ((int) m_styles.size() <= item)
      m_styles.resize(item + 1);
   return m_styles[item];
}

void Screen::setItemColor (int item, sf::Color color, float radiusInTiles) {
   style(item).color  = color;
   style(item).radius = radiusInTiles;
}

bool Screen::setItemImage (int item, const std::string &file) {
   ItemStyle &s = style(item);
   s.hasImage = s.image.loadFromFile(file);
   return s.hasImage;
}

bool Screen::loadInterface (const std::string &folder, const std::string &textFont,
                            const std::string &titleFont) {
   m_hasInterface =
      m_panel.loadFromFile(folder + "panel.png") &&
      m_button.loadFromFile(folder + "button.png") &&
      m_buttonSelected.loadFromFile(folder + "button_selected.png") &&
      m_barBack.loadFromFile(folder + "bar_back.png") &&
      m_barGreen.loadFromFile(folder + "bar_green.png") &&
      m_barYellow.loadFromFile(folder + "bar_yellow.png") &&
      m_barRed.loadFromFile(folder + "bar_red.png") &&
      m_font.loadFromFile(textFont) &&
      m_titleFont.loadFromFile(titleFont);

   makeSharp(m_font);
   makeSharp(m_titleFont);
   return m_hasInterface;
}

bool Screen::setLifeIcon (const std::string &file) {
   return m_lifeIcon.loadFromFile(file);
}


//--------------------------------------------------------------
// Main loop

void Screen::run () {

   sf::Clock clock;

   while (m_window.isOpen()) {

      sf::Event event;
      while (m_window.pollEvent(event)) {
         if (event.type == sf::Event::Closed)
            m_window.close();
         else if (event.type == sf::Event::Resized)
            fitCamera(event.size.width, event.size.height);
         else if (event.type == sf::Event::KeyPressed && m_key)
            m_key(event.key.code);
      }

      float dt = clock.restart().asSeconds();
      if (m_frame)
         m_frame(dt);

      if (!m_window.isOpen())
         break;

      m_window.clear(m_background);

      m_window.setView(m_camera);
      drawMaze();
      drawItems();
      drawCharacters();

      m_window.setView(m_fixed);
      if (m_hasInterface) {
         drawHud();
         drawBanner();
         drawMenu();
      }

      m_window.display();
   }
}


//--------------------------------------------------------------
// Maze and characters

void Screen::drawMaze () {

   sf::Sprite wall(m_wallImage), floor(m_floorImage);
   fitToTile(wall);
   fitToTile(floor);

   sf::RectangleShape block(sf::Vector2f(TILE_SIZE, TILE_SIZE));
   block.setFillColor(sf::Color(120, 120, 130));

   for (int row = 0; row < m_map.height(); ++row)
      for (int col = 0; col < m_map.width(); ++col) {
         sf::Vector2f where(col * TILE_SIZE, row * TILE_SIZE);

         if (m_map.isWall(col, row)) {
            if (m_hasWall) { wall.setPosition(where);  m_window.draw(wall);  }
            else           { block.setPosition(where); m_window.draw(block); }
         } else if (m_hasFloor) {
            floor.setPosition(where);
            m_window.draw(floor);
         }
      }
}

void Screen::drawItems () {

   if (m_items == nullptr)
      return;

   const int width = m_map.width();

   for (int row = 0; row < m_map.height(); ++row)
      for (int col = 0; col < width; ++col) {
         size_t i = (size_t) row * width + col;
         if (i >= m_items->size())
            return;

         int item = (*m_items)[i];
         if (item <= 0 || item >= (int) m_styles.size())
            continue;

         const ItemStyle &s = m_styles[item];

         if (s.hasImage) {
            sf::Sprite sprite(s.image);
            fitToTile(sprite);
            sprite.setPosition(col * TILE_SIZE, row * TILE_SIZE);
            m_window.draw(sprite);
         } else {
            float r = s.radius * TILE_SIZE;
            sf::CircleShape c(r);
            c.setOrigin(r, r);
            c.setFillColor(s.color);
            c.setPosition((col + 0.5f) * TILE_SIZE, (row + 0.5f) * TILE_SIZE);
            m_window.draw(c);
         }
      }
}

void Screen::drawCharacters () {
   for (NPC *npc: m_npcs) {
      if (npc == nullptr || !npc->active)
         continue;
      fitToTile(npc->m_sprite);
      npc->m_sprite.setPosition(npc->body.position.x * TILE_SIZE,
                                npc->body.position.y * TILE_SIZE);
      m_window.draw(npc->m_sprite);
   }
}


//--------------------------------------------------------------
// Interface

void Screen::drawPanel (const sf::Texture &texture, const sf::FloatRect &where,
                        float border, float scale) {

   const float sw = (float) texture.getSize().x;
   const float sh = (float) texture.getSize().y;
   const float d  = border * scale;

   // Column and row edges, in the image and on the screen.
   const float srcX[4] = {0, border, sw - border, sw};
   const float srcY[4] = {0, border, sh - border, sh};
   const float dstX[4] = {where.left, where.left + d,
                          where.left + where.width - d, where.left + where.width};
   const float dstY[4] = {where.top, where.top + d,
                          where.top + where.height - d, where.top + where.height};

   sf::Sprite piece(texture);
   for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) {
         float srcW = srcX[i + 1] - srcX[i], srcH = srcY[j + 1] - srcY[j];
         float dstW = dstX[i + 1] - dstX[i], dstH = dstY[j + 1] - dstY[j];
         if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
            continue;
         piece.setTextureRect(sf::IntRect((int) srcX[i], (int) srcY[j], (int) srcW, (int) srcH));
         piece.setPosition(dstX[i], dstY[j]);
         piece.setScale(dstW / srcW, dstH / srcH);
         m_window.draw(piece);
      }
}

void Screen::drawText (const sf::Font &font, const std::string &text, unsigned size,
                       sf::Vector2f where, Align align, sf::Color color) {

   sf::Text t(sf::String::fromUtf8(text.begin(), text.end()), font, size);
   sf::FloatRect b = t.getLocalBounds();

   float x = where.x - b.left;
   if (align == CENTER_ALIGN) x -= b.width / 2;
   if (align == RIGHT_ALIGN)  x -= b.width;

   // Whole pixels keep the pixel font sharp.
   sf::Vector2f pos(std::round(x), std::round(where.y));

   t.setFillColor(SHADOW_COLOR);
   t.setPosition(pos + sf::Vector2f(2, 2));
   m_window.draw(t);

   t.setFillColor(color);
   t.setPosition(pos);
   m_window.draw(t);
}

void Screen::drawHud () {

   const float windowW = (float) m_window.getSize().x;
   const float h = std::min(HUD_HEIGHT, m_window.getSize().y * 0.5f);

   // Same width as the maze (but never too narrow for the four columns).
   const sf::FloatRect vp = m_camera.getViewport();
   const float w    = std::min(windowW - 16, std::max(vp.width * windowW, 560.0f));
   const float left = (windowW - w) / 2;

   drawPanel(m_panel, sf::FloatRect(left, 8, w, h - 16), 8, 2);

   // Four columns: score, high score, level, lives.
   const float column = w / 4;
   auto centre = [&](int i) { return left + column * (i + 0.5f); };

   drawText(m_font, "SCORE",      16, {centre(0), 20}, CENTER_ALIGN, LABEL_COLOR);
   drawText(m_font, "HIGH SCORE", 16, {centre(1), 20}, CENTER_ALIGN, LABEL_COLOR);
   drawText(m_font, "LEVEL",      16, {centre(2), 20}, CENTER_ALIGN, LABEL_COLOR);
   drawText(m_font, "LIVES",      16, {centre(3), 20}, CENTER_ALIGN, LABEL_COLOR);

   drawText(m_font, std::to_string(m_hud.score),     32, {centre(0), 42}, CENTER_ALIGN, TEXT_COLOR);
   drawText(m_font, std::to_string(m_hud.highScore), 32, {centre(1), 42}, CENTER_ALIGN, TEXT_COLOR);
   drawText(m_font, std::to_string(m_hud.level),     32, {centre(2), 42}, CENTER_ALIGN, TEXT_COLOR);

   // Lives as small knights.
   if (m_lifeIcon.getSize().x > 0) {
      const float icon = 32, gap = 4;
      float total = m_hud.lives * icon + std::max(0, m_hud.lives - 1) * gap;
      float x = centre(3) - total / 2;
      sf::Sprite s(m_lifeIcon);
      s.setScale(icon / m_lifeIcon.getSize().x, icon / m_lifeIcon.getSize().y);
      for (int i = 0; i < m_hud.lives; ++i) {
         s.setPosition(std::round(x + i * (icon + gap)), 42);
         m_window.draw(s);
      }
   }

   // Potion bar along the bottom of the panel, while a potion lasts.
   if (m_hud.bar >= 0) {
      const float barW = std::min(360.0f, w * 0.5f), barH = 20;
      sf::FloatRect back(left + (w - barW) / 2, h - 34, barW, barH);
      drawPanel(m_barBack, back, 6, 1);

      float f = std::min(m_hud.bar, 1.0f);
      const sf::Texture &fill = f > 0.5f ? m_barGreen : f > 0.25f ? m_barYellow : m_barRed;
      float fillW = (barW - 8) * f;
      if (fillW >= 12)
         drawPanel(fill, sf::FloatRect(back.left + 4, back.top + 4, fillW, barH - 8), 4, 1);
   }
}

void Screen::drawBanner () {

   if (m_hud.banner.empty() || m_menu.visible)
      return;

   // Centre of the maze on the window.
   const sf::FloatRect vp = m_camera.getViewport();
   const sf::Vector2u size = m_window.getSize();
   const float cx = (vp.left + vp.width / 2) * size.x;
   const float cy = (vp.top + vp.height / 2) * size.y;

   sf::Text measure(m_hud.banner, m_font, 40);
   float tw = measure.getLocalBounds().width;

   sf::FloatRect panel(cx - tw / 2 - 32, cy - 36, tw + 64, 72);
   drawPanel(m_panel, panel, 8, 2);
   drawText(m_font, m_hud.banner, 40, {cx, cy - 20}, CENTER_ALIGN, TEXT_COLOR);
}

void Screen::drawMenu () {

   if (!m_menu.visible)
      return;

   const sf::Vector2u size = m_window.getSize();
   const float w = (float) size.x, h = (float) size.y;

   // Darken the game behind the menu.
   sf::RectangleShape shade(sf::Vector2f(w, h));
   shade.setFillColor(sf::Color(10, 6, 12, 170));
   m_window.draw(shade);

   const float buttonW = 320, buttonH = 56, gap = 16;
   const float panelW = std::min(w - 32, 560.0f);
   const float panelH = 40 + 80 + 48 + m_menu.options.size() * (buttonH + gap) + 56;
   const float left = (w - panelW) / 2, top = (h - panelH) / 2;

   drawPanel(m_panel, sf::FloatRect(left, top, panelW, panelH), 8, 3);

   float y = top + 36;
   drawText(m_titleFont, m_menu.title, 64, {w / 2, y}, CENTER_ALIGN, TEXT_COLOR);
   y += 80;
   drawText(m_font, m_menu.subtitle, 20, {w / 2, y}, CENTER_ALIGN, LABEL_COLOR);
   y += 48;

   for (size_t i = 0; i < m_menu.options.size(); ++i) {
      bool chosen = (int) i == m_menu.selected;
      sf::FloatRect button((w - buttonW) / 2, y, buttonW, buttonH);
      drawPanel(chosen ? m_buttonSelected : m_button, button, 8, 2);
      drawText(m_font, m_menu.options[i], 28, {w / 2, y + 12}, CENTER_ALIGN,
               chosen ? DARK_TEXT : TEXT_COLOR);
      y += buttonH + gap;
   }

   drawText(m_font, "ARROWS CHOOSE    ENTER CONFIRMS", 16, {w / 2, y + 12},
            CENTER_ALIGN, LABEL_COLOR);
}
