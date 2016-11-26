#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "ScheduleCreator.h"
#include "Tester.h"

using namespace std;

void Tester :: print_vector(const vector<int>& vec) {
    for (size_t i = 0; i < vec.size(); ++i) {
        cout << vec[i] << ' ';
    }
    cout << endl;
}

void Tester :: sort_n_check(const vector< pair<int,int> >& sched, vector< int >& to_sort, const vector< int >& answer) {
    
//    cout << "To sort:" << endl;
//	print_vector(to_sort);
//	cout << "Right way:" << endl;
//	print_vector(answer);
    
    int buf;
    
    for (size_t i = 0; i < sched.size(); ++i) {
        if (to_sort[ sched[i].first ] > to_sort[ sched[i].second ]) {
            buf = to_sort[ sched[i].first ];
            to_sort[ sched[i].first ] = to_sort[ sched[i].second ];
            to_sort[ sched[i].second ] = buf;
        }
    }
    
    bool errors = to_sort.size() != answer.size();
    
    for (size_t i = 0; i < to_sort.size(); ++i) {
        if (to_sort[i] != answer[i]) {
//            cout << "Error in sorting or swaping: to_sort["<< i <<"] is " << to_sort[i] << " and answer["<< i << "] is " << answer[i] << endl;
            errors = true;
        }
    }
    
    if (!errors) {
        cout << "All okay!" << endl;
    } else {
        cout << "All is bad!!!" << endl;
    }
}

int Tester :: test(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Not enough params" << endl;
        return -1;
    }
    cout << "Starting" << endl;

    ScheduleCreator creator;
    vector< pair<int, int> > sched;
/*
    for (size_t i = 0; i < sched.size(); ++i) {
        cout << '(' << sched[i].first << ' ' << sched[i].second << ") ";
    }
    cout << endl;*/

    ifstream file ("test");
    string str;
    string sub;
    vector< vector<int> > to_sort;
    vector< vector<int> > answer;
    vector<int> buf;

    if (file.is_open()) {
        int i = 0;
        while (getline(file, str)) {
            stringstream sstr(str);

            if (!i % 2) {
                while (sstr >> sub) {
                    buf.push_back( stoi(sub) );
                }
                to_sort.push_back(buf);
                buf.clear();
            } else {
                while (sstr >> sub) {
                    buf.push_back( stoi(sub) );
                }
                answer.push_back(buf);
                buf.clear();
            }
            ++i;
        }
        file.close();
    }
    
    for (size_t i = 0; i < to_sort.size(); ++i) {
        sched = creator.create_schedule(to_sort[i].size());
        
        sort_n_check(sched, to_sort[i], answer[i]);
        
        to_sort[i].clear();
        answer[i].clear();
    }
    
    return 0;
}
