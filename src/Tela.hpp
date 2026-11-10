#pragma once

#include <functional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "NPC.hpp"
#include "TileMap.hpp"

/**
 * A janela do jogo: desenha o labirinto inteiro, os itens, os personagens
 * e o placar, e chama o jogo a cada quadro e a cada tecla.
 *
 * Unidades: posições dos personagens (npc.body.position) em TILES, canto
 * superior esquerdo; cada tile ocupa TILE_SIZE x TILE_SIZE no mundo, e
 * qualquer imagem é esticada para caber em um tile.
 *
 * A câmera fica parada mostrando o mapa inteiro, sem distorcer: sobram
 * faixas nas laterais (ou em cima e embaixo) conforme o formato da janela.
 */
class Tela {
public:
   using FuncaoQuadro = std::function<void (float dt)>;
   using FuncaoTecla  = std::function<void (sf::Keyboard::Key tecla)>;

   // O mapa precisa existir enquanto a tela existir.
   explicit Tela (const TileMap &mapa);

   // Espaço livre acima do mapa, em tiles, para o placar não cobrir nada.
   void margemSuperior (float tiles);

   bool abre (unsigned largura, unsigned altura, const std::string &titulo);

   // Chamada uma vez por quadro, com o tempo desde o quadro anterior.
   void aoPassarQuadro (FuncaoQuadro f) { m_quadro = std::move(f); }

   // Chamada para cada tecla apertada, na ordem, sem perder nenhuma.
   void aoApertarTecla (FuncaoTecla f) { m_tecla = std::move(f); }

   // Aparência do labirinto.
   void corDeFundo   (sf::Color cor) { m_fundo = cor; }
   void corDaParede  (sf::Color cor) { m_corParede = cor; }
   bool imagemDaParede (const std::string &arquivo);
   bool imagemDoChao   (const std::string &arquivo);

   // Itens: uma int por tile (mesmo tamanho do mapa). 0 = nada; os outros
   // valores são desenhados com a imagem ou, sem imagem, como círculo.
   void itens (const std::vector<int> *grade) { m_itens = grade; }
   void corDoItem    (int item, sf::Color cor, float raioEmTiles);
   bool imagemDoItem (int item, const std::string &arquivo);

   // Personagens, desenhados na ordem em que foram adicionados.
   void adiciona (NPC *npc) { m_npcs.push_back(npc); }

   // Placar no canto superior esquerdo da janela.
   bool fonteDoPlacar (const std::string &arquivo);
   void textoDoPlacar (const std::string &textoUtf8);
   // Barra embaixo do texto, de 0 a 1; negativo esconde.
   void barraDoPlacar (float fracao) { m_barra = fracao; }

   // Laço principal; volta quando a janela fecha.
   void roda ();

private:
   struct EstiloDeItem {
      sf::Color   cor {255, 255, 255};
      float       raio {0.1f};
      sf::Texture imagem;
      bool        temImagem {false};
   };

   const TileMap &m_mapa;
   sf::RenderWindow m_janela;
   sf::View m_camera;         // mundo (labirinto)
   sf::View m_telaFixa;       // pixels da janela (placar)
   float m_margemSuperior {0};

   FuncaoQuadro m_quadro;
   FuncaoTecla  m_tecla;

   sf::Color   m_fundo {20, 16, 24};
   sf::Color   m_corParede {120, 120, 130};
   sf::Texture m_imagemParede, m_imagemChao;
   bool        m_temParede {false}, m_temChao {false};

   const std::vector<int> *m_itens {nullptr};
   std::vector<EstiloDeItem> m_estilos;   // índice = valor do item

   std::vector<NPC *> m_npcs;

   sf::Font m_fonte;
   sf::Text m_texto;
   bool     m_temFonte {false};
   float    m_barra {-1};

   void ajustaCamera (unsigned largura, unsigned altura);
   EstiloDeItem &estilo (int item);

   void desenhaLabirinto ();
   void desenhaItens ();
   void desenhaPersonagens ();
   void desenhaPlacar ();
};
