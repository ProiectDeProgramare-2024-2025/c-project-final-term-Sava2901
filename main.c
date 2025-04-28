#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include <string.h>
#include "cJSON.h"

#define COLOR_RESET   "\033[0m"
#define COLOR_NAME    "\033[36m"   // Cyan
#define COLOR_SCORE   "\033[32m"   // Green
#define COLOR_TIME    "\033[33m"   // Yellow
#define COLOR_OPTION  "\033[35m"   // Magenta

typedef struct {
    char name[100];
    int score;
    time_t timestamp;
} PlayerHistory;

void main_menu();
void play_game();
void leaderboard_menu();
void match_history_menu();
void clearScreen();
char* readFile(const char *filename);
void display_question(const cJSON *question_obj, char highlight, int lifeline_used, char remaining_options[], int *remaining_count);
void update_leaderboard(const char *name, int score);
void save_match_history(const char *name, int score);
void show_player_history(const char *search_name);

int main() {
    srand((unsigned)time(NULL));
    main_menu();
    return 0;
}

void clearScreen() {
    system("cls");
}

char* readFile(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);
    char* data = (char*)malloc(len + 1);
    if (data) {
        fread(data, 1, len, fp);
        data[len] = '\0';
    }
    fclose(fp);
    return data;
}

void display_question(const cJSON *question_obj, char highlight, int lifeline_used, char remaining_options[], int *remaining_count) {
    const cJSON *question_text = cJSON_GetObjectItemCaseSensitive(question_obj, "question");
    const cJSON *options = cJSON_GetObjectItemCaseSensitive(question_obj, "options");
    const cJSON *correct_answer = cJSON_GetObjectItemCaseSensitive(question_obj, "correct_answer");

    clearScreen();
    printf("Who Wants to Be a Millionaire - Play Game\n\n");
    if (cJSON_IsString(question_text) && question_text->valuestring != NULL) {
        printf("Question: %s\n\n", question_text->valuestring);
    }

    const char option_labels[] = {'A', 'B', 'C', 'D'};
    *remaining_count = 0;

    if (lifeline_used) {
        for (int i = 0; i < 4; i++) {
            char option_label[2] = {option_labels[i], '\0'};
            const cJSON *option = cJSON_GetObjectItemCaseSensitive(options, option_label);
            if (cJSON_IsString(option) && option->valuestring) {
                if (option_labels[i] == correct_answer->valuestring[0] || (rand() % 2 == 0)) {
                    if (*remaining_count < 2) {
                        remaining_options[(*remaining_count)++] = option_labels[i];
                    }
                } else if (*remaining_count < 2) {
                    remaining_options[(*remaining_count)++] = option_labels[i];
                }
                if (*remaining_count == 2) break;
            }
        }
    } else {
        *remaining_count = 4;
        for (int i = 0; i < 4; i++) {
            remaining_options[i] = option_labels[i];
        }
    }

    if (highlight != 'A' && highlight != 'B' && highlight != 'C' && highlight != 'D') {
        highlight = remaining_options[0];
    }

    for (int i = 0; i < *remaining_count; i++) {
        char option_label[2] = {remaining_options[i], '\0'};
        const cJSON *option = cJSON_GetObjectItemCaseSensitive(options, option_label);
        if (cJSON_IsString(option) && option->valuestring) {
            if (remaining_options[i] == highlight) {
                printf("%s>> %c) %s%s\n", COLOR_OPTION, remaining_options[i], option->valuestring, COLOR_RESET);
            } else {
                printf("   %c) %s\n", remaining_options[i], option->valuestring);
            }
        }
    }
}

