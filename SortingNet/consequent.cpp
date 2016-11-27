//
//  main.cpp
//  SortingNet
//
//  Created by anton on 26/11/2016.
//  Copyright © 2016 RandomStuff. All rights reserved.
//

#include <iostream>
#include <ctime>
#include <string>

struct Point {
    float coord[2];
    int index;
};

typedef struct Point point;
using namespace std;

int pcompareX (const void * a, const void * b) {
    float x1 = ((point*)a) -> coord[0];
    float x2 = ((point*)b) -> coord[0];
    if (x1 < x2)
        return -1;
    else if (x1 == x2)
        return 0;
    else
        return 1;
}

int pcompareY (const void * a, const void * b) {
    float y1 = ((point*)a) -> coord[1];
    float y2 = ((point*)b) -> coord[1];
    if (y1 < y2)
        return -1;
    else if (y1 == y2)
        return 0;
    else
        return 1;
}

float get_rand_float(float i, float j) {
    if (i * j == 0)
        return -(static_cast<float>(rand() % 10000));
    else
        return static_cast<float>(rand() % 10000) / (i * j);
}

void init_arr(float * arr, int n1, int n2) {
    for (int i = 0; i < n1; ++i) {
        for (int j = 0; j < n2; ++j) {
            arr[i * n2 + j] = get_rand_float(i, j);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        return -1;
    }
    
    srand(static_cast<unsigned int>(time(NULL)));
    
    int n1 = atoi(argv[1]);
    int n2 = atoi(argv[2]);
    int sortX = atoi(argv[3]);
    
    int size = n1 * n2;
    
    float * x = new float[size];
    float * y = new float[size];
    point * P = new point[size];
    
    init_arr(x, n1, n2);
    init_arr(y, n1, n2);
    
    for (int it = 0; it < size; ++it) {
        P[it].coord[0] = x[it];
        P[it].coord[1] = y[it];
        P[it].index = it;
    }
    
    time_t timer;
    time(&timer);
    
    if (sortX) {
        qsort(&P[0], size, sizeof(point), pcompareX);
    } else {
        qsort(&P[0], size, sizeof(point), pcompareY);
    }
    
    cout << "Sort time " << difftime(timer, time(NULL)) << endl;
    
    delete [] P;
    delete [] x;
    delete [] y;
    
    return 0;
}
