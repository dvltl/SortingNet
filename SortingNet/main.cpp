//
//  main.cpp
//  SortingNet
//
//  Created by anton on 26/11/2016.
//  Copyright © 2016 RandomStuff. All rights reserved.
//

#include <mpi.h>
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <string>
#include <climits>
#include "ScheduleCreator.h"

struct Point {
    float coord[2];
    int index;
};

typedef struct Point point;
using namespace std;

bool pcompareX (point a, point b) {
    float x1 = a.coord[0];
    float x2 = b.coord[0];
    
    if (a.index < 0) {
        return false;
    } else if (b.index < 0) {
        return true;
    }
    
    return (x1 < x2);
}

bool pcompareY (point a, point b) {
    float y1 = a.coord[1];
    float y2 = b.coord[1];
    
    if (a.index < 0) {
        return false;
    } else if (b.index < 0) {
        return true;
    }
    
    return (y1 < y2);
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

vector< point > to_vector(const point * const arr, int size) {
    vector< point > result;
    for (int i = 0; i < size; ++i) {
        result.push_back(arr[i]);
    }
    return result;
}

// possible memory leak?
// call delete before using again
point * to_carray (const vector< point > & vec) {
    point * result = new point[vec.size()];
    for (int i = 0; i < vec.size(); ++i) {
        result[i] = vec[i];
    }
    return result;
}

vector< point > merge(const vector< point > & first, const vector< point > & second, bool head, bool sortX) {
    vector< point > result;
    int i, j, k;
    
    if (head) {
        i = 0;
        j = 0;
        
        while (result.size() < first.size()) {
            if (sortX ? pcompareX(first[i], second[j]) : pcompareY(first[i], second[j])) {
                result.push_back(first[i]);
                ++i;
            } else {
                result.push_back(second[j]);
                ++j;
            }
        }
    } else {
        i = first.size() - 1;
        j = second.size() - 1;
        stack< point > st;
        // being lazy here
        while (st.size() < first.size()) {
            if (sortX ? !pcompareX(first[i], second[j]) : !pcompareY(first[i], second[j])) {
                st.push(first[i]);
                --i;
            } else {
                st.push(second[j]);
                --j;
            }
        }
        
        while (!st.empty()) {
            result.push_back(st.top());
            st.pop();
        }
    }
    
    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Correct usage: ./a.out <n1> <n2> [ 0 | !0 ] (depending on coordinate to sort)" << endl;
        return -1;
    }
    
    srand(static_cast<unsigned int>(time(NULL)));
    
    // every process will be able to see this info
    int n1 = stoi(argv[1]);
    int n2 = stoi(argv[2]);
    bool sortX = string(argv[3]) == "0";
    
    int size = n1 * n2;
    
    float * x = new float[size];
    float * y = new float[size];
    point * P = new point[size];
    point * result;
    
    init_arr(x, size, n2);
    init_arr(y, size, n2);
    
    cout << "Before sort:" << endl;
    for (int it = 0; it < size; ++it) {
        P[it].coord[0] = x[it];
        P[it].coord[1] = y[it];
        P[it].index = it;
        cout << it << ' ' << x[it] << ' ' << y[it] << endl;
    }
    
    delete [] x;
    delete [] y;
    
    
    // initialization of MPI services
    MPI_Init(&argc, &argv);
    
    int rank, proc_count;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_count);
    
    if (proc_count < 2) {
        cout << "Processor count must be greater than 1" << endl;
        return -1;
    }
    
    // creating new MPI_Datatypes for our future needs
    // coords array
    MPI_Datatype CoordsMPI;
    
    if (MPI_Type_vector(2, 1, 1, MPI_FLOAT, &CoordsMPI) != MPI_SUCCESS) {
        MPI_Finalize();
        return -1;
    }
    if (MPI_Type_commit(&CoordsMPI) != MPI_SUCCESS) {
        MPI_Finalize();
        return -1;
    }
    
    // Point
    MPI_Datatype PointMPI;
    MPI_Datatype types[2] = { CoordsMPI, MPI_INT };
    int blocks[2] = {1, 1};
    MPI_Aint disp[2];
    disp[0] = reinterpret_cast<const unsigned char*>(&P[0].coord[0]) - reinterpret_cast<const unsigned char*>(&P[0]);
    disp[1] = reinterpret_cast<const unsigned char*>(&P[0].index) - reinterpret_cast<const unsigned char*>(&P[0]);
    
    if (MPI_Type_create_struct(2, blocks, disp, types, &PointMPI) != MPI_SUCCESS) {
        MPI_Finalize();
        return -1;
    }
    if (MPI_Type_commit(&PointMPI) != MPI_SUCCESS) {
        MPI_Finalize();
        return -1;
    }
    
    
    // dispatching first batches that need sort
    int arr_size = (size + 1) / proc_count;
    vector< point > to_sort;
    
    for (int it = rank * arr_size; it < min((rank + 1) * arr_size, size); ++it) {
        to_sort.push_back(P[it]);
    }
    
    
    // adding fictive elements in case we are short on elements
    point fictive;
    fictive.coord[0] = INT_MAX; // just in case
    fictive.coord[1] = INT_MAX;
    fictive.index = -1;
    
    for (int i = 0; i < (arr_size - to_sort.size()); ++i) {
        to_sort.push_back(fictive);
    }
    
    
    // initial sort
    if (sortX) {
        sort(to_sort.begin(), to_sort.end(), pcompareX);
    } else {
        sort(to_sort.begin(), to_sort.end(), pcompareY);
    }
    
    
    // CURRENT
    point * current = new point[arr_size];
    for (int i = 0; i < arr_size; ++i) {
        current[i] = to_sort[i];
    }
    
    
    // creating schedule for processors to act as batcher sorting net
    ScheduleCreator creator;
    vector< pair<int,int> > sched = creator.create_schedule(proc_count);
    
    // TODO: 2nd part of the array goes in the reversed order (from biggest to smallest)
    
    // batcher sorting net part
    MPI_Status status;
    point * received = new point[arr_size];
    vector< point > merge_res;
    
    for (size_t i = 0; i < sched.size(); ++i) {
        if (rank == sched[i].first) {
            MPI_Send(current, arr_size, PointMPI, sched[i].second, 0, MPI_COMM_WORLD);
            MPI_Recv(received, arr_size, PointMPI, sched[i].second, 0, MPI_COMM_WORLD, &status);
            
            // need only first $arr_size elements
            vector< point > new_tosort = to_vector(received, arr_size);
            
            to_sort = merge(to_sort, new_tosort, true, sortX);
            
            delete [] current;
            current = to_carray(to_sort);
            
        } else if (rank == sched[i].second) {
            MPI_Recv(received, arr_size, PointMPI, sched[i].first, 0, MPI_COMM_WORLD, &status);
            MPI_Send(current, arr_size, PointMPI, sched[i].first, 0, MPI_COMM_WORLD);
            
            // need only last $arr_size elements
            vector< point > new_tosort = to_vector(received, arr_size);
            
            to_sort = merge(to_sort, new_tosort, false, sortX);
            
            delete [] current;
            current = to_carray(to_sort);
            
        }
    }
    
    
    // gathering sort results from all processors
    if (rank == 0) {
        result = new point[ arr_size * proc_count ];
    }
    MPI_Gather(current, arr_size, PointMPI, result, arr_size, PointMPI, 0, MPI_COMM_WORLD);
    
    
    // finalization
    if (rank == 0) {
        cout << "After sort:" << endl;
        for (int i = 0; i < arr_size * proc_count; ++i) {
            if (result[i].index != -1 && result[i].coord[0] != INT_MAX) {
                cout << i << ' ' << result[i].coord[0] << ' ' << result[i].coord[1] << endl;
            }
        }
        delete [] P;
        delete [] result;
    }
    
    MPI_Finalize();
    
    return 0;
}