void update_leaderboard(const char *name, int score) {
    cJSON *json = NULL;
    cJSON *leaderboard = NULL;
    char *json_data = readFile("leaderboard.json");

    if (json_data) {
        json = cJSON_Parse(json_data);
        free(json_data);
    }

    if (!json) {
        json = cJSON_CreateObject();
        leaderboard = cJSON_CreateArray();
        cJSON_AddItemToObject(json, "leaderboard", leaderboard);
    } else {
        leaderboard = cJSON_GetObjectItemCaseSensitive(json, "leaderboard");
        if (!cJSON_IsArray(leaderboard)) {
            cJSON_Delete(json);
            json = cJSON_CreateObject();
            leaderboard = cJSON_CreateArray();
            cJSON_AddItemToObject(json, "leaderboard", leaderboard);
        }
    }

    cJSON *new_entry = cJSON_CreateObject();
    cJSON_AddStringToObject(new_entry, "name", name);
    cJSON_AddNumberToObject(new_entry, "score", score);
    cJSON_AddNumberToObject(new_entry, "timestamp", (double)time(NULL));
    cJSON_AddItemToArray(leaderboard, new_entry);

    int leaderboard_size = cJSON_GetArraySize(leaderboard);
    for (int i = 0; i < leaderboard_size - 1; i++) {
        for (int j = i + 1; j < leaderboard_size; j++) {
            cJSON *entry1 = cJSON_GetArrayItem(leaderboard, i);
            cJSON *entry2 = cJSON_GetArrayItem(leaderboard, j);
            int score1 = cJSON_GetObjectItemCaseSensitive(entry1, "score")->valueint;
            int score2 = cJSON_GetObjectItemCaseSensitive(entry2, "score")->valueint;
            double timestamp1 = cJSON_GetObjectItemCaseSensitive(entry1, "timestamp")->valuedouble;
            double timestamp2 = cJSON_GetObjectItemCaseSensitive(entry2, "timestamp")->valuedouble;

            if (score2 > score1 || (score2 == score1 && timestamp2 < timestamp1)) {
                cJSON *temp = cJSON_Duplicate(entry1, 1);
                cJSON_ReplaceItemInArray(leaderboard, i, cJSON_Duplicate(entry2, 1));
                cJSON_ReplaceItemInArray(leaderboard, j, temp);
            }
        }
    }

    if (leaderboard_size > 10) {
        for (int i = leaderboard_size - 1; i >= 10; i--) {
            cJSON_DeleteItemFromArray(leaderboard, i);
        }
    }

    char *updated_json = cJSON_Print(json);
    FILE *fp = fopen("leaderboard.json", "w");
    if (fp) {
        fprintf(fp, "%s", updated_json);
        fclose(fp);
    }
    free(updated_json);
    cJSON_Delete(json);
}

void leaderboard_menu() {
    clearScreen();
    printf("Who Wants to Be a Millionaire - Leaderboard\n\n");

    char *json_data = readFile("leaderboard.json");
    if (!json_data) {
        printf("No leaderboard data found.\n");
        printf("Press any key to return to the Main Menu.\n");
        _getch();
        return;
    }

    cJSON *json = cJSON_Parse(json_data);
    free(json_data);
    if (!json) {
        printf("Error parsing leaderboard data.\n");
        printf("Press any key to return to the Main Menu.\n");
        _getch();
        return;
    }

    const cJSON *leaderboard = cJSON_GetObjectItemCaseSensitive(json, "leaderboard");
    if (cJSON_IsArray(leaderboard)) {
        int count = cJSON_GetArraySize(leaderboard);
        for (int i = 0; i < count; i++) {
            const cJSON *entry = cJSON_GetArrayItem(leaderboard, i);
            const cJSON *name = cJSON_GetObjectItemCaseSensitive(entry, "name");
            const cJSON *score = cJSON_GetObjectItemCaseSensitive(entry, "score");
            printf("%d. %s%-20s%s - %s%4d pts%s\n",
                i + 1,
                COLOR_NAME, name->valuestring, COLOR_RESET,
                COLOR_SCORE, score->valueint, COLOR_RESET
            );
        }
    }

    printf("\nPress any key to return to the Main Menu.\n");
    _getch();
    cJSON_Delete(json);
}

void save_match_history(const char *name, int score) {
    cJSON *json = NULL;
    cJSON *history = NULL;
    char *json_data = readFile("match_history.json");

    if (json_data) {
        json = cJSON_Parse(json_data);
        free(json_data);
    }

    if (!json) {
        json = cJSON_CreateObject();
        history = cJSON_CreateArray();
        cJSON_AddItemToObject(json, "history", history);
    } else {
        history = cJSON_GetObjectItemCaseSensitive(json, "history");
        if (!cJSON_IsArray(history)) {
            cJSON_Delete(json);
            json = cJSON_CreateObject();
            history = cJSON_CreateArray();
            cJSON_AddItemToObject(json, "history", history);
        }
    }

    cJSON *new_entry = cJSON_CreateObject();
    cJSON_AddStringToObject(new_entry, "name", name);
    cJSON_AddNumberToObject(new_entry, "score", score);
    cJSON_AddNumberToObject(new_entry, "timestamp", (double)time(NULL));
    cJSON_AddItemToArray(history, new_entry);

    int history_size = cJSON_GetArraySize(history);
    if (history_size > 100) {
        for (int i = 0; i < history_size - 100; i++) {
            cJSON_DeleteItemFromArray(history, 0);
        }
    }

    char *updated_json = cJSON_Print(json);
    FILE *fp = fopen("match_history.json", "w");
    if (fp) {
        fprintf(fp, "%s", updated_json);
        fclose(fp);
    }
    free(updated_json);
    cJSON_Delete(json);
}

