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
#include <fstream>
#include <sstream>

using namespace std;

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

int sort_with_net(int rank, int proc_count, const vector< pair<int,int> > & sched, const MPI_Datatype & PointMPI) {
    const int init_recv = 2;
    int * params = new int[init_recv];
    MPI_Bcast(params, init_recv, MPI_INT, proc_count - 1, MPI_COMM_WORLD);
    
    if (params[0] == -1) {
        return -1;
    }
    
    // params[0] - size, params[1] - sortX
    point p;
    bool (*compare)(point, point);
    
    if (params[init_recv - 1]) {
        compare = &p.pcompareX;
    } else {
        compare = &p.pcompareY;
    }
    int arr_size = params[0];
    
    MPI_Status status;
    vector< point > to_sort(arr_size);
    
    MPI_Recv(&to_sort[0], arr_size, PointMPI, proc_count - 1, 0, MPI_COMM_WORLD, &status);
    
    vector< point > sub(arr_size);
    point * received = new point[arr_size];
    point * buf = &sub[0];
    point * pbuf;
    point * current;
    
    // initial sort
    
    init_sort(&to_sort[0], arr_size, rank, compare);
    
    // CURRENT
    current = &to_sort[0];
    
    // batcher sorting net part
    
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
    
    MPI_Send(current, arr_size, PointMPI, proc_count - 1, 0, MPI_COMM_WORLD);
    
    delete [] received;
    to_sort.clear();
    sub.clear();
    
    delete [] params;
    
    return 0;
}

void add_cut(vector<point> & arr, int & cut, const int & n1, const int & n2, const bool sortedX) {
    cut += sortedX ? n1 : n2;
}

// TODO: will work bad with ( desired_domains_number % 2 == 1 )
void rec_bisect(vector<point> & arr, int n1, int n2, const int & min_size,
                const int & proc_count, vector< pair<point, int> > & domains,
                int & k, int & cut, const MPI_Datatype & PointMPI) {
    int params[2];
    if (arr.size() / 2 >= min_size) {
        MPI_Status status;
        int init_size = arr.size();
        params[0] = (n1 * n2) / (proc_count - 1);
        
        
        float maxX = INT_MIN, minX = INT_MAX, maxY = INT_MIN, minY = INT_MAX;
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].coord[0] > maxX) {
                maxX = arr[i].coord[0];
            }
            if (arr[i].coord[0] < minX) {
                minX = arr[i].coord[0];
            }
            if (arr[i].coord[1] > maxY) {
                maxY = arr[i].coord[1];
            }
            if (arr[i].coord[1] < minY) {
                minY = arr[i].coord[1];
            }
        }
        
        params[1] = (maxX - minX) > (maxY - minY);
        
        MPI_Bcast(params, 2, MPI_INT, proc_count - 1, MPI_COMM_WORLD);
        
        for (int i = 0; i < proc_count - 1; ++i) {
            MPI_Send(&arr[i * params[0]], params[0], PointMPI, i, 0, MPI_COMM_WORLD);
        }
        
        for (int i = 0; i < proc_count - 1; ++i) {
            MPI_Recv(&arr[i * params[0]], params[0], PointMPI, i, 0, MPI_COMM_WORLD, &status);
        }
        
        add_cut(arr, cut, n1, n2, params[1]);
        
        vector<point> arr_left(arr.begin(), arr.begin() + init_size / 2);
        vector<point> arr_right(arr.begin() + init_size / 2, arr.end());
        arr.clear();
        
        if (params[1]) {
            n2 /= 2;
        } else {
            n1 /= 2;
        }
        
        rec_bisect(arr_left, n1, n2, min_size, proc_count, domains, k, cut, PointMPI);
        rec_bisect(arr_right, n1, n2, min_size, proc_count, domains, k, cut, PointMPI);
    } else {
        ++k;
        for (int i = 0; i < arr.size(); ++i){
            domains.push_back(pair<point,int>(arr[i], k));
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Correct usage: ./a.out <n1> <n2> [ 0 | !0 ] (depending on coordinate to sort)" << endl;
        return -1;
    }
    
    int n1 = atoi(argv[1]);
    int n2 = atoi(argv[2]);
    int k = atoi(argv[3]);
    
    int size = n1 * n2;
    
    point p;
    
    int cut = 0;
    double bisect_time;
    
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
    
    vector< pair<point,int> > domains;
    
    bisect_time = MPI_Wtime();
    
    if (rank != proc_count - 1) {
        // creating schedule for processors to act as batcher sorting net
        ScheduleCreator creator;
        vector< pair<int,int> > sched = creator.create_schedule(proc_count - 1);

        while( sort_with_net(rank, proc_count, sched, PointMPI) != -1 );
    } else {
        // initial array creation
        vector< point > arr(size);
        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            arr[i].coord[0] = i / n2;
            arr[i].coord[1] = i % n2;
            arr[i].index = i;
        }
        
        const int min_size = (n1 * n2) / k;
        int cur_dom = 0;
        
        // TODO: bad work with size : (size % (proc_count - 1) != 0)
        rec_bisect(arr, n1, n2, min_size, proc_count, domains, cur_dom, cut, PointMPI);
        
        int params[2] = {-1, -1};
        MPI_Bcast(params, 2, MPI_INT, proc_count - 1, MPI_COMM_WORLD);
    }
    
    bisect_time = MPI_Wtime() - bisect_time;
    
    double * times;
    if (rank == proc_count - 1) {
        times = new double[proc_count];
    }
    MPI_Gather(&bisect_time, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, proc_count - 1, MPI_COMM_WORLD);
    
    
    // finalization
    if (rank == proc_count - 1) {
        
        double resulting_time = 0;
        for (int it = 0; it < proc_count; ++it) {
            resulting_time += times[it];
        }
        
        cout << "output_" << proc_count << endl;
        cout << "elem to sort: " << size << endl;
        cout << "total bisect time: " << resulting_time << endl;
        cout << "average bisect time: " << resulting_time / proc_count << endl;
        cout << "edges cut: " << cut << endl;
        
        string out;
        stringstream ss;
        
        cout << "Started output" << endl;
        for (int i = 0; i < domains.size(); ++i) {
            p = domains[i].first;
            ss << p.index / n2 << ' ' << p.index % n2 << ' ' << p.coord[0] << ' ' << p.coord[1] << ' ' << domains[i].second << endl;
            out += ss.str();
            ss.str(string());
        }
        
        ofstream results;
        ss << proc_count;
        string filename = "output_" + string(argv[1]) + "_" + string(argv[2]) + "_" + string(argv[3]) + "_" + ss.str();
        results.open(filename.c_str());
        results << out;
        cout << "Output finished" << endl;
        results.close();
    }
    
    MPI_Finalize();
    
    return 0;
}
