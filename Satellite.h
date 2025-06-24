
#include <unordered_map>
#include <set>
#include "utils.h"
#include <algorithm>

class Satellite{
    public:

    /*
    #####################################################################
    ##################         SATELLITE VARIABLES        ###############
    #####################################################################
    */

    float memory = 125000.0f; //total amount of memory in MB
    std::vector<Schedule> schedules;
    size_type index; //Used to identify the agent, as well as its bias.
    size_type totAgentsInOrbit; //The amount of agents in the orbit, also used to compute the bias
    size_type orbitPeriodicity; //The parameter p in the paper 
    size_type satelliteBias; 
    size_type numOfNeighborhoodsInOrbit; //How many bias will share a request fulfillment
    size_type orbitIndex; //The index of the orbit in the constellation
    size_type counter = 0;  //TO delete
    Vector3 position = {0};
    float theta = 0; //The longitutde [0, 2*PI]
    float inclination; //The altitude [-PI/2, PI/2];
    float radius = 10.79f;
    float nadir = 30*DEG2RAD; //Angle of visibility for the satellite
    //How fast we move
    size_type period = 0;
    float angularSpeed = 0.0561f;
    Matrix rotationZ; 
    Matrix rotation;
    Color color;
    //We save the structure of the constellation we are part of
    //we use a mapp indexed by the orbital angle which contains
    //the number of satellites and the period of each
    typedef std::unordered_map<int, std::tuple<float, size_type, size_type>> constellationMap;
    constellationMap map;
    TimePoint now;
    bool runningObservation = false;

    /*
    #####################################################################
    ##################         SATELLITE SETUP        ###################
    #####################################################################
    */
    
    Satellite(){
        Satellite(0, 0.0561f);
    }

    Satellite(float inclination){
        Satellite(inclination, 0.0561f);
    }

    Satellite(float inclination, size_type period){
        position.z = radius;
        this->inclination = inclination;
        this->period = period;
        this->angularSpeed = static_cast<float>(period) * (2*PI) / 1800;
        
        rotationZ = MatrixRotateZ(inclination);
    }

    void initialize(float inclination, float theta, size_type period, bool clockwise){
        if(clockwise) angularSpeed = -angularSpeed;
        this->theta = theta;
        this->inclination = inclination;
        this->period = period;
        this->angularSpeed = static_cast<float>(period) * (2*PI) / 1800;
        rotationZ = MatrixRotateZ(inclination);
    }


    /*
    #####################################################################
    ##################         SATELLITE MOTION        ##################
    #####################################################################
    */
   
    //Calculate the next position accordingly with theta and the inclination angle:
    //Imagine to move the satellite along the equator first, and then we rotate it
    //with the respect to the Z axis.
    //We do so by executing the formula 
    //newPostion = Rotation_Inclination * Rotation_Theta * initial_position
    //It's an over simplified motion for a satellite, but that's not key to the simulation 
    Vector3 computePosition(float theta, const Matrix& rotationZ){
        Vector3 newPostion{0}; 
        Matrix rotationY = MatrixRotateY(theta);
        rotation = MatrixMultiply(rotationY, rotationZ);
        newPostion = Vector3Transform({0.0f,0.0f,radius}, rotation);
        return newPostion;
    }

    //Update the satellite position in space.
    //It accepts a timeSpeed to regulate how fast the object is moving,
    //using it to update theta
    void updatePosition(float timeSpeed){
        theta += angularSpeed/timeSpeed;
        //Keep theta in range
        if(theta > 2*PI) theta = theta - 2*PI;
        this->position = computePosition(theta, this->rotationZ);
    }


    /*
    #####################################################################
    #################         SATELLITE PLANNING        #################
    #####################################################################
    */

