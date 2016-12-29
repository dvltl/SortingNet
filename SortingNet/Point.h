//
//  Point.h
//  SortingNet
//
//  Created by anton on 29/12/2016.
//  Copyright © 2016 RandomStuff. All rights reserved.
//

#ifndef Point_h
#define Point_h

struct Point {
    float coord[2];
    int index;
    
    static bool pcompareX (struct Point a, struct Point b) {
        float x1 = a.coord[0];
        float x2 = b.coord[0];
        
        if (a.index < 0) {
            return false;
        } else if (b.index < 0) {
            return true;
        }
        
        return (x1 < x2);
    }
    
    static bool pcompareY (struct Point a, struct Point b) {
        float y1 = a.coord[1];
        float y2 = b.coord[1];
        
        if (a.index < 0) {
            return false;
        } else if (b.index < 0) {
            return true;
        }
        
        return (y1 < y2);
    }
};

typedef struct Point point;

#endif /* Point_h */
