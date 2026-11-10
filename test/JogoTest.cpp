// Testes das regras, sem abrir janela.

#include <gtest/gtest.h>

#include "Jogo.hpp"

namespace {

const float QUADRO = 1.0f / 60;

// Um jogo novo, parado no instante em que acaba o "Preparar...".
struct JogoTest : ::testing::Test {
   Jogo jogo;

   void SetUp () override {
      ASSERT_TRUE(jogo.prepara(ASSETS_DIR "labirinto.txt"));
      jogo.comecaJogo();
      for (int i = 0; i < 1000 && jogo.etapa != JOGANDO; ++i)
         jogo.atualiza(QUADRO);
      ASSERT_EQ(jogo.etapa, JOGANDO);
      ASSERT_EQ(jogo.pontos, 0);
   }

   void roda (float segundos) {
      for (float t = 0; t < segundos; t += QUADRO)
         jogo.atualiza(QUADRO);
   }

   // Tira os monstros do caminho: todos parados no curral.
   void prendeMonstros () {
      for (auto &m: jogo.monstros) {
         m.ator.colocaEm(jogo.lab.curral[0]);
         m.estado = NO_CURRAL;
         m.espera = 1000;
      }
   }
};


// --- Labirinto -------------------------------------------------------------

TEST_F(JogoTest, LabirintoTemTamanhoEItensDoArquivo) {
   EXPECT_EQ(jogo.lab.largura(), 21);
   EXPECT_EQ(jogo.lab.altura(), 23);
   EXPECT_EQ(jogo.lab.mapa.width(), 21);
   EXPECT_EQ(jogo.lab.curral.size(), 5u);
}

// Sem bloco 2x2 aberto, o A* nunca precisa andar na diagonal.
TEST_F(JogoTest, LabirintoNaoTemBloco2x2Aberto) {
   const Labirinto &lab = jogo.lab;
   for (int r = 0; r + 1 < lab.altura(); ++r)
      for (int c = 0; c + 1 < lab.largura(); ++c)
         EXPECT_FALSE(lab.livreParaMonstro(c, r) && lab.livreParaMonstro(c + 1, r) &&
                      lab.livreParaMonstro(c, r + 1) && lab.livreParaMonstro(c + 1, r + 1))
            << "bloco aberto em " << c << "," << r;
}

TEST_F(JogoTest, HeroiNaoEntraNoCurralMasMonstroSim) {
   Vector2D porta = jogo.lab.saidaDoCurral + Vector2D(0, 1);
   EXPECT_FALSE(jogo.lab.livreParaHeroi((int) porta.x, (int) porta.y));
   EXPECT_TRUE(jogo.lab.livreParaMonstro((int) porta.x, (int) porta.y));
}


// --- Herói -----------------------------------------------------------------

TEST_F(JogoTest, HeroiComePontinhosAndando) {
   prendeMonstros();
   int antes = jogo.lab.itensRestantes();
   roda(1.0f);   // começa indo para a esquerda, por um corredor de pontinhos
   int comidos = antes - jogo.lab.itensRestantes();
   EXPECT_GE(comidos, 3);
   EXPECT_EQ(jogo.pontos, comidos * 10);
}

TEST_F(JogoTest, HeroiParaNaParede) {
   prendeMonstros();
   jogo.tecla(sf::Keyboard::Left);
   roda(5.0f);
   // Na linha do início, a parede da esquerda fica na coluna 5.
   EXPECT_TRUE(jogo.heroi.noCentro());
   EXPECT_EQ(jogo.heroi.col(), 6);
   EXPECT_TRUE(jogo.heroi.dir.parado());
}

TEST_F(JogoTest, HeroiDaMeiaVoltaNaHora) {
   prendeMonstros();
   roda(0.1f);
   ASSERT_EQ(jogo.heroi.dir, ESQUERDA);
   jogo.tecla(sf::Keyboard::Right);
   jogo.atualiza(QUADRO);
   EXPECT_EQ(jogo.heroi.dir, DIREITA);
}

TEST_F(JogoTest, TunelLevaParaOOutroLado) {
   prendeMonstros();
   jogo.heroi.colocaEm(Vector2D(1, 11));
   jogo.tecla(sf::Keyboard::Left);
   roda(0.6f);
   EXPECT_GT(jogo.heroi.npc.body.position.x, jogo.lab.largura() - 3);
}


// --- Poção e monstros ------------------------------------------------------

TEST_F(JogoTest, PocaoDeixaMonstrosVulneraveisEPontosDobram) {
   prendeMonstros();
   jogo.heroi.colocaEm(Vector2D(1, 1));   // em cima de uma poção
   jogo.atualiza(QUADRO);
   EXPECT_GT(jogo.tempoVulneravel, 0);
   for (auto &m: jogo.monstros)
      EXPECT_TRUE(m.vulneravel);

   int antes = jogo.pontos;
   for (int i = 0; i < 2; ++i) {
      Monstro &m = jogo.monstros[i];
      m.estado = ANDANDO;
      m.ator.colocaEm(jogo.heroi.npc.body.position);
      jogo.atualiza(QUADRO);
      EXPECT_EQ(m.estado, NO_CURRAL);
   }
   EXPECT_EQ(jogo.pontos - antes, 200 + 400);
}

TEST_F(JogoTest, MonstroPegaHeroiETiraVida) {
   Monstro &bruto = jogo.monstros[BRUTO];
   bruto.ator.colocaEm(jogo.heroi.npc.body.position);
   jogo.atualiza(QUADRO);
   EXPECT_EQ(jogo.etapa, PEGO);
   EXPECT_EQ(jogo.vidas, 2);
   roda(2.0f);
   EXPECT_EQ(jogo.etapa, PRONTO);
}

TEST_F(JogoTest, SemVidasAcabaERRecomeca) {
   jogo.vidas = 1;
   jogo.monstros[BRUTO].ator.colocaEm(jogo.heroi.npc.body.position);
   jogo.atualiza(QUADRO);
   roda(2.0f);
   EXPECT_EQ(jogo.etapa, FIM_DE_JOGO);

   jogo.tecla(sf::Keyboard::R);
   EXPECT_EQ(jogo.vidas, 3);
   EXPECT_EQ(jogo.pontos, 0);
   EXPECT_EQ(jogo.etapa, PRONTO);
}

TEST_F(JogoTest, MonstrosSaemDoCurral) {
   prendeMonstros();
   for (auto &m: jogo.monstros)
      m.espera = 0;
   jogo.heroi.colocaEm(Vector2D(1, 21));   // longe, num canto
   roda(4.0f);
   for (auto &m: jogo.monstros)
      EXPECT_EQ(m.estado, ANDANDO) << m.nome;
}

// Partida longa com o herói parado: os monstros acham ele, e ninguém
// entra em parede nem no curral depois de sair.
TEST_F(JogoTest, MonstrosAlcancamOHeroiSemAtravessarParede) {
   jogo.heroi.colocaEm(Vector2D(1, 21));
   bool pegou = false;
   for (float t = 0; t < 60 && !pegou; t += QUADRO) {
      jogo.desejo = PARADO;
      jogo.atualiza(QUADRO);
      pegou = jogo.etapa == PEGO;
      for (auto &m: jogo.monstros) {
         ASSERT_TRUE(jogo.lab.livreParaMonstro(m.ator.col(), m.ator.row())) << m.nome;
         if (m.estado == ANDANDO)
            ASSERT_FALSE(jogo.lab.noCurral(m.ator.col(), m.ator.row())) << m.nome;
      }
   }
   EXPECT_TRUE(pegou);
}

TEST_F(JogoTest, ComerTudoPassaDeFase) {
   prendeMonstros();
   // Deixa só um pontinho: o que fica à esquerda do início.
   Labirinto &lab = jogo.lab;
   int col = (int) lab.inicioHeroi.x - 1, row = (int) lab.inicioHeroi.y;
   ASSERT_EQ(lab.item(col, row), PONTINHO);
   for (int r = 0; r < lab.altura(); ++r)
      for (int c = 0; c < lab.largura(); ++c)
         if (c != col || r != row)
            lab.tiraItem(c, r);
   ASSERT_EQ(lab.itensRestantes(), 1);

   roda(0.5f);
   EXPECT_EQ(jogo.etapa, FASE_COMPLETA);
   roda(2.5f);
   EXPECT_EQ(jogo.fase, 2);
   EXPECT_GT(lab.itensRestantes(), 100);
}

}  // namespace
