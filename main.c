#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "raygui.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <uchar.h>
#define RAND_LIST_SIZE 16384
float rand_list[RAND_LIST_SIZE];
int rand_index = 0;

void init_rand_list() {
    for (int i = 0; i < RAND_LIST_SIZE; i++) {
        rand_list[i] = (float)rand() / RAND_MAX;
    }
}

float get_rand() {
    float r = rand_list[rand_index];
    rand_index = (rand_index + 1) % RAND_LIST_SIZE;
    return r;
}


/* Параметры */
#define SEED 0
// Графика
#define WIDTH 1280
#define WIDTH_board 0.2
#define HEIGHT 720
#define SIZE 10
#define WIDTH_MAP WIDTH / SIZE
#define HEIGHT_MAP HEIGHT / SIZE
#define PANEL_WIDTH (WIDTH * 0.2f)
#define WORLD_WIDTH (WIDTH - PANEL_WIDTH)
// Энергия
#define PHOTOSYNTHESIS_ENERGY 20
#define MIN_DIVISION_ENERGY 75
#define PERCENT_ATTACK 0.66
   
// Параметры мозга
#define INPUT_SIZE 9
#define HIDDEN_SIZE 10
#define OUTPUT_SIZE 6
/* Глобальные состояния */
unsigned int ticks = 0, // Счётчик тиков
             bots_count = 0; // Количество ботов на карте
