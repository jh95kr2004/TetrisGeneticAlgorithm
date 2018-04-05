#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <algorithm>
#include <ncurses.h>
#include <omp.h>
#include "main.h"

using namespace std;

/*
 Structure of node for state space tree.
 Each node stores the state of game.
 */
typedef struct _NODE {
    int level;                                  // Level of node.
    double accumulatedScore;                    // Accumulated score of node.
    int recField[HEIGHT][WIDTH];                // State of field.
    struct _NODE *child;                        // Linked-list of child nodes.
    int blockId;                                // ID of block.
    int blockX, blockY, blockRotate;            // Rotation status and position of block.
    bool scoreUpdateFlag;                       // Set TRUE once accunulatedScore is updated by child node.
} NODE;

/*
 Structure of individual of genetic algorithm.
 Each individual stores the weights of factors.
 */
typedef struct _INDIVIDUAL {
    double weight[NUM_OF_WEIGHTS];
    /*
     Weights of factors (properties)
     [0]: Sum of heights of each column of field
     [1]: # of Holes
     [2]: # of Blockades
     [3]: Gotten score by reaching block to the floor
     [4]: Gotten score by removing lines
     [5]: # of blocks reached at wall
     [6]: SD(Standard deviation) of heights of field
     [7]: Difference between max height and min height of field
     */
    double score;           // Score of each individual. (Fitness of each individual)
} INDIVIDUAL;

INDIVIDUAL population[NUM_OF_POPULATION], good[NUM_OF_TOP_POPULATION];  // Each generation has 20 chromosomes and select best 4 of them.
int gen = 1; // # of generation, # of individual

WINDOW* Windows[NUM_OF_POPULATION];

bool NO_SCREEN = false;

int playTetris(int pop);
bool checkBlockCanMove(int f[HEIGHT][WIDTH],int blockId, int blockRotate, int blockY, int blockX);
int addBlockToField(int f[HEIGHT][WIDTH], int blockId, int blockRotate, int blockY, int blockX);
int deleteLineFromField(int f[HEIGHT][WIDTH]);
void getRecommendedPlay(NODE *parent, int *blockRotate, int *blockY, int *blockX, int blockQueue[], int pop);

/*
 Comparison function for compare the fitness of two individuals.
 This function will compare the game score of them.
 */
bool individualLessFunction(struct _INDIVIDUAL a, struct _INDIVIDUAL b) {
    if(a.score > b.score) return true;
    return false;
}

