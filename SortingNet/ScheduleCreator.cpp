//
//  ScheduleCreator.cpp
//  SortingNet
//
//  Created by anton on 22/11/2016.
//  Copyright © 2016 RandomStuff. All rights reserved.
//

#include "ScheduleCreator.h"

ScheduleCreator :: ~ScheduleCreator() {
    comparators.clear();
}

void ScheduleCreator :: join(vector<int> up, vector<int> down){
    size_t size = up.size() + down.size();
    if (size <= 1) {
        return;
    } else if (size == 2) {
        comparators.push_back(pair<int, int>(up[0], down[0]));
        return;
    }
    
    vector<int> up_even;
    vector<int> up_odd;
    
    for (size_t i = 0; i < up.size(); ++i) {
        if (i % 2) {
            up_even.push_back(up[i]);
        } else {
            up_odd.push_back(up[i]);
        }
    }
    
    vector<int> down_even;
    vector<int> down_odd;
    
    for (size_t i = 0; i < down.size(); ++i) {
        if (i % 2) {
            down_even.push_back(down[i]);
        } else {
            down_odd.push_back(down[i]);
        }
    }
    
    join(up_odd, down_odd);
    join(up_even, down_even);
    
    vector<int> result = up;
    
    result.insert(result.end(), down.begin(), down.end());
    
    for (size_t i = 1; i + 1 < result.size(); i += 2) {
        comparators.push_back(pair<int,int> (result[i], result[i + 1]));
    }
    
    up_even.clear();
    up_odd.clear();
    down_even.clear();
    down_odd.clear();
    result.clear();
}

void ScheduleCreator :: divide(vector<int> processors) {
    size_t size = processors.size();
    if (size <= 1) {
        return;
    }
    
    vector<int> processors_up;
    vector<int> processors_down;
    
    size_t i = 0;
    for (vector<int>::iterator iter = processors.begin(); iter < processors.end(); ++iter) {
        if (i == size / 2) {
            processors_up.assign(processors.begin(), iter);
            processors_down.assign(iter, processors.end());
            break;
        }
        ++i;
    }
    
    divide(processors_up);
    divide(processors_down);
    join(processors_up, processors_down);
    
    processors_up.clear();
    processors_down.clear();
}

vector< pair<int, int> > ScheduleCreator :: create_schedule( int size ) {
    vector<int> processors(size);
    
    for (int i = 0; i < size; ++i) {
        processors[i] = i;
    }
    
    divide(processors);
    processors.clear();
    
    return comparators;
}
