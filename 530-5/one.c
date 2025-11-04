#include <stdio.h>

// FIXED macros 
#define BRANCHES 3
#define PREDICT_MAX ~((~0) << VAR_N)

#define VAR_N 1

int hits[BRANCHES] = { 0 };
int misses[BRANCHES] = { 0 };
int predictors[BRANCHES] = { 1, 0, 0 };

// branch WITH history
void branch(int b, int take) {
    int prediction = (int) (predictors[b] > (PREDICT_MAX / 2));

    if (prediction == take) hits[b] += 1;
    else misses[b] += 1;

    if (take && predictors[b] != PREDICT_MAX)
        predictors[b] += 1;
    else if (!take && predictors[b] != 0)
        predictors[b] -= 1;
}

int main() {

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