int main(int argc, char *argv[]) {
    FILE *outf;
    int i, j, thread_count = 1;
    
    if(argc >= 2) {
        for(i = 1; i < argc; i++) {
            if(strcmp(argv[i], "-t") == 0 && i < argc - 1) thread_count = strtol(argv[++i], NULL, 10);
            else if(strcmp(argv[i], "-noscreen") == 0) NO_SCREEN = true;
        }
    }
    
    /*
     Generate 20 random chromosomes for the first generation. (Initialization)
     We can assume some factors might be good for playing Tetris, but some factors are not.
     - Bad factors: 'Sum of heights of each column of field', '# of holes', '# of blockades' and 'SD of heights of field'.
     - Good factors: 'Gotten score by reaching block to the floor', 'Gotten score by removing lines' and '# of blocks reached at wall'
     Therefore, we make that bad factors as negative number and good factors as positive number to prevent stuck in local minima.
     */
    srand((unsigned int)time(NULL));
    for(i = 0; i < NUM_OF_POPULATION; i++) {
        for(j = 0; j < NUM_OF_WEIGHTS; j++) {
            population[i].weight[j] = (((double)rand())/((double)RAND_MAX/(double)5.0));
            if((j == 0 || j == 1 || j == 2 || j == 6) && population[i].weight[j] > 0) population[i].weight[j] = -population[i].weight[j];
            if((j == 3 || j == 4 || j == 5) && population[i].weight[j] < 0) population[i].weight[j] = -population[i].weight[j];
        }
    }
    /*
     Now start genetic algorithm.
     Population of each generation is 20.
     To generate next generation, top 4 individuals will be chosen which show good performance. (Selection)
     And 20 individuals of next generation will be generated by mixing weights of them. (Crossover)
     Also mutation will occured by 10% chance and weight will be modified in range of -0.5 ~ 0.5. (Mutation)
     Fintness(performance) of each individual will be evaluated by average score after playing 20 games. So, fitness function is just playing tetris.
     For every generation, the weights of the best individual will be written in the 'output.txt' file.
     */
    initscr();
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_WHITE);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(5, COLOR_WHITE, COLOR_BLACK);
    init_pair(6, COLOR_GREEN, COLOR_BLACK);
    init_pair(7, COLOR_RED, COLOR_BLACK);
    noecho();
    curs_set(false);
    
    WINDOW *generationWindow = newwin(0, 0, 0, 0), *resultWindow;
    scrollok(generationWindow, TRUE);
    wrefresh(generationWindow);
    for(i = 0; i < NUM_OF_POPULATION; i++) {
        if(!NO_SCREEN) Windows[i] = newwin(HEIGHT + 4, WIDTH + 10, 1 + (i / 10) * (HEIGHT + 4), (WIDTH + 10) * (i % 10));
        else Windows[i] = newwin(4, WIDTH + 10, 1 + (i / 10) * 4, (WIDTH + 10) * (i % 10));
        scrollok(Windows[i], TRUE);
        wrefresh(Windows[i]);
    }
    if(!NO_SCREEN) resultWindow = newwin(0, 0, 1 + 2 * (HEIGHT + 4), 0);
    else resultWindow = newwin(0, 0, 1 + 2 * 4, 0);
    scrollok(resultWindow, TRUE);
    wrefresh(resultWindow);
    
    while(1)
    {
        mvwprintw(generationWindow, 0, 0, "Generation: %d", gen);
        wrefresh(generationWindow);
        for(i = 0; i < NUM_OF_POPULATION; i++) {
            wclear(Windows[i]);
            mvwprintw(Windows[i], 0, 0, "Pop: %d", i + 1);
            wrefresh(Windows[i]);
        }
        #pragma omp parallel num_threads(thread_count)
        #pragma omp for private(i, j)
        for(i = 0; i < NUM_OF_POPULATION; i++)
        {
            for(j = 0; j < NUM_OF_PLAY; j++)
            {
                mvwprintw(Windows[i], 0, 0, "Pop: %d (%d)", i + 1, j + 1);
                #pragma omp critical(wrefresh)
                {
                    wrefresh(Windows[i]);
                }
                population[i].score += playTetris(i);
            }
            population[i].score /= (double)NUM_OF_PLAY; // Get the average of 20 scores.
            if(!NO_SCREEN) mvwprintw(Windows[i], HEIGHT + 1, 0, "score: %.2lf\n", population[i].score);
            else mvwprintw(Windows[i], 1, 0, "score: %.2lf\n", population[i].score);
            #pragma omp critical(wrefresh)
            {
                wrefresh(Windows[i]);
            }
        }
        sort(population, population + NUM_OF_POPULATION, individualLessFunction);
        mvwprintw(resultWindow, 0, 0, "Generation %d is finished. (Max Score: %.2lf)", gen, population[0].score);
        wrefresh(resultWindow);
        outf = fopen("output.txt", "a");
        fprintf(outf, "Gen %d : ", gen);
        for(i = 0; i < NUM_OF_WEIGHTS; i++)
            fprintf(outf, "%lf ", population[0].weight[i]);
        fprintf(outf, "\n");
        fclose(outf);
        // mvprintw(4, 0, "generation %d complete\n", gen);
        // refresh();
        // Selection: Select top 4 individuals.
        for(i = 0; i < NUM_OF_TOP_POPULATION; i++)
            for(j = 0; j < NUM_OF_WEIGHTS; j++)
                good[i].weight[j] = population[i].weight[j];
        // Crossover: Mix weights of top 4 individuals to make new generation.
        for(i = 0; i < NUM_OF_POPULATION; i++) {
            for(j = 0; j < NUM_OF_WEIGHTS; j++) {
                population[i].weight[j] = good[rand() % NUM_OF_TOP_POPULATION].weight[j];
                // Mutation: Mutate the weights in -0.5 ~ 0.5 by 10% chance.
                if(rand() % 100 < CHANCE_OF_MUTATION) {
                    if(rand() % 2 == 0) population[i].weight[j] += (((double)rand())/((double)RAND_MAX/(double)AMOUNT_OF_MUTATION));
                    else population[i].weight[j] -= (((double)rand())/((double)RAND_MAX/(double)AMOUNT_OF_MUTATION));
                }
            }
            population[i].score = 0;
        }
        gen++;
    }
    endwin();
    return 0;
}

