#include <raylib.h>

#include <cmath>

typedef unsigned int size_type;

const float EARTH_RADIUS = 10.0f;
const float EARTH_ROTATION_RATE = 0.25f*DEG2RAD;
/*
Return coordinates in standard raylib right-hand coordinate system (Y vertical, X orizontal and Z towards you)
I adapted the formulas in order to map XYZ to ZXY and so that the coordinates (0,0,radius) are along the Z axis.
https://en.wikipedia.org/wiki/Spherical_coordinate_system
*/
Vector3 getCartesian(float theta, float phi, float radius){
    //return {radius * sinf(phi) * sinf(theta),
    //        radius * cosf(phi),
    //        radius * sinf(phi) * cosf(theta)
    //        };
    return {radius * sinf(theta) * cosf(phi),
            radius * sinf(phi),
            radius * cosf(theta) * cosf(phi)
            };
}

//TimePoint is used to discretize each time istant (which for us is in the order of the minutes)
//No checks are applied since the use is pretty straight forward 
   struct TimePoint{
        int day; 
        int hour; 
        int minute;

        TimePoint():day(0),hour(0),minute(0){}
        TimePoint(int day, int hour, int minute):day(day),hour(hour),minute(minute){}
        TimePoint(int elapsedMinutes):day(elapsedMinutes%1440),hour(elapsedMinutes/24),minute(elapsedMinutes % 60){
        }

        bool isBeforeOrEqual(TimePoint p){
            if (day < p.day) return true;
            if (day > p.day) return false;
            if (hour < p.hour) return true;
            if (hour > p.hour) return false;
            return minute <= p.minute;
        }

        TimePoint computeDateAfterElapsedTime(int elapsedMinutes){
            int mm = elapsedMinutes % 60;
            int hh = elapsedMinutes / 60;
            int dd = day;
            mm = (mm + minute) % 60;
            hh = (hh + hour) % 24 + ((mm + minute) / 60);
            dd += elapsedMinutes / 1440 + ((hh + hour) / 24);
            return TimePoint(dd,hh,mm);
        }

        TimePoint computeTimeDifference(TimePoint point){
            int minutes = toMinutes();
            int pointMinutes = point.toMinutes();
            return TimePoint(pointMinutes - minutes);
        }

        int toMinutes(){
           return (minute) + 60 * (hour) + 1440 * (day); 
        }
    };


struct Target{
    float lat;
    float lon;

    Target():lat(0.0f),lon(0.0f){};
    Target(float lat, float lon):lat(lat),lon(lon){};
    Target(Vector3 target):lat(0.0f),lon(0.0f){};
};


class Request{
    public: 
    Target target;
    TimePoint start;
    TimePoint end;
    float radius;
    //Required memory in MB
    float reqMemory;

    //Request(){};

    Request(Target target, float reqMemory, float radius, TimePoint start,TimePoint end)
            :target(target),reqMemory(reqMemory),radius(radius),start(start),end(end) {}

    bool overlap(Request r){
        TimePoint s = r.start;
        TimePoint e = r.end;

        if(end.isBeforeOrEqual(s) || e.isBeforeOrEqual(start)) return false;
    }
};



 struct Schedule{    
        Request request;
        TimePoint observationStart;
        TimePoint observationEnd;

        Schedule(Request req, TimePoint start, TimePoint end):request(req),observationStart(start),observationEnd(end){}

        TimePoint duration(){
            return observationStart.computeTimeDifference(observationEnd);
        }
    }; 