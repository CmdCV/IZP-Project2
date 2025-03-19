#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define LEFT_WALL 1
#define RIGHT_WALL 2
#define HORIZONTAL_WALL 4

#define SHORTEST_METHOD 0
#define LEFT_HAND_METHOD 1
#define RIGHT_HAND_METHOD 2

typedef struct {
    int rows; //pocet radku
    int cols; //pocet sloupcu
    unsigned char *cells; //ukazatel na pole[rows][cols]
} Map;

typedef struct {
    int r;
    int c;
} Position;

void print_help();

bool file_validate(const char *filename); // Validace vstupniho souboru
Map *map_init(int rows, int cols); // Inicializace mapy
Map *map_load(const char *filename); // Nacteni mapy ze souboru
void map_free(Map *m); // Uvolneni pameti
bool isborder(Map *map, int row, int c, int border); // Nachazi se na dane pozici v policku stena?
bool has_upper_wall(int r, int c); // Vrati true pokud ma bunka horni hranici
int start_border(Map *map, int r, int c); // Vrati hodnotu reprezentujici stenu pres kterou vstupujeme do bludiste
void print_path(Map *map, int r, int c, int leftright); // Vytiskne cestu bludistem
int pathfinder(Map *map, int *r, int *c, int from,int leftright); // Vrati hodnotu reprezentujici stenu pres kterou vstupujeme do nasledujiciho pole
int index_2d_to_1d(int r, int c, int cols); // Prevede index 2D pole na index 1D pole
void measure_distances(Map *map, int r, int c, int *cell_distances, int index); // Nastavi do pole cell_distances vzdalenost poli od vstupu
Position find_nearest_exit(Map *map, const int *cell_destances); // Najde nejblizsi vychod z bludiste
Position *find_shortest_path(Map *map, const int *cell_distances, Position shortest_exit, int path_lenght); // Najde a ulozi cestu k nejblizsimu vychodu

int main(int argc, const char *argv[]) {
    // Ocekavame pouze tyto mozne vstupy zbytek je spatne
    // ./maze --help
    // ./maze --test <map>
    // ./maze --rpath <R> <C> <map>
    if (argc < 2) {
        fprintf(stderr, "Pouzijte argument --help pro zobrazeni napovedy\n");
        return 1;
    }
    if (strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }
    if (argc < 3) {
        fprintf(stderr, "Neplatne argumenty\n");
        return 1;
    } else if (strcmp(argv[1], "--test") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: ./maze --test <map>\n");
            return 1;
        } else {
            printf("%s\n", file_validate(argv[2]) ? "Valid" : "Invalid");
            return 0;
        }
    }
    if (argc != 5) {
        fprintf(stderr, "Usage: ./maze [argument] R C <map>\n");
        return 1;
    }

    // Pokud uzivatel zadal spravny pocet argumentu, kontroulujeme zda jsou validni
    if (!strcmp(argv[1], "--rpath") || !strcmp(argv[1], "--lpath") || !strcmp(argv[1], "--shortest")) {
        Map *map = map_load(argv[4]);
        if (map == NULL) {
            fprintf(stderr, "Nepodarilo se nacist mapu ze souboru\n");
            return 1;
        }
        int r = atoi(argv[2]) - 1;
        int c = atoi(argv[3]) - 1;
        // Nastaveni metody hledani cesty
        int leftright = -1;
        if (strcmp(argv[1], "--rpath") == 0) {
            leftright = RIGHT_HAND_METHOD;
        } else if (strcmp(argv[1], "--lpath") == 0) {
            leftright = LEFT_HAND_METHOD;
        } else if (strcmp(argv[1], "--shortest") == 0) {
            leftright = SHORTEST_METHOD;
        }

        // Overeni zda byla metoda hledani cesty nastavena, pote pokracuje k hledani
        if (leftright == -1) {
            fprintf(stderr, "Neplatny argument %s\n", argv[1]);
            return 1;
        }
        print_path(map, r, c, leftright);
        map_free(map);
        return 0;
    }

    fprintf(stderr, "Neplatne argumenty\n");
    return 1;
}