/*
 Plays tetris game until it is over.
 1. The game will be initialized.
 2. Recommended play of current block will be obtained by calling getRecommendedPlay().
    This will be presented as location(recommendX, recommendY) and rotation status(recommendR) of block.
 3. Current block will be stacked at field according to the recommended play and switched to the next block.
    If current block gets out of the boundary of field, the game is over.
 4. After current block is stacked on the field successfully, score is updated and current block will be switched to next block.
 5. 2 ~ 4 will be iterated until game ends.
 */
int playTetris(int pop) {
    int i, j;            // Variables for iterations.
    int field[HEIGHT][WIDTH];               // Field of game where blocks are stacked.
    int blockQueue[BLOCK_NUM];              // Queue of blocks. ([0]: Current block, [1]: Next block)
    int blockRotate, blockY, blockX;        // Recommended rotation(recommendR) and position(recommendX, recommendY) of current block.
    int score;                   // Stores score of game.
    NODE Root;                              // Root node of state space tree.
    
    // Initializes the game.
    for(i = 0; i < HEIGHT; i++) for(j = 0; j < WIDTH; j++) Root.recField[i][j] = field[i][j] = 0;
    for(i = 0; i < BLOCK_NUM; i++) blockQueue[i] = rand() % 7;
    score = 0;
    Root.level = 0;
    Root.accumulatedScore = 0;
    Root.child = NULL;
    Root.scoreUpdateFlag = false;
    
    // Play game until it is over.
    while(1) {
        // Get the recommended play of current block.
        getRecommendedPlay(&Root, &blockRotate, &blockY, &blockX, blockQueue, pop);
        // Check whether block will get out of the boundary of field by doing recommended play or not. If it does, game should be over.
        if(blockY <= boundary[blockQueue[0]][blockRotate].y1 - 1) break;
        // If block can be stacked on the field normally, add block to the field and update score.
        score += addBlockToField(field, blockQueue[0], blockRotate, blockY, blockX);
        score += deleteLineFromField(field);
        // Switch current blcok to next block.
        for(i = 0; i < BLOCK_NUM - 1; i++) blockQueue[i] = blockQueue[i + 1];
        blockQueue[i] = rand() % 7;
        for(i = 0; i < HEIGHT; i++) {
            for(j = 0; j < WIDTH; j++) {
                Root.recField[i][j] = field[i][j];
                if(!NO_SCREEN) {
                    if(field[i][j] != 0) {
                        wattron(Windows[pop], A_REVERSE);
                        wattron(Windows[pop], COLOR_PAIR(field[i][j]));
                        mvwprintw(Windows[pop], i + 1, j, " ");
                        wattroff(Windows[pop], COLOR_PAIR(field[i][j]));
                        wattroff(Windows[pop], A_REVERSE);
                    } else {
                        mvwprintw(Windows[pop], i + 1, j, " ");
                    }
                }
            }
        }
        if(!NO_SCREEN) {
            #pragma omp critical(wrefresh)
            {
                wrefresh(Windows[pop]);
            }
        }
    }
    // wrefresh(win);
    // Game is over. Return the score of the game.
    return score;
}

/*
 Checks whether block(blockId) can be moved to the location(blockX, blockY) of field(f) with rotation status(blockRotate) or not.
 It returns true if block can be moved, or returns false if block cannot be moved.
 */
bool checkBlockCanMove(int f[HEIGHT][WIDTH], int blockId, int blockRotate, int blockY, int blockX) {
    int i, j;
    for(i = 0; i < BLOCK_HEIGHT; i++)
        for(j = 0; j < BLOCK_WIDTH; j++)
            if(block[blockId][blockRotate][i][j] == 1 && (blockY + i >= HEIGHT || blockX + j >= WIDTH || blockX + j < 0 || (blockY + i >= 0 && blockY + i < HEIGHT && f[blockY + i][blockX + j] != 0)))
                return false;
    return true;
}

/*
 Adds block to the field.
 It returns 10 * the number of sides of block that touches the bottom of the field.
 */
int addBlockToField(int f[HEIGHT][WIDTH], int blockId, int blockRotate, int blockY, int blockX) {
    int i, j, touched = 0;
    for(i = 0; i < BLOCK_HEIGHT; i++) {
        for(j = 0; j < BLOCK_WIDTH; j++) {
            if(block[blockId][blockRotate][i][j] == 1 && i+blockY >= 0) {
                f[i + blockY][j + blockX] = blockId + 1;
                if(i + blockY + 1 >= HEIGHT || f[i + blockY + 1][j + blockX] != 0)
                    touched++;
            }
        }
    }
    return touched * 10;
}