    /*
    void calculateVisibilites(Vector3 destination){
        calculateVisibilites(destination, this->theta, rotationZ);
    }

    void calculateVisibilites(Vector3 destination, float theta, Matrix& rotationZ){
        std::cout << "X : " << destination.x << " | " << "Y : " << destination.y << " | " << "Z : " << destination.z << std::endl;
        //How many times we are going to see the objective
        int counter = 0;
        Vector3 satellitePos = {0};
        Vector3 destPos = {0};
        Vector3 coneEnd {0};
        float simTheta = theta;
        float simRotation = 0.0f;
        //Save weather or not we are already in an observation
        bool obs = false;
        for(size_type i = 0; i < 1800; i++){
            //We predict at each step where we are going to be
            //And the portion of earth that will be visible
            simTheta += angularSpeed;
            satellitePos = computePosition(simTheta, rotationZ);
            coneEnd = Vector3Scale(Vector3Normalize(satellitePos), 3.5);
            simRotation += EARTH_ROTATION_RATE;
            destPos = Vector3Transform(destination, MatrixRotateY(simRotation)); 
            if(Vector3Distance(destPos, coneEnd) < 0.5f){
                if(!obs){
                    obs = true;
                    counter++;
                }
            }else if(obs) obs = false; 
        }
        std::cout << id << " -> lo vedrò : "<< counter << " volte" << std::endl;
    }
    */
    //The logic to determine if a ground target is visibile from the satellite position
    //The first check is to avoid those cases where the target is behind earth and might have 
    //a angle inferior of the nadir angle 
    //The second check is acutally to see if the ground target is within the nadir angle
    bool isVisible(Vector3 satellitePos, Vector3 targetPos){
        //std::cout << "L'ho visto"<< std::endl;
        Vector3 coneEnd = Vector3Scale(Vector3Normalize(satellitePos), 10.0f);
        float coneRadius = Vector3Distance(satellitePos, coneEnd)*tanf(nadir/2);
        return Vector3Distance(Vector3Normalize(satellitePos), Vector3Normalize(targetPos)) <= coneRadius + 0.5f;
        //Vector3 dist = Vector3Normalize(Vector3Subtract(satellitePos, targetPos));
        //return (Vector3Angle(Vector3Normalize(satellitePos), Vector3Normalize(targetPos)) <= PI/2) 
        //    &&(Vector3Angle(Vector3Normalize(satellitePos), dist) <= this->nadir);
    }

    std::vector<Schedule> computeVisibilites(Request request, float theta, const Matrix& rotationZ){
        Vector3 destination = getCartesian(request.target.lat, request.target.lon, EARTH_RADIUS);
        std::vector<Schedule> schedules;
        //How many times we are going to see the objective
        int counter = 0;
        Vector3 satellitePos = {0};
        Vector3 targetPos = {0};
        float simTheta = theta;
        float simRotation = 0.0f;
        //Save weather or not we are already in an observation
        bool obs = false;
        int start = 0;
        TimePoint startTime;
        //For each minute within 24 hours we compute the visibility
        //and create possible schedules conseguently 
        size_type step = 0;
        for(; step < 1440; step++){
            //We predict at each step where we are going to be
            //And the portion of earth that will be visible
            simTheta += angularSpeed;
            satellitePos = computePosition(simTheta, rotationZ);
            simRotation += EARTH_ROTATION_RATE;
            targetPos = Vector3Transform(destination, MatrixRotateY(simRotation)); 
            if(isVisible(satellitePos, targetPos)){
                        if(!obs){
                            obs = true;
                            start = step;
                            counter++;
                            startTime = now.computeDateAfterElapsedTime(step);
                        }
                    }else if(obs){
                            obs = false;
                            TimePoint end = now.computeDateAfterElapsedTime(step);
                            std::cout << "Lo inizio a vedere alle "<< TextFormat("%d : %d : %d", startTime.day, startTime.hour, startTime.minute) << std::endl;
                            std::cout << "Lo finisco di vedere alle "<< TextFormat("%d : %d : %d", end.day, end.hour, end.minute) << std::endl;
                            Schedule schedule(request, startTime, end);
                            schedules.push_back(schedule);
                    }  
                }


        //handle the limit case where after exactly 24 hours we are in the middle of an observation
        //we have to finish computing the endTime for it.
        if(obs){
            while(isVisible(satellitePos, targetPos)){
                    simTheta += angularSpeed;
                    satellitePos = computePosition(simTheta, rotationZ);
                    simRotation += EARTH_ROTATION_RATE;
                    targetPos = Vector3Transform(destination, MatrixRotateY(simRotation)); 
                };
            Schedule schedule(request, startTime, now.computeDateAfterElapsedTime(step));
            schedules.push_back(schedule);
        }
        return schedules;
    }

