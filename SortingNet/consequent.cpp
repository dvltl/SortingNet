//
//  main.cpp
//  SortingNet
//
//  Created by anton on 26/11/2016.
//  Copyright © 2016 RandomStuff. All rights reserved.
//

#include <iostream>
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
        return -(static_cast<float>(rand() % 100000));
    else
        return static_cast<float>(rand() % 100000) / (i * j);
}

void init_arr(float * arr, int size, int n) {
    int i;
    int j;
    for (int it = 0; it < size; ++it) {
        i = it / n;
        j = it % n;
        arr[i * n + j] = get_rand_float(i, j);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        return -1;
    }
    
    srand(static_cast<unsigned int>(time(NULL)));
    
    int n1 = stoi(argv[1]);
    int n2 = stoi(argv[2]);
    int sortX = stoi(argv[3]);
    
    int size = n1 * n2;
    
    float * x = new float[size];
    float * y = new float[size];
    point * P = new point[size];
    
    init_arr(x, size, n2);
    init_arr(y, size, n2);
    
    cout << "Before sort:" << endl;
    for (int it = 0; it < size; ++it) {
        cout << it << ' ' << x[it] << ' ' << y[it] << endl;
        P[it].coord[0] = x[it];
        P[it].coord[1] = y[it];
        P[it].index = it;
    }
    
    if (sortX) {
        qsort(&P[0], size, sizeof(point), pcompareX);
    } else {
        qsort(&P[0], size, sizeof(point), pcompareY);
    }
    
    cout << "After sort:" << endl;
    for (int it = 0; it < size; ++it) {
        cout << P[it].index << ' ' << P[it].coord[0] << ' ' << P[it].coord[1] << endl;
    }
    
    delete [] P;
    delete [] x;
    delete [] y;
    
    return 0;
}
