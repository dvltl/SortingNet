//
//  ScheduleCreator.h
//  SortingNet
//
//  Created by anton on 22/11/2016.
//  Copyright © 2016 RandomStuff. All rights reserved.
//

#ifndef ScheduleCreator_h
#define ScheduleCreator_h

#include <vector>
using namespace std;

class ScheduleCreator {
private:
    vector< pair<int, int> > comparators;
    void join( vector<int>, vector<int> );
    void divide( vector<int> );
    void better_ticks();
public:
    ~ScheduleCreator();
    vector< pair<int, int> > create_schedule( int );
};

#endif /* ScheduleCreator_h */
