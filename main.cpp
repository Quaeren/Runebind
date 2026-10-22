#include <SFML/Graphics.hpp>
#include <cstdlib>
#include <iostream>

using namespace std;

#define WIDTH 100
#define HEIGHT 60

int map[HEIGHT][WIDTH];
int seed = time(NULL);

void genCave() {
    int rangeMin = 0;
    int rangeMax = 100;

    srand(seed);

    // Generate initial map
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if ((rand() % ((rangeMax+1 - rangeMin) + rangeMin)) < 55) {
                map[i][j] = 1;
            } else {
                map[i][j] = 0;
            }
        }
    }
}

int checkSurround(int x, int y, int gridSize=3) {
    int count = 0;
    int itMod = gridSize -1; // modifier for modifying the iterator
    int startMod = gridSize - itMod; // modifier for modifying the starting coordinates

    for (int i = y-startMod; i < y+itMod; i++) {
        for (int j = x-startMod; j < x+itMod; j++) {
            if (!(i == y && j == x)) {
                if (map[i][j] == 1) {
                    count++;
                    // cout << "1";
                } else {
                    // cout << "0";
                }
            } else {
                // cout << ".";
            }
        }
        // cout << endl;
    }
    return count;
}

void progressGen() {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (checkSurround(j, i) > 5) {
                map[i][j] = 1;
            } else if (checkSurround(j, i) < 4) {
                map[i][j] = 0;
            }
        }
    }
}

void displayCave() {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (map[i][j] == 0) {
                cout << "#";
            } else {
                cout << ".";
            }
        }
        cout << endl;
    }
}

int main() {
    genCave();
    cout << "Gen 0:" << endl;
    displayCave();
    cout << endl;
    for (int i=1; i<=5; i++) {
        progressGen();
        cout << "Gen " << i << ":" << endl;
        displayCave();
        cout << endl;
    }
    return 0;
}