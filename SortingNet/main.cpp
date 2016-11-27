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

point * m_merge(const point * first, const point * second, int size, bool head, bool (*comp)(point, point)) {
    point * result = new point[size];
    int i, j, k;
    
    if (head) {
        i = 0;
        j = 0;
        
        for (k = 0; k < size; ++k){
            if (comp(first[i], second[j])) {
                result[k] = first[i++];
            } else {
                result[k] = second[j++];
            }
        }
    } else {
        i = size - 1;
        j = size - 1;
        
        for (k = size - 1; k >= 0; --k) {
            if (!comp(first[i], second[j])) {
                result[k] = first[i--];
            } else {
                result[k] = second[j--];
            }
        }
        
    }
    
    delete [] first;
    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Correct usage: ./a.out <n1> <n2> [ 0 | !0 ] (depending on coordinate to sort)" << endl;
        return -1;
    }
    
    int n1 = atoi(argv[1]);
    int n2 = atoi(argv[2]);
    bool sortX = string(argv[3]) == "0";
    bool (*compare)(point, point);
    
    if (sortX) {
        compare = &pcompareX;
    } else {
        compare = &pcompareY;
    }
    
    int size = n1 * n2;
    
    point * P = new point[size];
    point * result;

    double init_time;
    double sort_time;
    double write_time;
    
    // initialization of MPI services
    MPI_Init(&argc, &argv);
    
    int rank, proc_count;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_count);
    
    if (proc_count < 2) {
        cout << "Processor count must be greater than 1" << endl;
        return -1;
    }

    init_time = MPI_Wtime();
    if (rank == 0) {
        srand(static_cast<unsigned int>(time(NULL)));
        
        float * arr = new float[size];
        
        init_arr(arr, n1, n2);
        for (int it = 0; it < size; ++it) {
            P[it].coord[0] = arr[it];
            P[it].index = it;
        }
        
        init_arr(arr, n1, n2);
        for (int it = 0; it < size; ++it) {
            P[it].coord[1] = arr[it];
        }
        delete [] arr;
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
    
    MPI_Bcast(P, size, PointMPI, 0, MPI_COMM_WORLD);
    
    // dispatching first batches that need sort
    int arr_size = (size + 1) / proc_count;
    vector< point > to_sort;
    
    for (int it = rank * arr_size; it < min((rank + 1) * arr_size, size); ++it) {
        to_sort.push_back(P[it]);
    }
    
    
    // adding fictive elements in case we are short on elements
    point fictive;
    fictive.coord[0] = LONG_MAX; // just in case
    fictive.coord[1] = LONG_MAX;
    fictive.index = -1;
    
    for (int i = 0; i < (arr_size - to_sort.size()); ++i) {
        to_sort.push_back(fictive);
    }
    
    // creating schedule for processors to act as batcher sorting net
    ScheduleCreator creator;
    vector< pair<int,int> > sched = creator.create_schedule(proc_count);
    
    init_time = MPI_Wtime() - init_time;

    
    // initial sort
    sort_time = MPI_Wtime();
    sort(to_sort.begin(), to_sort.end(), compare);
    
    
    // CURRENT
    point * current = to_carray(to_sort);
    
    // batcher sorting net part
    MPI_Status status;
    point * received = new point[arr_size];
    
    const int part_num = 2;
    const int sz = arr_size / part_num;
    
    for (size_t i = 0; i < sched.size(); ++i) {
        if (rank == sched[i].first) {
            MPI_Send(current, arr_size, PointMPI, sched[i].second, 0, MPI_COMM_WORLD);
            MPI_Recv(received, arr_size, PointMPI, sched[i].second, 0, MPI_COMM_WORLD, &status);
            
            // need only first $arr_size elements
            current = m_merge(current, received, arr_size, true, compare);
            
        } else if (rank == sched[i].second) {
            MPI_Recv(received, arr_size, PointMPI, sched[i].first, 0, MPI_COMM_WORLD, &status);
            MPI_Send(current, arr_size, PointMPI, sched[i].first, 0, MPI_COMM_WORLD);
            
            // need only last $arr_size elements
            current = m_merge(current, received, arr_size, false, compare);
            
        }
    }
    
    
    // gathering sort results from all processors
    if (rank == 0) {
        result = new point[ arr_size * proc_count ];
    }
    MPI_Gather(current, arr_size, PointMPI, result, arr_size, PointMPI, 0, MPI_COMM_WORLD);
    
    sort_time = MPI_Wtime() - sort_time;
    
    double * times;
    if (rank == 0) {
        times = new double[proc_count];
    }
    MPI_Gather(&sort_time, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    
    // finalization
    if (rank == 0) {
        
        double resulting_time = 0;
        for (int i = 0; i < proc_count; ++i) {
            resulting_time += times[i];
        }
        
        write_time = MPI_Wtime();
        cout << "output_" << proc_count << "_" << size << "_" << sortX << endl;
        cout << "elem to sort: " << size << endl;
        cout << "init time: " << init_time << endl;
        cout << "sort time: " << resulting_time << endl;
        
        int i, k;
        i = 0;
        k = 0;
        while (i < arr_size * proc_count && k < size) {
            if (result[i].index != -1) {
                P[k] = result[i];
                ++k;
            }
            ++i;
        }
        delete [] result;
        
        for (i = 1; i < size; ++i) {
            if (sortX ? P[i-1].coord[0] > P[i].coord[0] : P[i-1].coord[1] > P[i].coord[1]) {
                cout << "Error in sort" << endl;
                break;
            }
        }
        
        cout << "write time: " << MPI_Wtime() - write_time << endl;
        
    }
    
    delete [] P;
    MPI_Finalize();
    
    return 0;
}
