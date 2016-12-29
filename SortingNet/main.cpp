//
//  main.cpp
//  SortingNet
//
//  Created by anton on 26/11/2016.
//  Copyright © 2016 RandomStuff. All rights reserved.
//

#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <climits>
#include "ScheduleCreator.h"
#include "Point.h"

using namespace std;

float get_float(float i, float j) {
    return (float)(i + j) / 2;
}

void m_merge(const point * first, const point * second, point * sub, int size, bool head, bool (*comp)(point, point)) {
    int i, j, k;
    
    if (head) {
        i = 0;
        j = 0;
        
        for (k = 0; k < size; ++k){
            if (comp(first[i], second[j])) {
                sub[k] = first[i++];
            } else {
                sub[k] = second[j++];
            }
        }
    } else {
        i = size - 1;
        j = size - 1;
        
        for (k = size - 1; k >= 0; --k) {
            if (!comp(first[i], second[j])) {
                sub[k] = first[i--];
            } else {
                sub[k] = second[j--];
            }
        }
        
    }
    
}

void init_sort(point * to_sort, int arr_size, int rank, bool (*compare)(point, point)) {
    int part_num = 4;
    int part_size = arr_size / part_num;
    
    #pragma omp parallel for
    for (int i = 1; i < part_num; ++i){
        sort(&to_sort[0] + (i - 1) * part_size, &to_sort[0] + i * part_size, compare);
    }
    
    vector< vector<point> > sort_buf;
    vector< vector<point> > buf;
    
    for (int i = 0; i < part_num / 2; ++i){
        buf.push_back(vector<point>(2 * part_size));
    }
    
    for (int i = 0; i < part_num; ++i){
        sort_buf.push_back(vector<point>(part_size));
    }
    
    for (int i = 1; i <= part_num; ++i){
        copy(&to_sort[0] + (i - 1) * part_size, &to_sort[0] + i * part_size, (sort_buf[ i - 1 ]).begin());
    }
    
    #pragma omp parallel for
    for (int i = 1, k = 0; i < part_num; i += 2) {
        merge(sort_buf[ i - 1 ].begin(), sort_buf[ i - 1 ].end(),
              sort_buf[ i ].begin(), sort_buf[ i ].end(),
              buf[ k++ ].begin(), compare);
    }
    
    merge(buf[0].begin(), buf[0].end(),
          buf[1].begin(), buf[1].end(),
          &to_sort[0], compare);


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
    point p;
    
    if (sortX) {
        compare = &p.pcompareX;
    } else {
        compare = &p.pcompareY;
    }
    
    int size = n1 * n2;
    
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
    disp[0] = reinterpret_cast<const unsigned char*>(&p.coord[0]) - reinterpret_cast<const unsigned char*>(&p);
    disp[1] = reinterpret_cast<const unsigned char*>(&p.index) - reinterpret_cast<const unsigned char*>(&p);
    
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
    
    int i, j;
    
    for (int it = rank * arr_size; it < min((rank + 1) * arr_size, size); ++it) {
        i = it / n2;
        j = it % n2;
        p.coord[0] = get_float(i, j);
        p.coord[1] = get_float(i, j);
        p.index = it;
        to_sort.push_back(p);
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
    
    point * received = new point[arr_size];
    point * buf = new point[arr_size];
    point * pbuf;
    point * current;
    
    init_time = MPI_Wtime() - init_time;
    
    // initial sort
    sort_time = MPI_Wtime();
    
    init_sort(&to_sort[0], arr_size, rank, compare);
    
    // CURRENT
    current = &to_sort[0];
    
    // batcher sorting net part
    MPI_Status status;
    
    for (size_t i = 0; i < sched.size(); ++i) {
        if (rank == sched[i].first) {
            MPI_Send(current, arr_size, PointMPI, sched[i].second, 0, MPI_COMM_WORLD);
            MPI_Recv(received, arr_size, PointMPI, sched[i].second, 0, MPI_COMM_WORLD, &status);
            
            // need only first $arr_size elements
            m_merge(current, received, buf, arr_size, true, compare);

            pbuf = current;
            current = buf;
            buf = pbuf;
            
        } else if (rank == sched[i].second) {
            MPI_Recv(received, arr_size, PointMPI, sched[i].first, 0, MPI_COMM_WORLD, &status);
            MPI_Send(current, arr_size, PointMPI, sched[i].first, 0, MPI_COMM_WORLD);
            
            // need only last $arr_size elements
            m_merge(current, received, buf, arr_size, false, compare);

            pbuf = current;
            current = buf;
            buf = pbuf;
        }
    }
    
    sort_time = MPI_Wtime() - sort_time;

    delete [] received;
    to_sort.clear();
    
    double * times;
    if (rank == 0) {
        times = new double[proc_count];
    }
    MPI_Gather(&sort_time, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    
    // finalization
    if (rank == 0) {
        
        double resulting_time = 0;
        for (int it = 0; it < proc_count; ++it) {
            resulting_time += times[it];
        }
        
        write_time = MPI_Wtime();
        cout << "output_" << proc_count << "_" << size << "_" << sortX << endl;
        cout << "elem to sort: " << size << endl;
        cout << "init time: " << init_time << endl;
        cout << "sort time: " << resulting_time << endl;
        cout << "comparators: " << sched.size() << endl;
        cout << "write time: " << MPI_Wtime() - write_time << endl;
        
    }
    
    MPI_Finalize();
    
    return 0;
}
