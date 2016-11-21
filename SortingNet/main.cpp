#include <vector>
#include <iostream>
#include "ScheduleCreator.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Not enough params" << endl;
        return -1;
    }
    cout << "Starting" << endl;

    ScheduleCreator creator;
    vector< pair<int, int> > sched = creator.create_schedule(stoi(argv[1]));
    
    for (size_t i = 0; i < sched.size(); ++i) {
        cout << '(' << sched[i].first << ' ' << sched[i].second << ") ";
    }
    
    cout << endl;
    return 0;
}
