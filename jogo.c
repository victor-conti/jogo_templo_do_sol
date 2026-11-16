/* O SEGREDO DO TEMPO DO SOL
 * INCREMENTO 1: mapa, jogador e movimentação
 * INCREMENTO 2: itens, inventário limitado, PEGAR, EXAMINAR, INVENTARIO
*/
#include <stdio.h>          //usado para declarar funções de entrada e saída 
#include <string.h>         //usado para declarar funções de manipulação de strings
#include <ctype.h>          //usado para declarar a função para classificar e transformar caracteres individuais
#include <stdbool.h>        //usado para dar suporte a variáveis booleanas
#include <locale.h>         //usado para acentuação de palavras e caracteres especiais

#define NUM_AMBIENTES 9
#define NUM_ITENS 7
#define SEM_SAIDA -1
#define NO_INVENTARIO -1            /* item.local == -1  -> esta com o jogador*/
#define CAPACIDADE_INVENTARIO 6     /* capacidade limitada*/
#define TAM_NOME 50
#define TAM_Texto 256

/* Indices de vetor saidas [4]*/
enum Direcao { NORTE = 0, LESTE = 1, SUL = 2, OESTE = 3 };

typedef struct {
    char nome[TAM_NOME];
    int vida;
    int localAtual;
    int inventario[10];          /*guarda os ids dos itens*/
    int quantidadeItens;
} Jogador;

typedef struct {
    int id;
    char nome[TAM_NOME];
    char descricao[TAM_Texto];
    bool coletavel, utilizado;
    int local;                  /*id do ambiente onde esta, ou -1 se no inventario*/
} Item;

typedef struct {
    int id;
    char nome[TAM_NOME];
    char descricao[TAM_Texto];
    int saidas[4];              /* {NORTE, SUL, LESTE, OESTE} } -> id do destino ou -1 */
    bool visitado, bloqueado;
} Ambiente;

/* ---------- FUNÇÕES AUXILIARES ----------*/

void paraMaiusculo(char *texto) {
    for (; *texto != '\0'; texto++) 
        *texto = (char)toupper((unsigned char)*texto);
}

/* Le uma linha do teclado sem o '\n'. Devolve false se acabou a entrada */
bool lerLinha (char *destino, int tamanho) {
    if (fgets(destino, tamanho, stdin) == NULL) {
        destino[0] = '\0';
        return false;
    }
    if (strchr(destino, '\n') == NULL) {     /* linha maior que o buffer*/
        int c;
        while ((c = getchar()) != '\n' && c != EOF);  /* descarta o resto da linha*/ 
    }
    destino[strcspn(destino, "\r\n")] = '\0';  /* remove o '\n' ou '\r\n' do final da string*/
    return true;
}

/*Procura um item pelo nome em um local; devolve o indice ou -1*/
int encontrarItem(const Item itens[], int quantidade, const char *nome, int local) {
    for (int i = 0; i < quantidade; i++)
        if (strcmp(itens[i].nome, nome) == 0 && itens[i].local == local)
            return i;
    return -1;
}

/* Converte "NORTE"... em indice de direção; -1 se não for direção*/
int direcaoDoComando(const char *verbo) {
    if (strcmp(verbo, "NORTE") == 0) return NORTE;
    if (strcmp(verbo, "LESTE") == 0) return LESTE;
    if (strcmp(verbo, "SUL") == 0) return SUL;
    if (strcmp(verbo, "OESTE") == 0) return OESTE;
    return -1;
}

/* ---------- FUNÇÕES DO JOGO ----------*/

void iniciarJogo(Jogador *jogador) {
    printf("=== O SEGREDO DO TEMPO DO SOL ===\n");
    printf("O Dr. Voss também procura o Coração do Sol. Chegue primeiro!\n");
    printf("Qual o seu nome, aventureiro(a) ?");
    lerLinha(jogador->nome, TAM_NOME);
    jogador -> vida = 100;
    jogador -> localAtual = 0;                   /* começa no Acampamento*/
    jogador -> quantidadeItens = 0;
    for (int i = 0; i < 10; i++) jogador->inventario[i] = -1;
}

