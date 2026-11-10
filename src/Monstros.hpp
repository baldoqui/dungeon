#pragma once

#include <string>

#include "Ator.hpp"
#include "Labirinto.hpp"
#include "PathFind.hpp"

/**
 * Os quatro monstros. Todos andam pelo caminho mais curto (A*, PathFind)
 * até um tile-alvo; o que muda entre eles é como escolhem o alvo:
 *
 *   Bruto    (demônio) vai direto no herói.
 *   Espreita (morcego) mira 4 tiles à frente do herói, para cortar o caminho.
 *   Cerco    (aranha)  pega o ponto 2 tiles à frente do herói e espelha a
 *            posição do Bruto em relação a ele: os dois fecham o herói
 *            pelos dois lados.
 *   Medroso  (gosma)   persegue de longe, mas foge para o canto dele quando
 *            chega perto (menos de 8 tiles).
 *
 * Na fase de dispersão, cada um volta para o seu canto. Vulneráveis
 * (depois que o herói toma uma poção), fogem do herói.
 */

enum Personalidade { BRUTO, ESPREITA, CERCO, MEDROSO };

enum EstadoMonstro {
   NO_CURRAL,   // esperando a vez de sair
   SAINDO,      // indo do curral até a porta
   ANDANDO,     // no labirinto
};

struct Monstro {
   Ator          ator;
   Personalidade personalidade;
   std::string   nome;
   std::string   imagem;           // arquivo em assets/kenney/
   Vector2D      canto;            // alvo na dispersão
   EstadoMonstro estado {NO_CURRAL};
   float         espera {0};       // segundos até sair do curral
   bool          vulneravel {false};
};

// O que os monstros precisam saber do resto do jogo para escolher o alvo.
struct Situacao {
   Vector2D heroi;
   Direcao  dirHeroi;
   Vector2D bruto;       // o Cerco usa a posição do Bruto
   bool     dispersao;   // true = todos para os cantos
};

// Tile que o monstro quer alcançar agora.
Vector2D alvoDe (const Monstro &m, const Situacao &s);

// Tile livre (fora do curral) mais perto de um alvo qualquer,
// que pode ter caído fora do mapa ou numa parede.
Vector2D alvoValido (const Vector2D &alvo, const Labirinto &lab);

// Primeiro passo do caminho mais curto (A*) até o alvo.
Direcao direcaoPara (const Ator &ator, const Vector2D &alvo,
                     const PathFind &caminho, const Livre &livre);

// Vulnerável: vira para o lado que mais afasta do herói.
Direcao direcaoDeFuga (const Ator &ator, const Vector2D &heroi,
                       const Livre &livre);