void print_help() {
    printf("Usage: ./maze [argument] R C <map>\n");
    printf("argument:\n");
    printf("    --help - program vytiskne napovedu pouzivani programu a skonci.\n");
    printf("    --test - zkontroluje zda je zdrojovy soubor s mapou validni\n");
    printf("    --rpath - hleda pruchod bludistem pomoci pravidla prave ruky (prava ruka vzdy na zdi)\n");
    printf("    --lpath - hleda pruchod bludistem pomoci pravidla leve ruky (leva ruka vzdy na zdi)\n");
    printf("    --shortest - hleda nejkratsi cestu z bludiste\n");
    printf("R - adrersa pocatecniho radku\n");
    printf("C - adrersa pocatecniho sloupce\n");
    printf("<map> - nazev zdrojoveho souboru s mapou\n");
}

// filename (char*) - Cesta k soboru s mapou
// --------------------------------------------------------
// return (bool) - true == je validni | false == neni validni
bool file_validate(const char *filename) {
    FILE *soubor = fopen(filename, "r");
    if (soubor == NULL) {
        fprintf(stderr, "Chyba pri otevirani souboru\n");
        return false;
    }

    int znak;
    int rows = 0; // celkovy pocet radku
    int cols = 0; // celkovy pocet sloupcu

    fscanf(soubor, "%d %d\n", &rows, &cols);
    if (rows < 1 || cols < 1) {
        fprintf(stderr, "Spatne zadane rozmery bludiste\n");
        fclose(soubor);
        return false;
    }

    Map *map = map_init(rows, cols);
    if (map == NULL) {
        fprintf(stderr, "Chyba pri alokaci mapy\n");
        return false;
    }
    int i = 0;
    while (fscanf(soubor, "%d", &znak) != EOF) {
        if (i >= rows * cols) {
            fprintf(stderr, "Soubor obsahuje vice znaku nez je deklarovano\n");
            fclose(soubor);
            return false;
        }
        if ((znak - '0') > 7) {
            fprintf(stderr, "Soubor obsahuje nepovolene znaky!\n");
            fclose(soubor);
            return false;
        } else {
            map->cells[i] = (znak - '0');
        }
        i++;
    }

    if(i<rows * cols) {
        fprintf(stderr, "Soubor obsahuje mene znaku nez je deklarovano\n");
        fclose(soubor);
        return false;
    }


    //Kontrola spravnosti hranic
    for (int i = 0; i < map->rows; ++i) {
        for (int j = 0; j < map->cols; ++j) {
            if ((j > 0) && isborder(map, i, j, LEFT_WALL) == !isborder(map, i, j - 1, RIGHT_WALL)) {
                fprintf(stderr, "Na R:%d C:%d nesouhlasi leva stena\n", i + 1, j + 1);
                return false;
            }
            if ((j + 1 < map->cols) && isborder(map, i, j, RIGHT_WALL) == !isborder(map, i, j + 1, LEFT_WALL)) {
                fprintf(stderr, "Na R:%d C:%d nesouhlasi prava stena\n", i + 1, j + 1);
                return false;
            }
            if ((i > 0) && has_upper_wall(i, j) &&
                isborder(map, i, j, HORIZONTAL_WALL) == !isborder(map, i - 1, j, HORIZONTAL_WALL)) {
                fprintf(stderr, "Na R:%d C:%d nesouhlasi horni stena\n", i + 1, j + 1);
                return false;
            }
            if ((i + 1 < map->rows) && !has_upper_wall(i, j) &&
                isborder(map, i + 1, j, HORIZONTAL_WALL) == !isborder(map, i, j, HORIZONTAL_WALL)) {
                fprintf(stderr, "Na R:%d C:%d nesouhlasi dolni stena\n", i + 1, j + 1);
                return false;
            }
        }
    }

    map_free(map);
    fclose(soubor);
    return true;
}

// rows (int) - pocet radku k inicializaci
// cols (int) - poce sloupcu k inicializaci
// --------------------------------------------------------
// return (Map*) - Ukazatel na inicializovanou mapu
Map *map_init(int rows, int cols) {
    Map *map = (Map *) malloc(sizeof(Map));
    if (map == NULL) {
        fprintf(stderr, "Chyba pri alokaci pameti pro strukturu Map.\n");
        return NULL;
    }
    map->rows = rows;
    map->cols = cols;

    // Alokace pameti pro pole radku
    map->cells = (unsigned char *) malloc(rows * cols * sizeof(unsigned char));
    if (map->cells == NULL) {
        fprintf(stderr, "Chyba pri alokaci pameti pro pole bunek.\n");
        free(map);
        return NULL;
    }
    // Inicializace bunek, vychozi hodnotou 0
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            map->cells[index_2d_to_1d(i, j, cols)] = 0;
        }
    }

    return map;
}