/*
 Deletes complete lines of the fields.
 it returns 100 * the square of the number of complete lines.
 */
int deleteLineFromField(int f[HEIGHT][WIDTH]) {
    int cnt = 0, i, j, k;
    int deleteLineFromField[HEIGHT];
    for(i = HEIGHT - 1; i >= 0; i--) {
        for(j = 0; j < WIDTH; j++)
            if(f[i][j] == 0) break;
        if(j == WIDTH) deleteLineFromField[cnt++] = i;
    }
    for(k = 0; k < cnt - 1; k++)
        for(i = deleteLineFromField[k] + k; i >= deleteLineFromField[k + 1] + (k + 1); i--)
            for(j = 0; j < WIDTH; j++)
                f[i][j] = (i - (k + 1) >= 0) ? f[i - (k + 1)][j] : 0;
    if(cnt > 0)
        for(i = deleteLineFromField[k] + k; i >= 0; i--)
            for(j = 0; j < WIDTH; j++)
                f[i][j] = (i - cnt >= 0) ? f[i - cnt][j] : 0;
    return cnt * cnt * 100;
}

/*
 Returns recommended play of current block.
 It will be obtained by using state space tree and DFS.
 Each node of tree has accumulated score after block is stacked on the field with corresponding location and rotation state.
 Scores of each play will be calculated according to the weights of current individual of genetic algorithm.
 Every nodes in the same depth(level) use the same order of block in blcokQueue. But its location and rotation state is different.
 Parent node of child nodes choose one of them which shows the best performance.
 Therefore, root node of tree makes child nodes which stack current block(blockQueue[0]) and then select the best one of them.
 Recursively, they make next level of child nodes which stack next blokc(blockQueue[1]) and then select the best one of them.
 By DFS, the root node will hold the best play using all the blocks in blockQueue.
 The best play will be returned by storing values at the pointer parameters of function: location(blockX, blockY) and rotation(blockRotate) of block.
 */