size_t bots_capacity = 0;
int grid[HEIGHT_MAP][WIDTH_MAP];
/* Структуры */
// Типы специальных нейронов
typedef enum {
    SIMPLE,
    SENSITIVE,
    RANDOMISH
} NeuronType;
typedef struct {
    int input_size;
    int hidden_size;
    int output_size;
    
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
    unsigned char energy;
    unsigned char rotation;
    Brain brain;
    unsigned char age;
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
        arr[i] = randomize ? (get_rand() * 2 - 1) * scale : 0.0f;
    }
}
// Функция рандома в диапозоне
int randint(int min, int max) {
    return (int)(get_rand() * (max - min + 1)) + min;
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
    if (get_rand() < 0.01f) {
        init_brain(&nb);
        return nb;
    }
    // 2. Мутируем W1
    int w1_size = nb.input_size * nb.hidden_size;
    for (int i = 0; i < w1_size; i++) {
        if (get_rand() < mutation_rate)
            nb.W1[i] += (get_rand() * 2 - 1) * mutation_strength;
    }
    // 3. Мутируем W2
    int w2_size = nb.hidden_size * nb.output_size;
    for (int i = 0; i < w2_size; i++) {
        if (get_rand() < mutation_rate)
            nb.W2[i] += (get_rand() * 2 - 1) * mutation_strength;
    }
    // 4. Мутируем B1
    for (int i = 0; i < nb.hidden_size; i++) {
        if (get_rand() < mutation_rate)
            nb.B1[i] += (get_rand() * 2 - 1) * mutation_strength;
    }
    // 5. Мутируем B2
    for (int i = 0; i < nb.output_size; i++) {
        if (get_rand() < mutation_rate)
            nb.B2[i] += (get_rand() * 2 - 1) * mutation_strength;
    }
    return nb;
}
char think(const Brain *b, const float *inputs) {
    float hidden[HIDDEN_SIZE];
    float output[OUTPUT_SIZE];
    // INPUT → HIDDEN (unrolled)
    {
        float sum = b->B1[0];
        sum += inputs[0] * b->W1[0 * HIDDEN_SIZE + 0];
        sum += inputs[1] * b->W1[1 * HIDDEN_SIZE + 0];
        sum += inputs[2] * b->W1[2 * HIDDEN_SIZE + 0];
        sum += inputs[3] * b->W1[3 * HIDDEN_SIZE + 0];
        sum += inputs[4] * b->W1[4 * HIDDEN_SIZE + 0];
        sum += inputs[5] * b->W1[5 * HIDDEN_SIZE + 0];
        sum += inputs[6] * b->W1[6 * HIDDEN_SIZE + 0];
        sum += inputs[7] * b->W1[7 * HIDDEN_SIZE + 0];
        sum += inputs[8] * b->W1[8 * HIDDEN_SIZE + 0];
        hidden[0] = sum > 0 ? sum : 0;
    }
    {
        float sum = b->B1[1];
        sum += inputs[0] * b->W1[0 * HIDDEN_SIZE + 1];
        sum += inputs[1] * b->W1[1 * HIDDEN_SIZE + 1];
        sum += inputs[2] * b->W1[2 * HIDDEN_SIZE + 1];
        sum += inputs[3] * b->W1[3 * HIDDEN_SIZE + 1];
        sum += inputs[4] * b->W1[4 * HIDDEN_SIZE + 1];
        sum += inputs[5] * b->W1[5 * HIDDEN_SIZE + 1];
        sum += inputs[6] * b->W1[6 * HIDDEN_SIZE + 1];
        sum += inputs[7] * b->W1[7 * HIDDEN_SIZE + 1];
        sum += inputs[8] * b->W1[8 * HIDDEN_SIZE + 1];
        hidden[1] = sum > 0 ? sum : 0;
    }
    {
        float sum = b->B1[2];
        sum += inputs[0] * b->W1[0 * HIDDEN_SIZE + 2];
        sum += inputs[1] * b->W1[1 * HIDDEN_SIZE + 2];
        sum += inputs[2] * b->W1[2 * HIDDEN_SIZE + 2];
        sum += inputs[3] * b->W1[3 * HIDDEN_SIZE + 2];
        sum += inputs[4] * b->W1[4 * HIDDEN_SIZE + 2];
        sum += inputs[5] * b->W1[5 * HIDDEN_SIZE + 2];
        sum += inputs[6] * b->W1[6 * HIDDEN_SIZE + 2];
        sum += inputs[7] * b->W1[7 * HIDDEN_SIZE + 2];
        sum += inputs[8] * b->W1[8 * HIDDEN_SIZE + 2];
        hidden[2] = sum > 0 ? sum : 0;
    }
    {
        float sum = b->B1[3];
        sum += inputs[0] * b->W1[0 * HIDDEN_SIZE + 3];
        sum += inputs[1] * b->W1[1 * HIDDEN_SIZE + 3];
        sum += inputs[2] * b->W1[2 * HIDDEN_SIZE + 3];
        sum += inputs[3] * b->W1[3 * HIDDEN_SIZE + 3];
        sum += inputs[4] * b->W1[4 * HIDDEN_SIZE + 3];
        sum += inputs[5] * b->W1[5 * HIDDEN_SIZE + 3];
        sum += inputs[6] * b->W1[6 * HIDDEN_SIZE + 3];
        sum += inputs[7] * b->W1[7 * HIDDEN_SIZE + 3];
        sum += inputs[8] * b->W1[8 * HIDDEN_SIZE + 3];
        hidden[3] = sum > 0 ? sum : 0;
    }
    {
        float sum = b->B1[4];
        sum += inputs[0] * b->W1[0 * HIDDEN_SIZE + 4];
        sum += inputs[1] * b->W1[1 * HIDDEN_SIZE + 4];
        sum += inputs[2] * b->W1[2 * HIDDEN_SIZE + 4];
        sum += inputs[3] * b->W1[3 * HIDDEN_SIZE + 4];
        sum += inputs[4] * b->W1[4 * HIDDEN_SIZE + 4];
        sum += inputs[5] * b->W1[5 * HIDDEN_SIZE + 4];
        sum += inputs[6] * b->W1[6 * HIDDEN_SIZE + 4];
        sum += inputs[7] * b->W1[7 * HIDDEN_SIZE + 4];
        sum += inputs[8] * b->W1[8 * HIDDEN_SIZE + 4];
        hidden[4] = sum > 0 ? sum : 0;
    }
    {
        float sum = b->B1[5];
        sum += inputs[0] * b->W1[0 * HIDDEN_SIZE + 5];
        sum += inputs[1] * b->W1[1 * HIDDEN_SIZE + 5];
        sum += inputs[2] * b->W1[2 * HIDDEN_SIZE + 5];
        sum += inputs[3] * b->W1[3 * HIDDEN_SIZE + 5];
        sum += inputs[4] * b->W1[4 * HIDDEN_SIZE + 5];
        sum += inputs[5] * b->W1[5 * HIDDEN_SIZE + 5];
        sum += inputs[6] * b->W1[6 * HIDDEN_SIZE + 5];
        sum += inputs[7] * b->W1[7 * HIDDEN_SIZE + 5];
        sum += inputs[8] * b->W1[8 * HIDDEN_SIZE + 5];
        hidden[5] = sum > 0 ? sum : 0;
    }
    {
        float sum = b->B1[6];
        sum += inputs[0] * b->W1[0 * HIDDEN_SIZE + 6];
        sum += inputs[1] * b->W1[1 * HIDDEN_SIZE + 6];
        sum += inputs[2] * b->W1[2 * HIDDEN_SIZE + 6];
        sum += inputs[3] * b->W1[3 * HIDDEN_SIZE + 6];
        sum += inputs[4] * b->W1[4 * HIDDEN_SIZE + 6];
        sum += inputs[5] * b->W1[5 * HIDDEN_SIZE + 6];
        sum += inputs[6] * b->W1[6 * HIDDEN_SIZE + 6];
        sum += inputs[7] * b->W1[7 * HIDDEN_SIZE + 6];
        sum += inputs[8] * b->W1[8 * HIDDEN_SIZE + 6];
        hidden[6] = sum > 0 ? sum : 0;
    }
    {
        float sum = b->B1[7];
        sum += inputs[0] * b->W1[0 * HIDDEN_SIZE + 7];
        sum += inputs[1] * b->W1[1 * HIDDEN_SIZE + 7];
        sum += inputs[2] * b->W1[2 * HIDDEN_SIZE + 7];
        sum += inputs[3] * b->W1[3 * HIDDEN_SIZE + 7];
        sum += inputs[4] * b->W1[4 * HIDDEN_SIZE + 7];
        sum += inputs[5] * b->W1[5 * HIDDEN_SIZE + 7];
        sum += inputs[6] * b->W1[6 * HIDDEN_SIZE + 7];
        sum += inputs[7] * b->W1[7 * HIDDEN_SIZE + 7];
        sum += inputs[8] * b->W1[8 * HIDDEN_SIZE + 7];
        hidden[7] = sum > 0 ? sum : 0;
    }
    {
        float sum = b->B1[8];
        sum += inputs[0] * b->W1[0 * HIDDEN_SIZE + 8];
        sum += inputs[1] * b->W1[1 * HIDDEN_SIZE + 8];
        sum += inputs[2] * b->W1[2 * HIDDEN_SIZE + 8];
        sum += inputs[3] * b->W1[3 * HIDDEN_SIZE + 8];
        sum += inputs[4] * b->W1[4 * HIDDEN_SIZE + 8];
        sum += inputs[5] * b->W1[5 * HIDDEN_SIZE + 8];
        sum += inputs[6] * b->W1[6 * HIDDEN_SIZE + 8];
        sum += inputs[7] * b->W1[7 * HIDDEN_SIZE + 8];
        sum += inputs[8] * b->W1[8 * HIDDEN_SIZE + 8];
        hidden[8] = sum > 0 ? sum : 0;
    }
    {
        float sum = b->B1[9];
        sum += inputs[0] * b->W1[0 * HIDDEN_SIZE + 9];
        sum += inputs[1] * b->W1[1 * HIDDEN_SIZE + 9];
        sum += inputs[2] * b->W1[2 * HIDDEN_SIZE + 9];
        sum += inputs[3] * b->W1[3 * HIDDEN_SIZE + 9];
        sum += inputs[4] * b->W1[4 * HIDDEN_SIZE + 9];
        sum += inputs[5] * b->W1[5 * HIDDEN_SIZE + 9];
        sum += inputs[6] * b->W1[6 * HIDDEN_SIZE + 9];
        sum += inputs[7] * b->W1[7 * HIDDEN_SIZE + 9];
        sum += inputs[8] * b->W1[8 * HIDDEN_SIZE + 9];
        hidden[9] = sum > 0 ? sum : 0;
    }
    // HIDDEN → OUTPUT (unrolled)
    {
        float sum = b->B2[0];
        sum += hidden[0] * b->W2[0 * OUTPUT_SIZE + 0];
        sum += hidden[1] * b->W2[1 * OUTPUT_SIZE + 0];
        sum += hidden[2] * b->W2[2 * OUTPUT_SIZE + 0];
        sum += hidden[3] * b->W2[3 * OUTPUT_SIZE + 0];
        sum += hidden[4] * b->W2[4 * OUTPUT_SIZE + 0];
        sum += hidden[5] * b->W2[5 * OUTPUT_SIZE + 0];
        sum += hidden[6] * b->W2[6 * OUTPUT_SIZE + 0];
        sum += hidden[7] * b->W2[7 * OUTPUT_SIZE + 0];
        sum += hidden[8] * b->W2[8 * OUTPUT_SIZE + 0];
        sum += hidden[9] * b->W2[9 * OUTPUT_SIZE + 0];
        output[0] = sum;
    }
    {
        float sum = b->B2[1];
        sum += hidden[0] * b->W2[0 * OUTPUT_SIZE + 1];
        sum += hidden[1] * b->W2[1 * OUTPUT_SIZE + 1];
        sum += hidden[2] * b->W2[2 * OUTPUT_SIZE + 1];
        sum += hidden[3] * b->W2[3 * OUTPUT_SIZE + 1];
        sum += hidden[4] * b->W2[4 * OUTPUT_SIZE + 1];
        sum += hidden[5] * b->W2[5 * OUTPUT_SIZE + 1];
        sum += hidden[6] * b->W2[6 * OUTPUT_SIZE + 1];
        sum += hidden[7] * b->W2[7 * OUTPUT_SIZE + 1];
        sum += hidden[8] * b->W2[8 * OUTPUT_SIZE + 1];
        sum += hidden[9] * b->W2[9 * OUTPUT_SIZE + 1];
        output[1] = sum;
    }
    {
        float sum = b->B2[2];
        sum += hidden[0] * b->W2[0 * OUTPUT_SIZE + 2];
        sum += hidden[1] * b->W2[1 * OUTPUT_SIZE + 2];
        sum += hidden[2] * b->W2[2 * OUTPUT_SIZE + 2];
        sum += hidden[3] * b->W2[3 * OUTPUT_SIZE + 2];
        sum += hidden[4] * b->W2[4 * OUTPUT_SIZE + 2];
        sum += hidden[5] * b->W2[5 * OUTPUT_SIZE + 2];
        sum += hidden[6] * b->W2[6 * OUTPUT_SIZE + 2];
        sum += hidden[7] * b->W2[7 * OUTPUT_SIZE + 2];
        sum += hidden[8] * b->W2[8 * OUTPUT_SIZE + 2];
        sum += hidden[9] * b->W2[9 * OUTPUT_SIZE + 2];
        output[2] = sum;
    }
    {
        float sum = b->B2[3];
        sum += hidden[0] * b->W2[0 * OUTPUT_SIZE + 3];
        sum += hidden[1] * b->W2[1 * OUTPUT_SIZE + 3];
        sum += hidden[2] * b->W2[2 * OUTPUT_SIZE + 3];
        sum += hidden[3] * b->W2[3 * OUTPUT_SIZE + 3];
        sum += hidden[4] * b->W2[4 * OUTPUT_SIZE + 3];
        sum += hidden[5] * b->W2[5 * OUTPUT_SIZE + 3];
        sum += hidden[6] * b->W2[6 * OUTPUT_SIZE + 3];
        sum += hidden[7] * b->W2[7 * OUTPUT_SIZE + 3];
        sum += hidden[8] * b->W2[8 * OUTPUT_SIZE + 3];
        sum += hidden[9] * b->W2[9 * OUTPUT_SIZE + 3];
        output[3] = sum;
    }
    {
        float sum = b->B2[4];
        sum += hidden[0] * b->W2[0 * OUTPUT_SIZE + 4];
        sum += hidden[1] * b->W2[1 * OUTPUT_SIZE + 4];
        sum += hidden[2] * b->W2[2 * OUTPUT_SIZE + 4];
        sum += hidden[3] * b->W2[3 * OUTPUT_SIZE + 4];
        sum += hidden[4] * b->W2[4 * OUTPUT_SIZE + 4];
        sum += hidden[5] * b->W2[5 * OUTPUT_SIZE + 4];
        sum += hidden[6] * b->W2[6 * OUTPUT_SIZE + 4];
        sum += hidden[7] * b->W2[7 * OUTPUT_SIZE + 4];
        sum += hidden[8] * b->W2[8 * OUTPUT_SIZE + 4];
        sum += hidden[9] * b->W2[9 * OUTPUT_SIZE + 4];
        output[4] = sum;
    }
    {
        float sum = b->B2[5];
        sum += hidden[0] * b->W2[0 * OUTPUT_SIZE + 5];
        sum += hidden[1] * b->W2[1 * OUTPUT_SIZE + 5];
        sum += hidden[2] * b->W2[2 * OUTPUT_SIZE + 5];
        sum += hidden[3] * b->W2[3 * OUTPUT_SIZE + 5];
        sum += hidden[4] * b->W2[4 * OUTPUT_SIZE + 5];
        sum += hidden[5] * b->W2[5 * OUTPUT_SIZE + 5];
        sum += hidden[6] * b->W2[6 * OUTPUT_SIZE + 5];
        sum += hidden[7] * b->W2[7 * OUTPUT_SIZE + 5];
        sum += hidden[8] * b->W2[8 * OUTPUT_SIZE + 5];
        sum += hidden[9] * b->W2[9 * OUTPUT_SIZE + 5];
        output[5] = sum;
    }
    // Argmax
    int best = 0;
    float best_val = output[0];
    for (int i = 1; i < OUTPUT_SIZE; i++) {
        if (output[i] > best_val) {
            best = i;
            best_val = output[i];
        }
    }
    return (char)best;
}
// Инит первых ботов
void init(int count) {
    if (bots_capacity < count) {
        bots_capacity = count * 2;
        bots = malloc(bots_capacity * sizeof(Bot));
    }
    // Для позиций: собираем все возможные и шаффлим
    const int map_size = WIDTH_MAP * HEIGHT_MAP;
    Pos all_pos[map_size];
    for (int y = 0; y < HEIGHT_MAP; y++) {
        for (int x = 0; x < WIDTH_MAP; x++) {
            all_pos[y * WIDTH_MAP + x] = (Pos){x, y};
        }
    }
    for (int i = map_size - 1; i > 0; i--) {
        int j = randint(0, i);
        Pos tmp = all_pos[i];
        all_pos[i] = all_pos[j];
        all_pos[j] = tmp;
    }
    for (int i = 0; i < count; i++) {
        Pos pos = all_pos[i];
        Color color = (Color){
            (unsigned char)randint(0, 255),
            (unsigned char)randint(0, 255),
            (unsigned char)randint(0, 255),
            255
        };
        Brain brain;
        init_brain(&brain);
        bots[i].pos = pos;
        bots[i].color = color;
        bots[i].energy = 100;
        bots[i].rotation = (unsigned char)randint(0, 3);
        bots[i].brain = brain;
        bots[i].age = 0;
    }
    bots_count = count;
}
int step(Bot **bots_ptr) {
    Bot *bots_arr = *bots_ptr; // локальная копия для удобства
    memset(grid, -1, sizeof(grid));
    for (size_t i = 0; i < bots_count; i++) {
        Pos p = bots_arr[i].pos;
        grid[p.y][p.x] = (int)i;
    }
    int dx[] = {0, 1, 0, -1};
    int dy[] = {-1, 0, 1, 0};
    size_t initial_bots_count = bots_count; // Чтобы не обрабатывать новых ботов в этом тике
    for (size_t i = 0; i < initial_bots_count; i++) {
        Bot *current = &bots_arr[i];
        // Куда смотрит бот сейчас
        Pos new_pos = {
            current->pos.x + dx[current->rotation],
            current->pos.y + dy[current->rotation]
        };
        // Проверяем, есть ли кто-то впереди
        bool has_target = false;
        bool is_same_team = false;
        Bot *target = NULL;
        if (new_pos.x >= 0 && new_pos.x < WIDTH_MAP &&
            new_pos.y >= 0 && new_pos.y < HEIGHT_MAP) {
            int idx = grid[new_pos.y][new_pos.x];
            if (idx != -1) {
                has_target = true;
                target = &bots_arr[idx];
                is_same_team = (current->color.r == target->color.r &&
                                current->color.g == target->color.g &&
                                current->color.b == target->color.b);
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
        inputs[7] = has_target ? target->energy / 100.0f : 0.0f;
        inputs[8] = has_target ? target->age / 100.0f : 0.0f;
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
                    grid[current->pos.y][current->pos.x] = -1;
                    current->pos = new_pos;
                    grid[new_pos.y][new_pos.x] = (int)i;
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
                    unsigned char parent_energy = current->energy / 2;
                    current->energy = parent_energy;
                    if (bots_count >= bots_capacity) {
                        bots_capacity *= 2;
                        Bot *tmp = realloc(*bots_ptr, bots_capacity * sizeof(Bot));
                        if (!tmp) {
                            current->energy += parent_energy; // откатываем
                            break;
                        }
                        *bots_ptr = tmp;
                        bots_arr = tmp;
                        current = &bots_arr[i];
                    }
                    int child_idx = bots_count;
                    Bot *child = &bots_arr[child_idx];
                    child->pos = new_pos;
                    child->energy = parent_energy;
                    child->age = 0;
                    child->rotation = (current->rotation + randint(0, 3)) % 4;
                    if (get_rand() < 0.01f) {
                        child->color = (Color){ (unsigned char)randint(0,255), (unsigned char)randint(0,255), (unsigned char)randint(0,255), 255 };
                        Brain mutated = mutate_brain(&current->brain, 0.05f, 0.5f);
                        child->brain = mutated;
                    } else {
                        child->color = current->color;
                        child->brain = current->brain;
                    }
                    grid[new_pos.y][new_pos.x] = child_idx;
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
                    unsigned char half = current->energy / 2;
                    current->energy -= half;
                    target->energy = min((int)target->energy + half, 100);
                }
                break;
        }
        current->age = min(current->age + 1, 100);
    }
    size_t alive = 0;
    for (size_t i = 0; i < bots_count; i++) {
        if (bots_arr[i].energy > 0 && bots_arr[i].age < 100) {  
            if (alive != i) {
                bots_arr[alive] = bots_arr[i];
                Pos p = bots_arr[alive].pos;
                grid[p.y][p.x] = alive;
            }
            alive++;
        }
    }
    bots_count = alive;
    if (bots_count < bots_capacity / 4 && bots_capacity > 100) {
        bots_capacity /= 2;
        Bot *tmp = realloc(*bots_ptr, bots_capacity * sizeof(Bot));
        if (tmp) *bots_ptr = tmp;
    }
    return 0;
}