// filename (char*) - Cesta k soboru s mapou
// --------------------------------------------------------
// return (Map*) - Ukazatel na nactenou mapu
Map *map_load(const char *filename) {
    // Overi zda je soubor validni
    if (!file_validate(filename)) {
        fprintf(stderr, "Soubor neprosel validaci\n");
        return NULL;
    }

    FILE *soubor = fopen(filename, "r");
    if (soubor == NULL) {
        fprintf(stderr, "Chyba pri otevirani souboru\n");
        return NULL;
    }
    int znak;
    int rows = 0; // celkovy pocet radku
    int cols = 0; // celkovy pocet sloupcu

    fscanf(soubor, "%d %d\n", &rows, &cols);
    if (rows < 1 || cols < 1) {
        fprintf(stderr, "Spatne zadane rozmery bludiste\n");
        fclose(soubor);
        return false;
    }

    Map *map = map_init(rows, cols);
    if (map == NULL) {
        fprintf(stderr, "Chyba pri alokaci mapy\n");
        return false;
    }

    for (int i = 0; fscanf(soubor, "%d", &znak) != EOF; i++) {
        if (i >= rows * cols) {
            fprintf(stderr, "Soubor obsahuje vice znaku nez je deklarovano\n");
            fclose(soubor);
            return false;
        }
        if ((znak - '0') > 7) {
            fprintf(stderr, "Soubor obsahuje nepovolene znaky!\n");
            fclose(soubor);
            return false;
        } else {
            map->cells[i] = (znak - '0');
        }
    }

    fclose(soubor);
    return map;
}

// map (Map*) - Ukazatel na mapu
void map_free(Map *m) {
    free(m->cells);
    free(m);
}

// map (Map*) - Ukazatel na mapu
// r (int) - Index radku
// c (int) - Index policka
// border (int) - Hranice na kterou se dotazujeme
// --------------------------------------------------------
// return (bool) - true == border je nepruchozi | false == border je pruchozi
bool isborder(Map *map, int r, int c, int border) {
    if (border != LEFT_WALL && border != RIGHT_WALL && border != HORIZONTAL_WALL) {
        return false;
    }
    // border(int) nabyva hodnot 100(LEFT_WALL), 010(RIGHT_WALL), 001(HORIZONTAL_WALL)
    // takze porovnanim s hodnotou v poly cells zjistime zda je hranice pruchozi
    return map->cells[index_2d_to_1d(r, c, map->cols)] & border;
}

// r (int) - Index radku
// c (int) - Index policka
// --------------------------------------------------------
// return (bool) - true == bunka ma horni hranici | false == bunka ma spodni hranici
bool has_upper_wall(int r, int c) {
    // Jestlize je radek i sloupec stejneho typu (sudy/lichy) tak ma bunka horni hranici
    // Suda cisla maji vzdy nulovy posledni bit
    return (r & 1) == (c & 1);
}

// map (Map*) - Ukazatel na mapu
// r (int*) - Index radku
// c (int*) - Index policka
// leftright (int) - metoda hledani cesty
void print_path(Map *map, int r, int c, int leftright) {
    if (leftright == LEFT_HAND_METHOD || leftright == RIGHT_HAND_METHOD) {
        printf("%d,%d\n", r + 1, c + 1);
        // next pres kterou hranici se vstupuje do dalsi bunky
        int next = pathfinder(map, &r, &c, start_border(map, r, c), leftright);
        // to se opakuje dokud se nedostaneme na okraj bludiste
        while ((r >= 0 && r < map->rows) && (c >= 0 && c < map->cols)) {
            printf("%d,%d\n", r + 1, c + 1);
            next = pathfinder(map, &r, &c, next, leftright);
        }
    } else if (leftright == SHORTEST_METHOD) {
        // Alokace pameti pro pole vzdalenosti bunek od vstupu
        int *cell_destances = (int *) malloc(map->rows * map->cols * sizeof(int));
        if (cell_destances == NULL) {
            fprintf(stderr, "Chyba pri alokaci pameti pro pole bunek.\n");
            return;
        }
        // Inicializace bunek, vychozi hodnotou 0
        for (int i = 0; i < map->rows; i++) {
            for (int j = 0; j < map->cols; j++) {
                cell_destances[index_2d_to_1d(i, j, map->cols)] = map->rows * map->cols + 1;
            }
        }

        // Nastaveni vzdalenosti bunek od vstupu
        measure_distances(map, r, c, cell_destances, 0);

        // Najde nejblizsi vychod z bludiste
        Position nearest_exit = find_nearest_exit(map, cell_destances);
        int path_lenght = cell_destances[index_2d_to_1d(nearest_exit.r, nearest_exit.c, map->cols)];

        // Najde a ulozi cestu k nejblizsimu vychodu
        Position *shortest_path = find_shortest_path(map, cell_destances, nearest_exit, path_lenght);

        // Vytiskne cestu k nejblizsimu vychodu
        for (int i = path_lenght; i >= 0; i--) {
            printf("%d,%d\n", shortest_path[i].r + 1, shortest_path[i].c + 1);
        }

        // uvolneni pameti
        free(shortest_path);
        free(cell_destances);
    }
}