    //Overload for computeVisibilites(Request request, float theta, const Matrix& rotationZ)
    std::vector<Schedule> computeVisibilites(Request request){
        return computeVisibilites(request, this->theta, this->rotationZ);
    }

    //Compute the Geometric Neiborhood Decomposition described in the paper
    std::vector<Request> computeGeometricNeighborhoodDecomposition(Request* requests, int numOfReqq, int numOfOrbitNeighborhoods, int numOfAgentNeighborhoods){
        int numOfReq = 2;
        int numOfOrbits = map.size();

        //Compute global supply 
        //The global supply for a request r is defined as the sum of a all supply(r, k)
        //for each orbit k.
        //supply(r, k) is defined as the time intervals of the observation opportunities times the number of agents in the orbit 
        //divided by the periodicity of the orbit
        float** supply = new float*[numOfReq];
        float totalSupply[numOfReq];
        for(size_type i = 0; i < numOfReq;  i++){
            supply[i] = new float[numOfOrbits+1];
            for(size_type j = 0; j < numOfOrbits; j++){
                std::vector<Schedule> schedule1 = computeVisibilites(requests[i], 0,MatrixRotateZ(std::get<0>(map[j])));
                std::vector<Schedule> schedule2 = computeVisibilites(requests[i], PI,MatrixRotateZ(std::get<0>(map[j])));
                float interval = 0.0f;
                size_type i;
                for(i = 0; i < schedule1.size(); i++) interval += schedule1[i].duration().toMinutes();
                for(i = 0; i < schedule2.size(); i++) interval += schedule2[i].duration().toMinutes();
                interval /= 2;
                //This log function here is a personal choise, will see if it remains
                interval = std::log(interval);
                float avgSupply = interval * std::get<1>(map[j]) / std::get<2>(map[j]);
                supply[i][j] = avgSupply;
                //Save the total as we iterate
                totalSupply[i] += avgSupply;
            }
            //Save also the total in order to help us with the sorting later
            supply[i][numOfOrbits] = totalSupply[i];
        } 

        //Compute Inter-Neighborhood Delegation
        //We start by sorting the requests by global supply 
        std::sort(totalSupply, totalSupply + numOfReq);
        for(size_type i = 0; i < numOfReq - 1;  i++){
            float tot = totalSupply[i];
            bool found = false;
            for(size_type k = i + 1; k < numOfReq && !found; k++){
                if(supply[k][numOfOrbits] == tot){
                    std::swap(supply[k], supply[i]);
                    found = true;
                }
            }   
        } 
        //Then we iterate to assign each request to a specific 
        //partition (here we divide the problem and the agents into sets effectively)
        std::vector<Request>* partitions = new std::vector<Request>[numOfOrbits];
        for(size_type i = 0; i < numOfReq;  i++){
            float heuristc[numOfOrbits];
            for(size_type j = 0; j < numOfOrbits; j++){
                //Calcolare l'overlap
                int overlaps = 0;
                int numOfReqInPartition = partitions[j].size();
                for(size_type k = 0; k < numOfReqInPartition; k++) if(requests[i].overlap(partitions[j][k])) overlaps++;
                heuristc[j] = supply[i][j] / overlaps; 
            }
            //Choose the best n neighborhoods 
            int numOfNeighborhoods = 4;
            int neighborhoods[numOfNeighborhoods];
            for(size_type k = 0; k < numOfNeighborhoods; k++){
                bool exclude = false;
                int maxPos = 0;
                for(size_type v = 0; v < numOfOrbits; v++){
                    for(size_type z = 0; z < k; z++) if(neighborhoods[z] == v) exclude = true;
                    if(heuristc[v] > heuristc[maxPos] && !exclude) maxPos = v;   
                }
                //insert the request to the best partitions
                neighborhoods[k] = maxPos;
                partitions[maxPos].push_back(requests[i]);
            } 
                
        }
        
        //Compute Intra-Neighborhood Delegation
        std::vector<Request> orbitRequests = partitions[orbitIndex];
        int numOfOrbitReq = orbitRequests.size();
        float** heuristicBias = new float*[numOfOrbitReq];
        float** heuristicPeriodic = new float*[numOfOrbitReq];
        for(size_type i = 0; i < numOfOrbitReq; i++){
            heuristicBias[i] = new float[totAgentsInOrbit];
            heuristicPeriodic[i] = new float[orbitPeriodicity];
            for(size_type j = 0; j < totAgentsInOrbit; j++){
                float latBias = static_cast<int>(orbitRequests[i].target.lat * 10) % orbitPeriodicity;
                //The formula 1 - pow((bias - latBias), 2) / pow(latBias, 2) is again a personal choise
                //the idea is that bigger bias differences lead to a worse lat/lon bias overall
                latBias = 1 - (pow((satelliteBias - latBias), 2) / pow(latBias, 2));
                float lonBias = static_cast<int>(orbitRequests[i].target.lon * 10) % orbitPeriodicity;
                lonBias = 1 - pow((satelliteBias - lonBias), 2) / pow(lonBias, 2);
                heuristicBias[i][j] = latBias + lonBias;
                heuristicPeriodic[i][j % orbitPeriodicity] += latBias + lonBias; 
            }
        }
        std::vector<Request>* finalPartition = new std::vector<Request>[orbitPeriodicity];
        int neighborhoods[numOfNeighborhoodsInOrbit];
        for(size_type i = 0; i < numOfOrbitReq; i++){
            for(int j = 0; j < numOfNeighborhoodsInOrbit; j++){
                int maxPos = 0;
                bool exclude = false;
                for(int k = 0; k < orbitPeriodicity; k++){
                    for(size_type z = 0; z < j; z++) if(neighborhoods[z] == k) exclude = true;
                    if(heuristicPeriodic[i][k] > heuristicPeriodic[i][maxPos] && !exclude) maxPos = k;
                }
                neighborhoods[j] = maxPos;
                finalPartition[maxPos].push_back(orbitRequests[i]);
            }
        }

        std::vector<Request> computedRequests = finalPartition[satelliteBias];

        //Free the memory
        delete[] supply;
        delete[] partitions;
        delete[] heuristicBias;
        delete[] finalPartition;

        return computedRequests;
    }

