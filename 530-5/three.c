#include <stdio.h>

// FIXED macros 
#define BRANCHES 3
#define PREDICT_MAX ~((~0) << VAR_N)
#define GLOBAL_MASK ~((~0) << VAR_M)

// USER CONFIGURABLE macros
#define VAR_M 1
#define VAR_N 2

int hits[BRANCHES];
int misses[BRANCHES];
int predictors[2 * VAR_M][BRANCHES];
int history = 0;

// branch WITH history
void branch(int b, int take) {
    int prediction = (int) (predictors[history][b] > (PREDICT_MAX / 2));

    if (prediction == take) hits[b] += 1;
    else misses[b] += 1;

    if (take && predictors[history][b] != PREDICT_MAX)
        predictors[history][b] += 1;
    else if (!take && predictors[history][b] != 0)
        predictors[history][b] -= 1;

    history = ((history << 1) | take) & GLOBAL_MASK;
}

int main(int argc, char** argv) {
    // simulation
    for (int i = 1; i < 101; i++) {
        branch(2, 1);

        if ((i % 4) == 0) {
            branch(0, 1);

            for (int j = 1; j < 11; j++) {
                branch(1, 1);
            }
            branch(1, 0);
        }
        else
            branch(0, 0);
    }
    branch(2, 0);

    // print results
    for (int i = 0; i < BRANCHES; i++) {
        printf("Branch %d: %d hits %d misses\n", i + 1, hits[i], misses[i]);
    }
}