void show_player_history(const char *search_name) {
    clearScreen();
    printf("Match History for: %s\n\n", search_name);

    char *json_data = readFile("match_history.json");
    if (!json_data) {
        printf("No match history found.\n");
        printf("Press any key to return to Match History.\n");
        _getch();
        return;
    }

    cJSON *json = cJSON_Parse(json_data);
    free(json_data);
    if (!json) {
        printf("Error parsing match history data.\n");
        printf("Press any key to return to Match History.\n");
        _getch();
        return;
    }

    const cJSON *history = cJSON_GetObjectItemCaseSensitive(json, "history");
    if (!cJSON_IsArray(history)) {
        printf("Invalid history format.\n");
        printf("Press any key to return to Match History.\n");
        _getch();
        cJSON_Delete(json);
        return;
    }

    int count = cJSON_GetArraySize(history);
    int matches_found = 0;

    for (int i = count - 1; i >= 0 && matches_found < 100; i--) {
        const cJSON *entry = cJSON_GetArrayItem(history, i);
        const cJSON *name = cJSON_GetObjectItemCaseSensitive(entry, "name");
        const cJSON *score = cJSON_GetObjectItemCaseSensitive(entry, "score");
        const cJSON *timestamp = cJSON_GetObjectItemCaseSensitive(entry, "timestamp");

        if (name && score && timestamp &&
            strcasecmp(name->valuestring, search_name) == 0) {

            time_t ts = (time_t)timestamp->valuedouble;
            struct tm *timeinfo = localtime(&ts);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);

            printf("%s%-20s%s - %s%4d pts%s at %s%s%s\n",
                COLOR_NAME, name->valuestring, COLOR_RESET,
                COLOR_SCORE, score->valueint, COLOR_RESET,
                COLOR_TIME, time_str, COLOR_RESET
            );

            matches_found++;
        }
    }

    if (matches_found == 0) {
        printf("No matches found for this player.\n");
    }

    printf("\nPress any key to return to Match History.\n");
    _getch();
    cJSON_Delete(json);
}

void match_history_menu() {
    int choice = 0;
    int highlight = 0;
    const char *options[] = {"View all match history", "Search for a player", "Return to Main Menu"};
    const int n_options = sizeof(options) / sizeof(options[0]);
    char search_name[100] = {0};

    while (1) {
        clearScreen();
        printf("Who Wants to Be a Millionaire - Match History\n\n");
        for (int i = 0; i < n_options; i++) {
            if (i == highlight) {
                printf(">> %s\n", options[i]);
            } else {
                printf("   %s\n", options[i]);
            }
        }
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            ch = _getch();
            if (ch == 72) {
                highlight--;
                if (highlight < 0)
                    highlight = n_options - 1;
            } else if (ch == 80) {
                highlight++;
                if (highlight >= n_options)
                    highlight = 0;
            }
        } else if (ch == 13) {
            choice = highlight;
            switch (choice) {
                case 0: {
                    clearScreen();
                    char *json_data = readFile("match_history.json");
                    if (!json_data) {
                        printf("No match history found.\n");
                        printf("Press any key to continue.\n");
                        _getch();
                        break;
                    }

                    cJSON *json = cJSON_Parse(json_data);
                    free(json_data);
                    if (!json) {
                        printf("Error parsing match history data.\n");
                        printf("Press any key to continue.\n");
                        _getch();
                        break;
                    }

                    const cJSON *history = cJSON_GetObjectItemCaseSensitive(json, "history");
                    if (cJSON_IsArray(history)) {
                        printf("Who Wants to Be a Millionaire - Match History\n\n");
                        int count = cJSON_GetArraySize(history);
                        int start = count > 100 ? count - 100 : 0;

                        for (int i = start; i < count; i++) {
                            const cJSON *entry = cJSON_GetArrayItem(history, i);
                            const cJSON *name = cJSON_GetObjectItemCaseSensitive(entry, "name");
                            const cJSON *score = cJSON_GetObjectItemCaseSensitive(entry, "score");
                            const cJSON *timestamp = cJSON_GetObjectItemCaseSensitive(entry, "timestamp");

                            time_t ts = (time_t)timestamp->valuedouble;
                            struct tm *timeinfo = localtime(&ts);
                            char time_str[20];
                            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);

                            printf("%s%-20s%s - %s%4d pts%s at %s%s%s\n",
                                COLOR_NAME, name->valuestring, COLOR_RESET,
                                COLOR_SCORE, score->valueint, COLOR_RESET,
                                COLOR_TIME, time_str, COLOR_RESET
                            );
                        }
                    }

                    printf("\nPress any key to continue.\n");
                    _getch();
                    cJSON_Delete(json);
                    break;
                }
                case 1: { // Search for a player
                    clearScreen();
                    printf("Enter player name to search: ");
                    fgets(search_name, sizeof(search_name), stdin);
                    search_name[strcspn(search_name, "\n")] = '\0';

                    if (strlen(search_name) > 0) {
                        show_player_history(search_name);
                    }
                    break;
                }
                case 2: // Return to Main Menu
                    return;
            }
        }
    }
}