// map (Map*) - Ukazatel na mapu
// cell_distances (int*) - pole vzdalenosti od vstupu
// shortest_exit (Position) - Souradnice nejblizsiho vychodu
// path_lenght (int) - delka cesty k vychodu
// --------------------------------------------------------
// return (Position) - pole souradnic cesty k nejblizsiho vychodu
Position *find_shortest_path(Map *map, const int *cell_distances, Position shortest_exit, int path_lenght) {
    Position *shortest_path = malloc((path_lenght + 1) * sizeof(Position));
    shortest_path[0] = (Position) {shortest_exit.r, shortest_exit.c};

    // Hleda mozne cesty z vychodu k vstupu, tak aby predchozi skok byl vzdy o jedno mensi a bylo mozne z daneho pole prejit na druhe
    for(int i = 1; i <= path_lenght; i++) {
        int pos = cell_distances[index_2d_to_1d(shortest_exit.r, shortest_exit.c, map->cols)];
        if(shortest_exit.r>0 && has_upper_wall(shortest_exit.r, shortest_exit.c) && !isborder(map,shortest_exit.r,shortest_exit.c,HORIZONTAL_WALL)){
            int pos_next = cell_distances[index_2d_to_1d(shortest_exit.r - 1, shortest_exit.c, map->cols)];
            if(pos == pos_next + 1){
                shortest_exit.r--;
            }
        }
        if(shortest_exit.r < map->rows && !has_upper_wall(shortest_exit.r, shortest_exit.c) && !isborder(map, shortest_exit.r, shortest_exit.c, HORIZONTAL_WALL)) {
            int pos_next = cell_distances[index_2d_to_1d(shortest_exit.r + 1, shortest_exit.c, map->cols)];
            if(pos == pos_next + 1){
                shortest_exit.r++;
            }
        }
        if(shortest_exit.c>0 && !isborder(map,shortest_exit.r,shortest_exit.c,LEFT_WALL)){
            int pos_next = cell_distances[index_2d_to_1d(shortest_exit.r, shortest_exit.c - 1, map->cols)];
            if(pos == pos_next + 1) {
                shortest_exit.c--;
            }
        }
        if(shortest_exit.c < map->cols && !isborder(map, shortest_exit.r, shortest_exit.c, RIGHT_WALL)){
            int pos_next = cell_distances[index_2d_to_1d(shortest_exit.r, shortest_exit.c + 1, map->cols)];
            if (pos == pos_next + 1) {
                shortest_exit.c++;
            }
        }
        // Ulozi souradnice pole do pole
        shortest_path[i] = (Position) {shortest_exit.r, shortest_exit.c};
    }
    return shortest_path;
}

