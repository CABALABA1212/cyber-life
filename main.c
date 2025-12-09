
#include "raylib.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <stdio.h>
/* Параметры */

// Графика
#define WIDTH       1280
#define HEIGHT      720
#define SIZE        10
#define WIDTH_MAP   WIDTH / SIZE
#define HEIGHT_MAP  HEIGHT / SIZE
#define missing     5

// Энергия 
#define PHOTOSYNTHESIS_ENERGY   20
#define MIN_DIVISION_ENERGY     75
#define PERCENT_ATTACK          0.66
    
// Параметры мозга
#define INPUT_SIZE  9
#define HIDDEN_SIZE 10
#define OUTPUT_SIZE 6

/* Глобальные состояния */

char theme = 'C'; // Тема
unsigned int ticks = 0, // Счётчик тиков
             tps = 0,   // Количество тиков в секунду
             bots_count = 0; // Количество ботов на карте 
bool run = false;
int grid[HEIGHT_MAP][WIDTH_MAP];
/* Структуры */

// Типы специальных нейронов
typedef enum {
    SIMPLE,
    SENSITIVE,
    RANDOMISH
} NeuronType;

typedef struct {
    // Размеры храним для совместимости с логикой, но они константны
    int input_size;
    int hidden_size;
    int output_size;

    // Статические массивы вместо указателей
    float W1[INPUT_SIZE * HIDDEN_SIZE];
    float W2[HIDDEN_SIZE * OUTPUT_SIZE];
    
    float B1[HIDDEN_SIZE];
    float B2[OUTPUT_SIZE];
} Brain;

// Позиция
typedef struct {
    int x;
    int y;
} Pos;

// Структура бота
typedef struct {
    Pos pos;
    Color color;
    char energy;
    char rotation;
    Brain brain;
    char age;
} Bot;



/* Функции */

static inline int min(int a, int b) {
    return a < b ? a : b;
}
static inline int max(int a, int b) {
    return a > b ? a : b;
}

// Заполняет массив случайными числами или нулями
void randomize_array(float *arr, int size, bool randomize, float scale) {
    for (int i = 0; i < size; i++) {
        arr[i] = randomize ? ((float)rand() / RAND_MAX * 2 - 1) * scale : 0.0f;
    }
}

// Функция рандома в диапозоне
int randint(int min, int max) { 
    return (int)(rand() % (max - min + 1) + min);
}

static Bot *bots = NULL;



// Инит мозга
void init_brain(Brain *b) {
    b->input_size = INPUT_SIZE;
    b->hidden_size = HIDDEN_SIZE;
    b->output_size = OUTPUT_SIZE;

    // Инициализируем веса случайными значениями
    randomize_array(b->W1, INPUT_SIZE * HIDDEN_SIZE, true, 1.0f);
    randomize_array(b->W2, HIDDEN_SIZE * OUTPUT_SIZE, true, 1.0f);

    // Биасы инициализируем нулями
    randomize_array(b->B1, HIDDEN_SIZE, false, 0);
    randomize_array(b->B2, OUTPUT_SIZE, false, 0);
}


Brain mutate_brain(const Brain *b, float mutation_rate, float mutation_strength) {
    Brain nb = *b; 

    if ((float)rand() / RAND_MAX < 0.01f) { 
        init_brain(&nb);
        return nb;
    }

    // 2. Мутируем W1
    int w1_size = nb.input_size * nb.hidden_size;
    for (int i = 0; i < w1_size; i++) {
        if ((float)rand() / RAND_MAX < mutation_rate)
            nb.W1[i] += ((float)rand() / RAND_MAX * 2 - 1) * mutation_strength;
    }

    // 3. Мутируем W2
    int w2_size = nb.hidden_size * nb.output_size;
    for (int i = 0; i < w2_size; i++) {
        if ((float)rand() / RAND_MAX < mutation_rate)
            nb.W2[i] += ((float)rand() / RAND_MAX * 2 - 1) * mutation_strength;
    }

    // 4. Мутируем B1
    for (int i = 0; i < nb.hidden_size; i++) {
        if ((float)rand() / RAND_MAX < mutation_rate)
            nb.B1[i] += ((float)rand() / RAND_MAX * 2 - 1) * mutation_strength;
    }

    // 5. Мутируем B2
    for (int i = 0; i < nb.output_size; i++) {
        if ((float)rand() / RAND_MAX < mutation_rate)
            nb.B2[i] += ((float)rand() / RAND_MAX * 2 - 1) * mutation_strength;
    }

    return nb;
}