void play_game() {
    clearScreen();

    char *json_data = readFile("questions.json");
    if (!json_data) {
        printf("Error reading questions.json file.\n");
        printf("Press any key to return to the Main Menu.\n");
        _getch();
        return;
    }

    cJSON *json = cJSON_Parse(json_data);
    free(json_data);
    if (!json) {
        printf("Error parsing JSON data.\n");
        printf("Press any key to return to the Main Menu.\n");
        _getch();
        return;
    }

    const cJSON *questions = cJSON_GetObjectItemCaseSensitive(json, "questions");
    if (!cJSON_IsArray(questions)) {
        printf("Questions not found or not in array format.\n");
        cJSON_Delete(json);
        printf("Press any key to return to the Main Menu.\n");
        _getch();
        return;
    }

    const int count = cJSON_GetArraySize(questions);
    if (count == 0) {
        printf("No questions available in the JSON file.\n");
        cJSON_Delete(json);
        printf("Press any key to return to the Main Menu.\n");
        _getch();
        return;
    }

    int lifeline_used = 0;
    char highlight = 'A';
    int score = 0;

    for (int idx = 0; idx < count; idx++) {
        const cJSON *question_obj = cJSON_GetArrayItem(questions, idx);
        const cJSON *correct_answer = cJSON_GetObjectItemCaseSensitive(question_obj, "correct_answer");

        int current_lifeline_used = 0;
        int remaining_count = 0;

        while (1) {
            char remaining_options[4];
            display_question(question_obj, highlight, current_lifeline_used, remaining_options, &remaining_count);

            printf("\nUse arrow keys to navigate and press Enter to select an answer.\n");
            if (!lifeline_used) {
                printf("Press 'f' for 50/50 lifeline.\n");
            }
            printf("Press 'q' to quit.\n");

            int ch = _getch();
            if (ch == 0 || ch == 224) {
                ch = _getch();
                if (ch == 72) {
                    int current_index = 0;
                    for (int i = 0; i < remaining_count; i++) {
                        if (remaining_options[i] == highlight) {
                            current_index = i;
                            break;
                        }
                    }
                    current_index--;
                    if (current_index < 0) {
                        current_index = remaining_count - 1;
                    }
                    highlight = remaining_options[current_index];
                } else if (ch == 80) {
                    int current_index = 0;
                    for (int i = 0; i < remaining_count; i++) {
                        if (remaining_options[i] == highlight) {
                            current_index = i;
                            break;
                        }
                    }
                    current_index++;
                    if (current_index >= remaining_count) {
                        current_index = 0;
                    }
                    highlight = remaining_options[current_index];
                }
            } else if (ch == 'f' || ch == 'F') {
                if (!lifeline_used && !current_lifeline_used) {
                    current_lifeline_used = 1;
                    lifeline_used = 1;

                    const cJSON *options = cJSON_GetObjectItemCaseSensitive(question_obj, "options");
                    const char option_labels[] = {'A', 'B', 'C', 'D'};
                    char incorrect_answers[4] = {'A', 'B', 'C', 'D'};
                    int incorrect_count = 0;

                    for (int i = 0; i < 4; i++) {
                        char option_label[2] = {option_labels[i], '\0'};
                        cJSON *option = cJSON_GetObjectItemCaseSensitive(options, option_label);
                        if (cJSON_IsString(option) && option->valuestring && option_labels[i] != correct_answer->valuestring[0]) {
                            incorrect_answers[incorrect_count++] = option_labels[i];
                        }
                    }

                    for (int i = 0; i < 2; i++) {
                        const int rand_idx = rand() % incorrect_count;
                        const char incorrect_answer = incorrect_answers[rand_idx];
                        const char option_label[2] = {incorrect_answer, '\0'};
                        cJSON *option = cJSON_GetObjectItemCaseSensitive(options, option_label);
                        if (cJSON_IsString(option)) {
                            option->valuestring = NULL;
                        }

                        for (int j = rand_idx; j < incorrect_count - 1; j++) {
                            incorrect_answers[j] = incorrect_answers[j + 1];
                        }
                        incorrect_count--;
                    }
                }
            } else if (ch == 13) {
                if (highlight == correct_answer->valuestring[0]) {
                    printf("\nCorrect!\n");
                    score++;
                    break;
                }
                printf("\nWrong! The correct answer was: %c\n", correct_answer->valuestring[0]);
                printf("You lost the game. Returning to the Main Menu.\n");
                _getch();
                cJSON_Delete(json);

                char name[51];
                printf("\033[s");
                printf("Enter your name (or press Enter to use 'Player'): ");
                fflush(stdout);
                fgets(name, sizeof(name), stdin);

                while (strlen(name) > 50
                    || strchr(name, '`') != NULL
                    || strchr(name, '~') != NULL
                    || strchr(name, '!') != NULL
                    || strchr(name, '@') != NULL
                    || strchr(name, '#') != NULL
                    || strchr(name, '$') != NULL
                    || strchr(name, '%') != NULL
                    || strchr(name, '^') != NULL
                    || strchr(name, '&') != NULL
                    || strchr(name, '*') != NULL
                    || strchr(name, '(') != NULL
                    || strchr(name, ')') != NULL
                    || strchr(name, '=') != NULL
                    || strchr(name, '+') != NULL
                    || strchr(name, '\\') != NULL
                    || strchr(name, '/') != NULL
                    || strchr(name, '<') != NULL
                    || strchr(name, '>') != NULL
                    || strchr(name, '?') != NULL) {
                    printf("\033[u\033[K\033[J");

                    printf("Your name should be under 50 characters and not contain any special characters: ");

                    fgets(name, sizeof(name), stdin);
                }

                name[strcspn(name, "\n")] = '\0';
                if (strlen(name) == 0) {
                    strcpy(name, "Player");
                }
                update_leaderboard(name, score);
                save_match_history(name, score);
                return;
            } else if (ch == 'q' || ch == 'Q') {
                cJSON_Delete(json);
                return;
            }
        }
    }

    clearScreen();
    printf("Congratulations! You have completed all questions correctly.\n");
    printf("You win!\n");
    printf("Press any key to return to the Main Menu.\n");
    _getch();
    cJSON_Delete(json);

    char name[100];
    printf("Enter your name (or press Enter to use 'Player'): ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    if (strlen(name) == 0) {
        strcpy(name, "Player");
    }
    update_leaderboard(name, score);
    save_match_history(name, score);
}

void main_menu() {
    int choice = 0;
    int highlight = 0;
    const char *options[] = {"Play Game", "Leaderboard", "Match History", "Exit"};
    const int n_options = sizeof(options) / sizeof(options[0]);

    while (1) {
        clearScreen();
        printf("Who Wants to Be a Millionaire - Main Menu\n\n");
        for (int i = 0; i < n_options; i++) {
            if (i == highlight) {
                printf("%s>> %s%s\n", COLOR_OPTION, options[i], COLOR_RESET);
            } else {
                printf("   %s\n", options[i]);
            }
        }
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            ch = _getch();
            if (ch == 72) {
                highlight--;
                if (highlight < 0)
                    highlight = n_options - 1;
            } else if (ch == 80) {
                highlight++;
                if (highlight >= n_options)
                    highlight = 0;
            }
        } else if (ch == 13) {
            choice = highlight;
            switch (choice) {
                case 0:
                    play_game();
                    break;
                case 1:
                    leaderboard_menu();
                    break;
                case 2:
                    match_history_menu();
                    break;
                case 3:
                    exit(0);
                default:
                    break;
            }
        }
    }
}