// map (Map*) - Ukazatel na mapu
// cell_distances (int*) - pole vzdalenosti od vstupu
// --------------------------------------------------------
// return (Position) - souradnice nejblizsiho vychodu
Position find_nearest_exit(Map *map, const int *cell_destances) {
    Position shortest_exit;
    // Projde krajni pole bludiste a najde vychod s nejnizsii vzdalenosti od vstupu
    for (int i = 0; i < map->rows || i < map->cols; i++) {
        if(i==0){
            shortest_exit.r = 0;
            shortest_exit.c = 0;
        }
        if (i < map->rows) {
            if(cell_destances[index_2d_to_1d(i, 0, map->cols)] != 0 && !isborder(map, i, 0, LEFT_WALL)) {
                if (cell_destances[index_2d_to_1d(i, 0, map->cols)] <
                    cell_destances[index_2d_to_1d(shortest_exit.r, shortest_exit.c, map->cols)]) {
                    shortest_exit.r = i;
                    shortest_exit.c = 0;
                }
            }
            if(cell_destances[index_2d_to_1d(i, map->cols - 1, map->cols)] != 0 && !isborder(map, i, map->cols - 1, RIGHT_WALL)) {
                if (cell_destances[index_2d_to_1d(i, map->cols - 1, map->cols)] < cell_destances[index_2d_to_1d(shortest_exit.r, shortest_exit.c, map->cols)]) {
                    shortest_exit.r = i;
                    shortest_exit.c = map->cols - 1;
                }
            }
        }
        if (i < map->cols) {
            if(cell_destances[index_2d_to_1d(0, i, map->cols)] != 0 && !isborder(map, 0, i, HORIZONTAL_WALL) && has_upper_wall(0, i)) {
                if (cell_destances[index_2d_to_1d(0, i, map->cols)] < cell_destances[index_2d_to_1d(shortest_exit.r, shortest_exit.c, map->cols)]) {
                    shortest_exit.r = 0;
                    shortest_exit.c = i;
                }
            }
            if(cell_destances[index_2d_to_1d(map->rows - 1, i, map->cols)] != 0 && !isborder(map, map->rows - 1, i, HORIZONTAL_WALL) && !has_upper_wall(map->rows - 1, i)) {
                if (cell_destances[index_2d_to_1d(map->rows - 1, i, map->cols)] < cell_destances[index_2d_to_1d(shortest_exit.r, shortest_exit.c, map->cols)]) {
                    shortest_exit.r = map->rows - 1;
                    shortest_exit.c = i;
                }
            }
        }
    }
    return shortest_exit;
}

// map (Map*) - Ukazatel na mapu
// r (int*) - Index aktualniho radku
// c (int*) - Index aktualniho policka
// cell_distances (int*) - Pole vzdalenosti od vstupu (vraci se odkazem)
// index (int) - Vzdalenost od vstupu
void measure_distances(Map *map, int r, int c, int *cell_distances, int index) {
    // Prochazi vsechny pole na ktera lze vstoupit a nastavuje jejich vzdalenost od vstupu
    if (index < cell_distances[index_2d_to_1d(r, c, map->cols)]) {
        cell_distances[index_2d_to_1d(r, c, map->cols)] = index;
    }
    if ((r >= 0 && r < map->rows) && (c >= 0 && c < map->cols)) {
        if (!isborder(map, r, c, HORIZONTAL_WALL)) {
            if (has_upper_wall(r, c) && r > 0) {
                if (index < cell_distances[index_2d_to_1d(r - 1, c, map->cols)]) {
                    measure_distances(map, r - 1, c, cell_distances, index + 1);
                }
            }
            if (!has_upper_wall(r, c) && r < (map->rows - 1)) {
                if (index < cell_distances[index_2d_to_1d(r + 1, c, map->cols)]) {
                    measure_distances(map, r + 1, c, cell_distances, index + 1);
                }
            }
        }
        if (!isborder(map, r, c, LEFT_WALL) && c > 0) {
            if (index < cell_distances[index_2d_to_1d(r, c - 1, map->cols)]) {
                measure_distances(map, r, c - 1, cell_distances, index + 1);
            }
        }
        if (!isborder(map, r, c, RIGHT_WALL) && c < (map->cols - 1)) {
            if (index < cell_distances[index_2d_to_1d(r, c + 1, map->cols)]) {
                measure_distances(map, r, c + 1, cell_distances, index + 1);
            }
        }
    }
}

// map (Map*) - Ukazatel na mapu
// r (int) - Index radku
// c (int) - Index policka
// --------------------------------------------------------
// return (int) - 1 == leva | 2 == prava | 4 == horni/dolni
// ERROR return (int) - (-1) Do pole nelze vstoupit | (-2) Vstup se stredu nebo mimo bludiste
int start_border(Map *map, int r, int c) {
    int rows = map->rows - 1;
    int cols = map->cols - 1;
    // Zjisti zda jsou souradnice uvnitr bludiste
    if ((r == 0 && c < map->cols) || (r < map->rows && c == 0) || (r < map->rows && c < map->cols)) {
        // Zjisti zda jsou souradnice na okraji bludiste
        if (r == rows || c == cols || r == 0 || c == 0) {
            // Na levem okraji (leva stena musi byt pruchozi)
            if (c == 0 && !isborder(map, r, c, LEFT_WALL)) {
                return LEFT_WALL;
            }
            // Na pravem okraji (prava stena musi byt pruchozi)
            if (c == cols && !isborder(map, r, c, RIGHT_WALL)) {
                return RIGHT_WALL;
            }
            // Na hornim okraji (horni stena musi existoat a byt pruchozi)
            if (r == 0 && !isborder(map, r, c, HORIZONTAL_WALL) && has_upper_wall(r, c)) {
                return HORIZONTAL_WALL; // upper
            }
            // Na dolnim okraji (dolni stena musi musi existoat a byt pruchozi)
            if (r == rows && !isborder(map, r, c, HORIZONTAL_WALL) && !has_upper_wall(r, c)) {
                return HORIZONTAL_WALL; // lower
            }
            fprintf(stderr, "Na toto pole nelze vstoupit\n");
            return -1;
        }
    }
    fprintf(stderr, "Vstoupit lze pouze na okraji bludiste\n");
    return -2;
}