char think(const Brain *b, const float *inputs) {
    float hidden[HIDDEN_SIZE]; 
    float output[OUTPUT_SIZE];

    // INPUT → HIDDEN
    for (int j = 0; j < b->hidden_size; j++) {
        float sum = b->B1[j];
        for (int i = 0; i < b->input_size; i++) {
            sum += inputs[i] * b->W1[i * b->hidden_size + j];
        }
        hidden[j] = sum > 0 ? sum : 0; // ReLU
    }

    // HIDDEN → OUTPUT
    for (int j = 0; j < b->output_size; j++) {
        float sum = b->B2[j];
        for (int i = 0; i < b->hidden_size; i++) {
            sum += hidden[i] * b->W2[i * b->output_size + j];
        }
        output[j] = sum;
    }

    // Argmax первых 5
    int best = 0;
    float best_val = output[0];
    for (int i = 1; i < 5; i++) {
        if (output[i] > best_val) {
            best = i;
            best_val = output[i];
        }
    }

    return (char)best;
}


// Инит первых ботов
void init(int count) {
    bots = malloc(sizeof(Bot) * count);
    Pos occupied_pos[count];
    Color existint_colors[count];

    for (int i = 0; i < count; i++) {

        Pos pos;
    rerand:
        pos = (Pos){
            randint(0, WIDTH_MAP - 1),
            randint(0, HEIGHT_MAP - 1)
        };
        for (int j = 0; j < i; j++) {
            if (occupied_pos[j].x == pos.x &&
                occupied_pos[j].y == pos.y) {
                goto rerand;

            }
        }

        Color color;
    recolor:
        color = (Color){
            (unsigned char)randint(0, 255),
            (unsigned char)randint(0, 255),
            (unsigned char)randint(0, 255),
            255
        };

        for (int j = 0; j < i; j++) {
            if (existint_colors[j].r == color.r &&
                existint_colors[j].g == color.g &&
                existint_colors[j].b == color.b) {
                goto recolor;
            }
        }

        Brain brain;
        init_brain(&brain);

        bots[i].pos = pos;
        bots[i].color = color;
        bots[i].energy = 100;
        bots[i].rotation = (char)randint(0, 3);
        bots[i].brain = brain;
        bots[i].age = 0;

        occupied_pos[i] = pos;
        existint_colors[i] = color;
    }
    bots_count = count;
}

// Возвращает 0 и устанавливает *bot_out если найден; иначе -1
int get(Bot *bots_arr, Pos pos, Bot **bot_out) {
    for (size_t i = 0; i < bots_count; i++) {
        if (bots_arr[i].pos.x == pos.x && bots_arr[i].pos.y == pos.y) {
            *bot_out = &bots_arr[i];
            return 0;
        }
    }
    return -1;
}
// Вставка нового бота в конец массива
int append_bot(Bot **bots_ptr, Bot *newbot) {
    Bot *tmp = realloc(*bots_ptr, (bots_count + 1) * sizeof(Bot));
    if (!tmp) return -1;
    *bots_ptr = tmp;
    (*bots_ptr)[bots_count] = *newbot;
    bots_count++;
    return 0;
}


