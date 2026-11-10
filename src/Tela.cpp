#include "Tela.hpp"

#include <algorithm>

#include "Global.hpp"


// Estica um sprite para ocupar exatamente um tile.
static void cabeNumTile (sf::Sprite &sprite) {
   const sf::Texture *t = sprite.getTexture();
   if (t == nullptr || t->getSize().x == 0 || t->getSize().y == 0)
      return;
   sprite.setScale(TILE_SIZE / t->getSize().x, TILE_SIZE / t->getSize().y);
}


Tela::Tela (const TileMap &mapa) : m_mapa(mapa) {}

void Tela::margemSuperior (float tiles) {
   m_margemSuperior = tiles;
   if (m_janela.isOpen())
      ajustaCamera(m_janela.getSize().x, m_janela.getSize().y);
}

bool Tela::abre (unsigned largura, unsigned altura, const std::string &titulo) {
   m_janela.create(sf::VideoMode(largura, altura), titulo);
   if (!m_janela.isOpen())
      return false;

   m_janela.setFramerateLimit(60);
   ajustaCamera(largura, altura);
   return true;
}

void Tela::ajustaCamera (unsigned largura, unsigned altura) {

   m_telaFixa = sf::View(sf::FloatRect(0, 0, (float) largura, (float) altura));

   // O mundo visível: o mapa inteiro mais a margem de cima.
   const float mundoL = m_mapa.width() * TILE_SIZE;
   const float mundoA = (m_mapa.height() + m_margemSuperior) * TILE_SIZE;
   m_camera = sf::View(sf::FloatRect(0, -m_margemSuperior * TILE_SIZE, mundoL, mundoA));

   // Mesma proporção do mundo, centralizado na janela.
   const float proporcaoJanela = (float) largura / altura;
   const float proporcaoMundo  = mundoL / mundoA;

   if (proporcaoJanela > proporcaoMundo) {
      float l = proporcaoMundo / proporcaoJanela;
      m_camera.setViewport(sf::FloatRect((1 - l) / 2, 0, l, 1));
   } else {
      float a = proporcaoJanela / proporcaoMundo;
      m_camera.setViewport(sf::FloatRect(0, (1 - a) / 2, 1, a));
   }
}


//--------------------------------------------------------------
// Aparência

bool Tela::imagemDaParede (const std::string &arquivo) {
   m_temParede = m_imagemParede.loadFromFile(arquivo);
   return m_temParede;
}

bool Tela::imagemDoChao (const std::string &arquivo) {
   m_temChao = m_imagemChao.loadFromFile(arquivo);
   return m_temChao;
}

Tela::EstiloDeItem &Tela::estilo (int item) {
   if ((int) m_estilos.size() <= item)
      m_estilos.resize(item + 1);
   return m_estilos[item];
}

void Tela::corDoItem (int item, sf::Color cor, float raioEmTiles) {
   estilo(item).cor  = cor;
   estilo(item).raio = raioEmTiles;
}

bool Tela::imagemDoItem (int item, const std::string &arquivo) {
   EstiloDeItem &e = estilo(item);
   e.temImagem = e.imagem.loadFromFile(arquivo);
   return e.temImagem;
}

bool Tela::fonteDoPlacar (const std::string &arquivo) {
   m_temFonte = m_fonte.loadFromFile(arquivo);
   if (m_temFonte) {
      m_texto.setFont(m_fonte);
      m_texto.setCharacterSize(28);
      m_texto.setFillColor(sf::Color::White);
      m_texto.setPosition(16, 10);
   }
   return m_temFonte;
}

void Tela::textoDoPlacar (const std::string &textoUtf8) {
   m_texto.setString(sf::String::fromUtf8(textoUtf8.begin(), textoUtf8.end()));
}


//--------------------------------------------------------------
// Laço principal

void Tela::roda () {

   sf::Clock relogio;

   while (m_janela.isOpen()) {

      sf::Event evento;
      while (m_janela.pollEvent(evento)) {
         if (evento.type == sf::Event::Closed)
            m_janela.close();
         else if (evento.type == sf::Event::Resized)
            ajustaCamera(evento.size.width, evento.size.height);
         else if (evento.type == sf::Event::KeyPressed && m_tecla)
            m_tecla(evento.key.code);
      }

      float dt = relogio.restart().asSeconds();
      if (m_quadro)
         m_quadro(dt);

      m_janela.clear(m_fundo);

      m_janela.setView(m_camera);
      desenhaLabirinto();
      desenhaItens();
      desenhaPersonagens();

      m_janela.setView(m_telaFixa);
      desenhaPlacar();

      m_janela.display();
   }
}


