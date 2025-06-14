#include <raylib.h>

   struct TimePoint{
        int day; 
        int hour; 
        int minute;

        bool isBefore(TimePoint p){
            if (day < p.day) return true;
            if (day > p.day) return false;
            if (hour < p.hour) return true;
            if (hour > p.hour) return false;
            return minute < p.minute;
        }
    };

class Request{
    public: 
    Vector3 target;
    TimePoint start;
    TimePoint end;
    float reqMemory;

    Request(Vector3 target, float reqMemory, int startD, int startH, int startM, int endD, int endH, int endM){
        this->target = target;
        this->reqMemory = reqMemory;
        start.day = startD;
        start.hour = startH;
        start.minute = startM;
        end.day = endD;
        end.hour = endH;
        end.minute = endM;
    }

    static bool doRequestsOverlap(Request r1, Request r2){
        TimePoint s1 = r1.start;
        TimePoint s2 = r2.start;
        TimePoint e1 = r1.end;
        TimePoint e2 = r2.end;

        if(e1.isBefore(s2) || e2.isBefore(s1)) return false;
    }
};