void getRecommendedPlay(NODE *parent, int *blockRotate, int *blockY, int *blockX, int blockQueue[], int pop) {
    int r, x, y, i, j;                                          // r: rotation state, x: x coordination, y: y coordination, i, j: for iterations.
    double scoreOfParent = parent->accumulatedScore;            // Store the score of parent node of the child nodes.
    double averageOfHeight, averageOfSquareOfHeight, SDofHeight;        // Variables for calculating standard deviation of height.
    int sumOfHeight, countOfBlocksOfColumn, countOfBlockades, countOfHolesOfColumn, countOfHoles, countOfWallSides, maxHeight, minEdgeHeight;
    NODE *child, *next;
    
    // As we are using DFS, not BFS, actually, only one child node will be exist simultaneously.
    // Therefore, for the optimization of memory usage, parent node will re-use child node after memory allocation.
    if(parent->child == NULL) {
        child = parent->child = (NODE*)malloc(sizeof(NODE));
        child->child = NULL;
        child->scoreUpdateFlag = false;
    }
    else child = parent->child;
    
    // Now creates child nodes which play the block of the blockQueue according to the current depth(level) of tree.
    // Set the rotation of block.
    for(r = 0; r < 4; r++) {
        // Set the x coordination of block.
        for(x = boundary[blockQueue[parent->level]][r].x1; x <= boundary[blockQueue[parent->level]][r].x2; x++) {
            // Set the y coordination of block. Value of y will be increased until block touches the stack of field or floor.
            for(y = boundary[blockQueue[parent->level]][r].y1 - boundary[blockQueue[parent->level]][r].size_y; y <= boundary[blockQueue[parent->level]][r].y2; y++)
                if(!checkBlockCanMove(parent->recField, blockQueue[parent->level], r, y + 1, x)) break;
            // If block gets out of the field, do next loop of x coordination.
            if(y > boundary[blockQueue[parent->level]][r].y2) continue;
            averageOfHeight = averageOfSquareOfHeight = maxHeight = minEdgeHeight = countOfWallSides = countOfBlockades = countOfHoles = sumOfHeight = 0;
            // Copy the field from the parent node.
            for(i = 0; i < HEIGHT; i++)
                for(j = 0; j < WIDTH; j++)
                    child->recField[i][j] = parent->recField[i][j];
            // Count the number of sides of block that touches the wall.
            for(i = 0; i < 4; i++)
                for(j = 0; j < 4; j++)
                    if(block[blockQueue[parent->level]][r][i][j] == 1 && (x + j == 0 || x + j == WIDTH - 1)) countOfWallSides++;
            // Add block to the field at the location of (x, y) with rotation(r) and delete complete lines. Also score will be updated.
            child->accumulatedScore = scoreOfParent;
            child->accumulatedScore += (double)addBlockToField(child->recField, blockQueue[parent->level], r, y, x) * population[pop].weight[3];
            child->accumulatedScore += (double)deleteLineFromField(child->recField) * population[pop].weight[4];
            // Calculate sum of height, the number of holes and blockades, standard deviation of height, maximum of height and minimun of height of edges.
            for(i = 0; i < WIDTH; i++) {
                countOfBlocksOfColumn = countOfHolesOfColumn = 0;
                for(j = 0; j < HEIGHT; j++) {
                    if(child->recField[j][i] > 0) {
                        if(countOfBlocksOfColumn == 0) {
                            sumOfHeight += HEIGHT - j;
                            averageOfHeight += (double)(HEIGHT - j);
                            averageOfSquareOfHeight += ((double)(HEIGHT - j) * (double)(HEIGHT - j));
                            maxHeight = (HEIGHT - j > maxHeight) ? HEIGHT - j : maxHeight;
                            if(i == 0) minEdgeHeight = HEIGHT - j;
                            if(i == WIDTH - 1 && minEdgeHeight > HEIGHT - j) minEdgeHeight = HEIGHT - j;
                        }
                        countOfBlocksOfColumn++;
                    }
                    else if(child->recField[j][i] == 0 && countOfBlocksOfColumn > 0)
                        countOfHolesOfColumn++;
                }
                if((i == 0 || i == WIDTH-1) && countOfBlocksOfColumn == 0) minEdgeHeight = 0;
                countOfHoles += countOfHolesOfColumn;
                if(countOfHolesOfColumn > 0) countOfBlockades += countOfBlocksOfColumn;
            }
            averageOfHeight /= (double)WIDTH;
            averageOfSquareOfHeight /= (double)WIDTH;
            SDofHeight = sqrt(averageOfSquareOfHeight - averageOfHeight * averageOfHeight);
            // Update score.
            child->accumulatedScore += population[pop].weight[0] * sumOfHeight;
            child->accumulatedScore += population[pop].weight[1] * countOfHoles;
            child->accumulatedScore += population[pop].weight[2] * countOfBlockades;
            child->accumulatedScore += population[pop].weight[5] * countOfWallSides;
            child->accumulatedScore += population[pop].weight[6] * SDofHeight;
            child->accumulatedScore += population[pop].weight[7] * (maxHeight - minEdgeHeight);
            child->level = parent->level + 1;
            child->scoreUpdateFlag = false;
            // If there are blocks to be considered in the blockQueue, do recursive.
            if(child->level < BLOCK_NUM) getRecommendedPlay(child, NULL, NULL, NULL, blockQueue, pop);
            // Parent node should recommend the situation of location and rotation which shows the best performance.
            // accumulatedScore variable of parent node will be contiuously updated to the best score whenever child node finishes DFS.
            if(parent->scoreUpdateFlag == false || child->accumulatedScore > parent->accumulatedScore || (child->accumulatedScore == parent->accumulatedScore && rand() % 2 == 0)) {
                parent->scoreUpdateFlag = true;
                parent->accumulatedScore = child->accumulatedScore;
                parent->blockX = x;
                parent->blockY = y;
                parent->blockRotate = r;
            }
        }
    }
    // If DFS of current node of finished and the current node is root node of tree, returns the result of DFS.
    // As we need just the recommended play of current block(first block of blockQueue), we don't need to know how the child nodes played game.
    // As the best play of current block is stored in the root node, copy the values of root node to the pointer parameters(blockRotate, blockX, blockY) of this function.
    if(parent->level == 0) {
        *blockRotate = parent->blockRotate;
        *blockY = parent->blockY;
        *blockX = parent->blockX;
        child = parent->child;
        while(child != NULL) {
            next = child->child;
            free(child);
            child = next;
        }
        parent->accumulatedScore = 0;
        parent->child = NULL;
        parent->scoreUpdateFlag = false;
    }
    return;
}