//--------------------------------------------------------------
// Desenho

void Tela::desenhaLabirinto () {

   sf::Sprite parede(m_imagemParede), chao(m_imagemChao);
   cabeNumTile(parede);
   cabeNumTile(chao);

   sf::RectangleShape bloco(sf::Vector2f(TILE_SIZE, TILE_SIZE));
   bloco.setFillColor(m_corParede);

   for (int row = 0; row < m_mapa.height(); ++row)
      for (int col = 0; col < m_mapa.width(); ++col) {
         sf::Vector2f onde(col * TILE_SIZE, row * TILE_SIZE);

         if (m_mapa.isWall(col, row)) {
            if (m_temParede) { parede.setPosition(onde); m_janela.draw(parede); }
            else             { bloco.setPosition(onde);  m_janela.draw(bloco);  }
         } else if (m_temChao) {
            chao.setPosition(onde);
            m_janela.draw(chao);
         }
      }
}

void Tela::desenhaItens () {

   if (m_itens == nullptr)
      return;

   const int largura = m_mapa.width();

   for (int row = 0; row < m_mapa.height(); ++row)
      for (int col = 0; col < largura; ++col) {
         size_t i = (size_t) row * largura + col;
         if (i >= m_itens->size())
            return;

         int item = (*m_itens)[i];
         if (item <= 0 || item >= (int) m_estilos.size())
            continue;

         const EstiloDeItem &e = m_estilos[item];

         if (e.temImagem) {
            sf::Sprite s(e.imagem);
            cabeNumTile(s);
            s.setPosition(col * TILE_SIZE, row * TILE_SIZE);
            m_janela.draw(s);
         } else {
            float r = e.raio * TILE_SIZE;
            sf::CircleShape c(r);
            c.setOrigin(r, r);
            c.setFillColor(e.cor);
            c.setPosition((col + 0.5f) * TILE_SIZE, (row + 0.5f) * TILE_SIZE);
            m_janela.draw(c);
         }
      }
}

void Tela::desenhaPersonagens () {
   for (NPC *npc: m_npcs) {
      if (npc == nullptr || !npc->active)
         continue;
      cabeNumTile(npc->m_sprite);
      npc->m_sprite.setPosition(npc->body.position.x * TILE_SIZE,
                                npc->body.position.y * TILE_SIZE);
      m_janela.draw(npc->m_sprite);
   }
}

void Tela::desenhaPlacar () {

   if (!m_temFonte)
      return;

   const float larguraBarra = 240, alturaBarra = 16, espaco = 10;
   const bool  temBarra = m_barra >= 0;

   sf::FloatRect t = m_texto.getGlobalBounds();
   float largura = temBarra ? std::max(t.width, larguraBarra) : t.width;
   float altura  = t.height + (temBarra ? espaco + alturaBarra : 0);

   sf::RectangleShape fundo(sf::Vector2f(largura + 24, altura + 20));
   fundo.setPosition(t.left - 12, t.top - 10);
   fundo.setFillColor(sf::Color(0, 0, 0, 170));
   m_janela.draw(fundo);
   m_janela.draw(m_texto);

   if (!temBarra)
      return;

   float topo = t.top + t.height + espaco;

   sf::RectangleShape vazio(sf::Vector2f(larguraBarra, alturaBarra));
   vazio.setPosition(t.left, topo);
   vazio.setFillColor(sf::Color(70, 70, 70));
   m_janela.draw(vazio);

   float fracao = std::min(m_barra, 1.0f);
   sf::RectangleShape cheio(sf::Vector2f(larguraBarra * fracao, alturaBarra));
   cheio.setPosition(t.left, topo);
   cheio.setFillColor(fracao > 0.5f  ? sf::Color(60, 200, 80)
                    : fracao > 0.25f ? sf::Color(240, 200, 40)
                                     : sf::Color(220, 60, 50));
   m_janela.draw(cheio);
}