int main(void) {
    bool button_press = false;
    int seed = time(NULL);
    char buffer[16];
restart: 
    sprintf(buffer, "%d", seed);
    ticks = 0;
    bots_count = 0;
    bots_capacity = 0;   
    rand_index = 0;
    srand(ticks + seed);
    init_rand_list();

    const int PANEL_W = WIDTH * WIDTH_board;
    const int WORLD_W = WIDTH;

    InitWindow(WIDTH + PANEL_W, HEIGHT, "Bots Simulation");
    SetTargetFPS(60);
    ClearBackground(LIGHTGRAY);

    // --- Панель ---
    RenderTexture2D panel = LoadRenderTexture(PANEL_W, HEIGHT);
    BeginTextureMode(panel);
        ClearBackground((Color){40, 40, 40, 255});
        DrawText("Simulation", 20, 20, 24, RAYWHITE);
        DrawRectangle(10, 50, PANEL_W - 20, 3, LIGHTGRAY);
    EndTextureMode();

    // --- Основной цикл ---
    while (!WindowShouldClose()) {
        if (bots_count <= 0) init(100);

        step(&bots);
        ticks++;

        // Рисуем только каждый 10-й тик 
        if (ticks % 10 == 0) {
            BeginDrawing();
                // рисуем фон мира
                DrawRectangle(0, 0, WORLD_W, HEIGHT, BLACK);

                // рисуем ботов
                for (size_t i = 0; i < bots_count; i++) {
                    Bot *b = &bots[i];
                    DrawRectangle(b->pos.x * SIZE, b->pos.y * SIZE, SIZE, SIZE, b->color);
                }

                DrawTextureRec(panel.texture,
                               (Rectangle){0, 0, panel.texture.width, -panel.texture.height},
                               (Vector2){WORLD_W, 0},
                               WHITE);

                DrawText(TextFormat("Ticks: %zu", ticks), WORLD_W + 20, 63, 20, YELLOW);
                DrawText(TextFormat("Bots: %zu", bots_count), WORLD_W + 20, 93, 20, YELLOW);
                
                if (GuiTextBox((Rectangle){WORLD_W + 10, 123, PANEL_W - 70, 30}, buffer, 64, true)) {}

                if (GuiTextBox((Rectangle){WORLD_W+PANEL_W-60, 123, 30, 30}, ">", 20, false)){
                    seed = atoi(buffer);
                    goto restart;
                }
            EndDrawing();
        }
    }

    UnloadRenderTexture(panel);
    free(bots);
    CloseWindow();
    return 0;
}
