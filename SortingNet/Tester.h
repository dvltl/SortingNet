//
//  Tester.h
//  SortingNet
//
//  Created by anton on 26/11/2016.
//  Copyright © 2016 RandomStuff. All rights reserved.
//

#ifndef Tester_h
#define Tester_h

class Tester {
private:
    void print_vector(const vector<int> &);
    void sort_n_check(const vector< pair<int,int> >&, vector< int >&, const vector< int >&);
public:
    int test(int, char*[]);
};

#endif /* Tester_h */