void mostrarAmbiente(const Ambiente *ambiente) {
    const char *nome[4] = {"NORTE", "LESTE", "SUL", "OESTE"};
    printf("\n=== &s ===\n", ambiente->nome);
    printf("%s\n", ambiente->descricao);
    printf("Saídas: ");
    for (int d = 0; d < 4; d++)
        if (ambiente->saidas[d] != SEM_SAIDA) printf("%s ", nome[d]);
    printf("\n");
}

void mostrarObjetos(const Item itens[], int quantidade, int local) {
    printf("Objetos: ");
    bool achou = false;
    for (int i = 0; i < quantidade; i++) {
        if (itens[i].local == local) {
            if (achou) printf(" | ");
            printf("%s", itens[i].nome);
            achou = true;
        }
    }
    if (!achou) printf("nenhum");
    printf("\n");
}

void mostrarInventario(const Jogador *jogador, const Item itens[]) {
    printf("Inventario (%d/%d): ", jogador->quantidadeItens, CAPACIDADE_INVENTARIO);
    if (jogador->quantidadeItens == 0) printf("vazio");
    for (int i = 0; i < jogador->quantidadeItens; i++) {
        if (i > 0) printf(" | ");
        printf("%s", itens[jogador->inventario[i]].nome);
    }
    printf("\n");
}

bool moverJogador(Jogador *jogador, Ambiente ambientes [], int quantidade, int direcao) {
    int destino = ambientes[jogador->localAtual].saidas[direcao];

    if (destino == SEM_SAIDA || destino < 0 || destino >= quantidade) {
        printf("Não há passagem nessa direcao.\n");
        return false;
    }
    if (ambientes[destino].bloqueado) {
        printf("O caminho está bloqueado. Você não pode passar.\n");
        return false;
    }
    jogador->localAtual = destino;
    ambientes[destino].visitado = true;
    return true;
}

bool coletarItem(Jogador *jogador, Item *item) {
    if (!item->coletavel) {
        printf("Você não consegue carregar isso.\n");
        return false;
    }
    if (jogador->quantidadeItens >= CAPACIDADE_INVENTARIO) {
        printf("Seu inventário está cheio. Você não pode pegar mais itens.\n");
        return false;
    }
    jogador->inventario[jogador->quantidadeItens] = item->id;
    jogador->quantidadeItens++;
    item->local = NO_INVENTARIO;
    printf("Você pegou: %s.\n", item->nome);
    return true; 
}

void examinarObjeto(const Item *item) {
    printf("%s: %s\n", item->nome, item->descricao);
}

/* ---------- CÓDIGO DO JOGO ----------*/