    /*
    #####################################################################
    ###############         SATELLITE MONITORING        #################
    #####################################################################
    */
    // CHECK AND MONITORING OF THE SATELLITE MISSION

    /**
    This function is useful since we have to discretize time(continuous) into steps(discrete), so 
    we can measure performances for observations that occur between them avoiding discretization related
    problems.
    */
    void checkMissionStatus(TimePoint now){
        std::vector completed = checkCompletedObs(now);
        size_type numCompleted = completed.size();
        if(numCompleted > 0){
            //Calculate the total memory used to perform the observations
            float usedMem = 0;
            for(size_type i = 0; i < numCompleted; i++){
                usedMem += completed[i].request.reqMemory;
            }
            //Subtract to the current available memory quantity
            memory -= usedMem;
        }

        //Calculate how much memory was freed if any
    }

    /**
    Check all the observations that were completed up until now.

    Future development: we still have to iterate each schedule, would be nice to use a set with a partial order
    over the elements or something like that
    */
    std::vector<Schedule> checkCompletedObs(TimePoint now){
        std::vector<Schedule> completed;
        size_type numOfSchedules = schedules.size();
        for(size_type i = 0; i < numOfSchedules; i++){
            //Simply determine if a schedule end time is before or equal to now
            if(schedules[i].observationEnd.isBeforeOrEqual(now)) completed.push_back(schedules[i]);
        }
        return completed;
    }

};