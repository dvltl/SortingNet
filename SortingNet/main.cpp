#include <iostream>
#include <vector>

using namespace std;

int compNum = 0;
/*
vector<pair<int, int>> sortAndMerge (vector<pair<int, int>> & fragmentSet, int itNum) {
    vector<pair<int, int>> result;
    //sort part
    if (itNum == 0){
        for (pair<int, int> entry : fragmentSet) {
            if (entry.first != entry.second){
                cout << entry.first << ' ' << entry.second << endl;
                ++compNum;
            }
        }
    } else {
        
    }
    
    //merge part
    for (int i = 0; i < fragmentSet.size(); i += 2) {
        result.push_back(pair<int, int>(fragmentSet[i].first, fragmentSet[i + 1].second));
    }
    
    return result;
}*/

void join(int start0, int start1, int step, int n0, int n1) {
    
    if(n0 + n1 > 2) {
        join(start0, start1, step * 2, (n0 + 1) / 2, (n1 + 1) / 2);
        join(start0 + step, start1 + step, step * 2, n0 - (n0 + 1) / 2, n1 - (n1 + 1) / 2);
        
        for(int i = start0 + step; i < (n0 + n1) * step; i += step * 2) {
            if (i + step < (n0 + n1) * step){
                cout << i << ' ' <<  i + step << endl;
                ++compNum;
            }
        }
    }
    else if(n0 + n1 == 2) {
        cout << start0 << ' ' << start1 << endl;
        ++compNum;
    }
    
}

void sort(int start, int step, int n) {
    int n0, n1; // n = n0 + n1
    n0 = n/2;
    n1 = n - n0;
    
    if(n > 2) {
        sort(start, step, n0);
        sort(start + n0 * step, step, n1);
        join(start, start + n0 * step, step, n0, n1);
    }
    else if(n == 2) {
        cout << start << ' ' << start + step << endl;
        ++compNum;
    }
    
}

int main(int argc, const char * argv[]) {
    if (argc < 2 || argc > 3) {
        cout << "Invalid number of parameters" << endl;
        return -1;
    }
    
    int n0 = stoi(argv[1]);
    int n1 = 0;
    //vector<pair<int, int>> fragmentSet;
    
    if (argc == 3) {
        n1 = stoi(argv[2]);
    }
    
    if ( !n1 ){
        sort(0, 1, n0);
/*
        int start = 0;
        int itNum = 0;
        
        if (n0 % 2) {
            start = 1;
            fragmentSet.push_back(pair<int, int> (0, 0));
        }
        
        for (int i = start; i < n0; i += 2) {
            fragmentSet.push_back(pair<int, int> (i, i + 1));
        }
        
        while (fragmentSet.size() > 1) {
            fragmentSet = sortAndMerge(fragmentSet, itNum++);
        }
 */
    } else {
        join(0, n0, 1, n0, n1);
/*
        fragmentSet.push_back(pair<int, int>(0, n0 - 1));
        fragmentSet.push_back(pair<int, int>(n0, n0 + n1));
        sortAndMerge(fragmentSet, 1);*/
    }
    cout << compNum << endl;
    return 0;
}