int main(void) {
    setlocale(LC_ALL, "Portuguese");
/*Mapa: {id, nome, descricao, {N, S, L, O}, visitado, bloqueado} */
    Ambiente ambientes[NUM_AMBIENTES] = {
        {0, "ACAMPAMENTO NA SELVA",
            "Sua barraca balança com vento. Ao norte, as ruinas do templo.",
            {4, -1, 1 , -1}, true, false},
        {1, "TRILHA NA SELVA",
            "Cipos e raizes cobrem o caminho. Ouve-se agua correndo ao norte.",
            {2, -1, -1, 0}, false, false},
        {2, "RIO DOS CROCODILOS",
            "Um rio lamacento. Olhos amarelos vigiam da margem.",
            {3, 1, -1, -1}, false, false},
        {3, "CÂMARA DO IDOLO",
            "Uma camara de pedra com um pedestal vazio no centro.",
            {-1, 2, -1, -1}, false, false},
        {4, "HALL DO TEMPLO",
            "Colunas gigantes sustentam um teto coberto de musgo.",
            {-1, 0, 5, 6}, false, false},
        {5, "SALA DOS HIEROGLIFOS",
            "Simbolos dourados cobrem as paredes. Um deles brilha fraco.",
            {-1, -1, -1, 4}, false, false},
        {6, "SALA DAS ARMADILHAS",
            "O piso tem placas de pedra suspeitas e furos nas paredes.",
            {7, -1, 4, -1}, false, false},
        {7, "CORREDOR SECRETO",
            "Um corredor estreito, escuro e frio.",
            {8, 6, -1, -1}, false, false},
        {8, "CÂMARA DO ARTEFATO",
            "No centro, um pedestal com três encaixes aguarda as relíquias. O Coração do Sol brilha no fundo.",
            {-1, 7, -1, -1}, false, false}
    };

    /* Itens: {id, nome, descricao, coletaval, utilizado, local} */
    Item itens[NUM_ITENS] = {
        {0, "LANTERNA", "Uma lanterna a querosene, ainda com bastante combustível.", true, false, 0},
        {1, "CHICOTE", "Um chicote de couro resistente, ideal para se balançar sobre obstáculos.", true, false, 0},
        {2, "CHAVE", "Uma chave de bronze com um sol gravado no cabo.", true, false, 1},
        {3, "ESTATUETA", "Uma estatueta de ouro de um chacel, com olhos de rubi.", true, false, 3},
        {4, "DIÁRIO", "Diário de um explorador. Uma página marcada diz: 'O nome que abre o caminho é o astro que da vida a este templo.'", true, false, 4},
        {5, "AMULETO", "Um amuleto de jade em forma de escaravelho.", true, false, 6},
        {6, "CRISTAL", "Um cristal amarelo que pulsa com uma luz quente.", true, false, 7}
    };

    Jogador jogador;
    iniciarJogo(&jogador);
    mostrarAmbiente(&ambientes[jogador.localAtual]);
    mostrarObjetos(itens, NUM_ITENS, jogador.localAtual);

    bool jogando = true;
    while (jogando) {
        char entrada[100], verbo[32], alvo[32];

        printf("\n> ");
        if (!lerLinha(entrada, sizeof entrada)) break; /* fim de arquivo */
        paraMaiusculo(entrada);

        /* Separa a entrada em VERBO e ALVO (ex.: "PEGAR LANTERNA") */
    verbo[0] == '\0';
    alvo[0] == '\0';
    sscanf(entrada, "%31s %31s[^\n]", verbo, alvo);

    if (verbo[0] == '\0') continue; /* entrada vazia */

    int direcao = direcaoDoComando(verbo);

    if(strcmp(verbo, "SAIR") == 0) {
        jogando = false;
    } else if (strcmp(verbo, "OLHAR") == 0) {
        mostrarAmbiente(&ambientes[jogador.localAtual]);
        mostrarObjetos(itens, NUM_ITENS, jogador.localAtual);
    } else if (direcao != -1) {
        if (moverJogador(&jogador, ambientes, NUM_AMBIENTES, direcao)) {
            mostrarAmbiente(&ambientes[jogador.localAtual]);
            mostrarObjetos(itens, NUM_ITENS, jogador.localAtual);
        }
    } else if (strcmp(verbo, "PEGAR") == 0) {
        if (alvo[0] == '\0') {
            printf("Pegar o que? Ex.: PEGAR LANTERNA\n");
        } else {
            int i = encontrarItem(itens, NUM_ITENS, alvo, jogador.localAtual);
            if (i == -1) printf("Não há '%s' aqui.\n", alvo);
            else coletarItem(&jogador, &itens[i]);
        }
    } else if (strcmp(verbo, "EXAMINAR") == 0) {
        if (alvo[0] == '\0') {
            printf("Examinar o que? Ex.: EXAMINAR DIÁRIO\n");
        } else {
            /* procura no ambiente e, se não achar, no inventário */
            int i = encontrarItem(itens, NUM_ITENS, alvo, jogador.localAtual);
            if (i == -1) i = encontrarItem(itens, NUM_ITENS, alvo, NO_INVENTARIO);
            if (i == -1) printf("Não há '%s' para examinar.\n", alvo);
            else examinarObjeto(&itens[i]);
        }
    } else if (strcmp(verbo, "INVENTARIO") == 0) {
        mostrarInventario(&jogador, itens);
    } else {
        printf("Comando desconhecido.\n", verbo);
    }
 } 
    printf("Até a próxima!\n");
    return 0;    
}