// map (Map*) - Ukazatel na mapu
// r (int*) - Ukazatel na index radku
// c (int*) - Ukazatel na index policka
// from (int) - hranice pres kterou do policka vstupujeme
// leftright (int) - metoda hledani cesty
// --------------------------------------------------------
// Vraci hodnotu reprezentujici stenu pres kterou vstupujeme do nasledujiciho pole
// return (int) - 1 == LEFT_WALL | 2 == RIGHT_WALL | 4 == HORIZONTAL_WALL
// ERROR return (int) - (-1) Do pole nelze vstoupit | (-2) Vstup se stredu nebo mimo bludiste
int pathfinder(Map *map, int *r, int *c, int from, int leftright) {
    int pos[] = {0, 0, 0};
    // Podle toho pres kterou stenu do bunky vstupujeme, nastavi prvky do pole v poradi ve kterem se maji steny testovat
    switch (from) {
        case LEFT_WALL:
            if ((has_upper_wall(*r, *c) && leftright == RIGHT_HAND_METHOD) ||
                (!has_upper_wall(*r, *c) && leftright == LEFT_HAND_METHOD)) {
                pos[0] = RIGHT_WALL;
                pos[1] = HORIZONTAL_WALL;
                pos[2] = LEFT_WALL;
            } else {
                pos[0] = HORIZONTAL_WALL;
                pos[1] = RIGHT_WALL;
                pos[2] = LEFT_WALL;
            }
            break;
        case RIGHT_WALL:
            if ((has_upper_wall(*r, *c) && leftright == RIGHT_HAND_METHOD) ||
                (!has_upper_wall(*r, *c) && leftright == LEFT_HAND_METHOD)) {
                pos[0] = HORIZONTAL_WALL;
                pos[1] = LEFT_WALL;
                pos[2] = RIGHT_WALL;
            } else {
                pos[0] = LEFT_WALL;
                pos[1] = HORIZONTAL_WALL;
                pos[2] = RIGHT_WALL;
            }
            break;
        case HORIZONTAL_WALL:
            if ((has_upper_wall(*r, *c) && leftright == RIGHT_HAND_METHOD) ||
                (!has_upper_wall(*r, *c) && leftright == LEFT_HAND_METHOD)) {
                pos[0] = LEFT_WALL;
                pos[1] = RIGHT_WALL;
                pos[2] = HORIZONTAL_WALL;
            } else {
                pos[0] = RIGHT_WALL;
                pos[1] = LEFT_WALL;
                pos[2] = HORIZONTAL_WALL;
            }
            break;
    }

    int go_through;
    // Zjisti zda je nasledujici stena pruchozi a podle toho zvoli skrz kterou hranici ma pokracovat
    if (isborder(map, *r, *c, pos[0])) {
        if (isborder(map, *r, *c, pos[1])) {
            if (isborder(map, *r, *c, pos[2])) {
                fprintf(stderr, "Nelze se hnout\n");
                return -1;
            } else {
                go_through = pos[2];
            }
        } else {
            go_through = pos[1];
        }
    } else {
        go_through = pos[0];
    }

    // Podle toho pres kterou stenu bunku opoustime, nastavi indexy na brati stenu vstupu nasledujici bunky
    switch (go_through) {
        case LEFT_WALL:
            (*c)--;
            return RIGHT_WALL;
        case RIGHT_WALL:
            (*c)++;
            return LEFT_WALL;
        case HORIZONTAL_WALL:
            if (has_upper_wall(*r, *c)) {
                (*r)--;
            } else {
                (*r)++;
            }
            return HORIZONTAL_WALL;
        default:
            fprintf(stderr, "Nelze se hnout\n");
            return -1;
    }
}

// r (int*) - Index radku 2D pole
// c (int*) - Index policka 2D pole
// --------------------------------------------------------
// return (int) - Index 1D pole
int index_2d_to_1d(int r, int c, int cols) {
    return r * cols + c;
}