int step(Bot **bots_ptr) {
    Bot *bots_arr = *bots_ptr;  // локальная копия для удобства

    int dx[] = {0, 1, 0, -1};
    int dy[] = {-1, 0, 1, 0};

    for (size_t i = 0; i < bots_count; i++) {
        Bot *current = &bots_arr[i];

        // Куда смотрит бот сейчас
        Pos new_pos = {
            current->pos.x + dx[(unsigned char)current->rotation],
            current->pos.y + dy[(unsigned char)current->rotation]
        };

        // Проверяем, есть ли кто-то впереди (O(N) с Grid Map, O(N²) без него)
        bool has_target = false;
        bool is_same_team = false;
        Bot *target = NULL;

        if (new_pos.x >= 0 && new_pos.x < WIDTH_MAP &&
            new_pos.y >= 0 && new_pos.y < HEIGHT_MAP) {
            
            for (size_t j = 0; j < bots_count; j++) {
                if (bots_arr[j].pos.x == new_pos.x && bots_arr[j].pos.y == new_pos.y) {
                    target = &bots_arr[j];
                    has_target = true;
                    is_same_team = (current->color.r == target->color.r &&
                                    current->color.g == target->color.g &&
                                    current->color.b == target->color.b);
                    break;
                }
            }

        }

        // Входные данные для нейросети
        float inputs[INPUT_SIZE] = {0};
        inputs[0] = min((int)current->energy, 100) / 100.0f;
        inputs[1] = has_target ? 1.0f : 0.0f;
        inputs[2] = is_same_team ? 1.0f : 0.0f;
        inputs[3] = current->rotation / 3.0f;
        inputs[4] = (float)current->pos.y / HEIGHT_MAP;
        inputs[5] = (float)current->pos.x / WIDTH_MAP;
        inputs[6] = current->age / 100.0f;
        inputs[7] = (has_target && target != NULL) ? target->energy / 100.0f : 0.0f;
        inputs[8] = (has_target && target != NULL) ? target->age / 100.0f : 0.0f;


        char action = think(&current->brain, inputs);

        switch (action) {
            case 0: // поворот направо
                current->rotation = (current->rotation + 1) % 4;
                current->energy = max((int)current->energy - 1, 0);
                break;

            case 1: // шаг вперёд
                if (!has_target &&
                    new_pos.x >= 0 && new_pos.x < WIDTH_MAP &&
                    new_pos.y >= 0 && new_pos.y < HEIGHT_MAP) {
                    current->pos = new_pos;
                    current->energy = max((int)current->energy - 1, 0);
                }
                break;

            case 2: // фотосинтез
                current->energy = min((int)current->energy + PHOTOSYNTHESIS_ENERGY, 100);
                break;

            case 3: { // деление
                if (current->energy >= MIN_DIVISION_ENERGY &&
                    !has_target &&
                    new_pos.x >= 0 && new_pos.x < WIDTH_MAP &&
                    new_pos.y >= 0 && new_pos.y < HEIGHT_MAP) {

                    char parent_energy = current->energy / 2;
                    current->energy = parent_energy;

                    Bot *tmp = realloc(*bots_ptr, (bots_count + 1) * sizeof(Bot));
                    if (!tmp) {
                        current->energy += parent_energy; // откатываем
                        break;
                    }
                    *bots_ptr = tmp;
                    bots_arr = tmp;  
                    current = &bots_arr[i]; 

                    Bot *child = &bots_arr[bots_count];

                    child->pos = new_pos;
                    child->energy = parent_energy;
                    child->age = 0;
                    child->rotation = (current->rotation + randint(0, 3)) % 4;

                    if ((float)rand() / RAND_MAX < 0.01f) {
                        child->color = (Color){ randint(0,255), randint(0,255), randint(0,255), 255 };
                        Brain mutated = mutate_brain(&current->brain, 0.05f, 0.5f);
                        child->brain = mutated; 
                    } else {
                        child->color = current->color;
                        child->brain = current->brain; 
                    }

                    bots_count++;
                }
                break;
            }

            case 4: // атака
                if (has_target && target) {
                    int stolen = (int)(target->energy * PERCENT_ATTACK);
                    current->energy = min((int)current->energy + stolen, 100);
                    target->energy = max((int)target->energy - stolen, 0);
                }
                break;

            case 5: // поделиться энергией
                if (has_target && target) {
                    char half = current->energy / 2;
                    current->energy -= half;
                    target->energy = min((int)target->energy + half, 100);
                }
                break;
        }

        current->age++;
    }

    size_t alive = 0;
    for (size_t i = 0; i < bots_count; i++) {
        if (bots_arr[i].energy > 0 && bots_arr[i].age <= 100) {
            if (alive != i) {
                // Копирование структуры - Brain копируется вместе с Bot
                bots_arr[alive] = bots_arr[i]; 
            }
            alive++;
        } 
    }

    if (alive < bots_count) {
        if (alive == 0) {
            free(*bots_ptr);
            *bots_ptr = NULL;
        } else {
            Bot *tmp = realloc(*bots_ptr, alive * sizeof(Bot));
            if (tmp) *bots_ptr = tmp;
        }
        bots_count = alive;
    }

    return 0;
}




int main(void) {
    srand((unsigned int)time(NULL)); 

    InitWindow(WIDTH, HEIGHT, "Bots Simulation");

    // Инициализация ботов
    while (!WindowShouldClose()) {
        if (bots_count <= 0)
            init(100);
            
        // Логика ботов
        step(&bots);
        ticks++;
        
        // Рисуем только каждый 10-й тик (как и было)
        if (ticks % 10 == 0) {
            BeginDrawing();
            // Рисуем всё
            ClearBackground(BLACK);

            // Рисуем ботов
            for (size_t i = 0; i < bots_count; i++) {
                Bot *b = &bots[i];
                DrawRectangle(b->pos.x * SIZE, b->pos.y * SIZE, SIZE, SIZE, b->color);
            }
            EndDrawing();

        }
    }

    // Освобождаем память
    free(bots); 

    CloseWindow();
    return 0;
}
