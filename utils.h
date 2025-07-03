#include <raylib.h>
#include <cmath>
#include <random>

typedef unsigned int size_type;

const float EARTH_RADIUS = 10.0f;
const float EARTH_ROTATION_RATE = 0.25f*DEG2RAD;


std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
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

struct OrbitParameters{
        size_type id;
        float inclination;
        float angularSpeed;
        size_type numOfSatellite;
        size_type orbitPeriod;

        OrbitParameters(size_type id, float inclination, float angularSpeed, size_type numOfSatellite, size_type orbitPeriod):
                        id(id),
                        inclination(inclination),
                        angularSpeed(angularSpeed),
                        numOfSatellite(numOfSatellite),
                        orbitPeriod(orbitPeriod){};

        OrbitParameters():id(0),inclination(0),angularSpeed(0),numOfSatellite(0),orbitPeriod(0){};
    };

//TimePoint is used to discretize each time istant (which for us is in the order of the minutes)
//No checks are applied since the use is pretty straight forward 
   struct TimePoint{
        int day; 
        int hour; 
        int minute;

        TimePoint():day(0),hour(0),minute(0){}
        TimePoint(int day, int hour, int minute):day(day),hour(hour),minute(minute){}
        TimePoint(int elapsedMinutes):day(elapsedMinutes/1440),hour((elapsedMinutes/60)%24),minute(elapsedMinutes % 60){
        }

        bool isBeforeOrEqual(TimePoint p){
            if (day < p.day) return true;
            if (day > p.day) return false;
            if (hour < p.hour) return true;
            if (hour > p.hour) return false;
            return minute <= p.minute;
        }

        TimePoint computeDateAfterElapsedTime(int elapsedMinutes){
            return TimePoint(toMinutes()+elapsedMinutes);
        }
        
        //The argument is assumed to be in a TimePoint in the future
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
    Vector3 position;

    Target():lat(0.0f),lon(0.0f){};
    Target(float lat, float lon):lat(lat),lon(lon){};
    Target(float lat, float lon, Vector3 position):lat(lat),lon(lon),position(position){};
};


class Request{
    public: 
    Target target;
    TimePoint start;
    TimePoint end;
    bool isGroundStation;
    size_type id;
    //Required memory in MB
    float reqMemory;

    //Request(){};

    Request(size_type id, Target target, float reqMemory, bool isGroundStation, TimePoint start,TimePoint end)
            :id(id),target(target),reqMemory(reqMemory),isGroundStation(isGroundStation),start(start),end(end) {}

    bool overlap(Request r){
        TimePoint s = r.start;
        TimePoint e = r.end;

        if(end.isBeforeOrEqual(s) || e.isBeforeOrEqual(start)) return false;
        else return true;
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


    /**
Generate a campaign composed of many requests each with a time interval to satisfy
also need to add the memory
*/
void generateRequests(std::vector<Request>& reqs, int numberOfReq, TimePoint startTime, TimePoint temporalWindow, int minDuration){
    if (!reqs.empty()) reqs.clear();
    float randomTheta = 0.0f;
    float randomPhi = 0.0f;
    TimePoint randomStart(0,0,0);
    TimePoint randomEnd(0,0,0);
    int randomTimeInterval = startTime.computeTimeDifference(startTime.computeDateAfterElapsedTime(temporalWindow.toMinutes())).toMinutes(); 
    for(size_type i = 0; i < numberOfReq; i++){
        randomTheta = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX))*2*PI;
        randomPhi = -PI/2 + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX))*PI;
        Vector3 randPos = getCartesian(randomTheta, randomPhi, EARTH_RADIUS);
        while(randomEnd.toMinutes() - randomStart.toMinutes() < minDuration){
            randomStart = TimePoint(static_cast<int>(dist(gen)*randomTimeInterval));
            randomEnd = TimePoint(static_cast<int>(dist(gen)*randomTimeInterval));
        }    
        Target t(randomPhi, randomTheta, randPos);
        Request r(i, t, 10.0f, false, randomStart, randomEnd); 
        reqs.push_back(r);
    }
}