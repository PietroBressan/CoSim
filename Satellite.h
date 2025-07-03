

#include <unordered_map>
#include <set>
#ifndef UTILS   
#define UTILS
#include "utils.h"
#endif
#include "MessageBroker.h"
#include <algorithm>
#include <map>
using namespace std;

class Satellite{

    public:

    /*
    #####################################################################
    ##################         SATELLITE VARIABLES        ###############
    #####################################################################
    */

    float maxMemory = 125000.0f; //total amount of memory in MB
    float currMemory = 0.0f;
    vector<Schedule> mission; //Our current mission
    size_type index; //Used to identify the agent, as well as its bias.
    size_type totAgentsInOrbit; //The amount of agents in the orbit, also used to compute the bias
    size_type agentPeriodicity; //The parameter p in the paper 
    size_type satelliteBias; 
    vector<vector<Schedule>> biasMissions; //The mission of the other agents with the same bias
    size_type totOrbitNeighborhood; //How many orbits should fulfill a request
    vector<Target> groundStations;
    size_type orbitIndex; //The index of the orbit in the constellation
    size_type counter = 0;  //TO delete
    Vector3 position = {0};
    float theta = 0; //The longitutde [0, 2*PI]
    float inclination; //The altitude [-PI/2, PI/2];
    float radius = 10.79f;
    float nadir = 30*DEG2RAD; //Angle of visibility for the satellite
    //How fast we move
    MessageBroker broker;
    Color color;
    size_type orbitPeriodicity = 0;
    float angularSpeed = 0.0561f;
    Matrix rotationZ; 
    Matrix rotation;
    //We save the structure of the constellation we are part of
    //we use a mapp indexed by the orbital angle which contains
    //the number of satellites and the period of each
    std::map<size_type, OrbitParameters> constellationMap;
    TimePoint now;
    bool runningObservation = false;
    int lost = 0;

    /*
    #####################################################################
    ##################         SATELLITE SETUP        ###################
    #####################################################################
    */
    

    Satellite(float inclination, float theta, TimePoint creationTime,
                size_type index, bool clockwise,
                size_type orbitPeriodicity, size_type orbitIndex,  
                size_type totAgentsInOrbit, size_type agentPeriodicity,
                size_type satelliteBias, size_type totOrbitNeighborhood,
                MessageBroker& broker){
        if(clockwise) angularSpeed = -angularSpeed;
        this->inclination = inclination;
        this->index = index;
        this->orbitPeriodicity = orbitPeriodicity;
        this->totAgentsInOrbit = totAgentsInOrbit;
        this->satelliteBias = satelliteBias;
        this->agentPeriodicity = agentPeriodicity;
        this->orbitIndex = orbitIndex;
        this->totOrbitNeighborhood = totOrbitNeighborhood;
        this->theta = theta;
        this->position = getCartesian(theta, inclination, EARTH_RADIUS);
        this->broker = broker;
        now = creationTime;
        this->angularSpeed = static_cast<float>(orbitPeriodicity) * (2*PI) / 1440;
        rotationZ = MatrixRotateZ(inclination);
    }

    void updateConstellationMap(std::map<size_type, OrbitParameters> constellationMap){
        this->constellationMap = constellationMap;
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
        //theta += angularSpeed/timeSpeed;
        ////Keep theta in range
        //if(theta > 2*PI) theta = theta - 2*PI;
        theta = fmod(theta + (angularSpeed / timeSpeed), 2*PI);
        this->position = computePosition(theta, this->rotationZ);
    }


    /*
    #####################################################################
    #################         SATELLITE PLANNING        #################
    #####################################################################
    */

    void generateMission(vector<Request>& requests){
        vector<Request> planReq;
        vector<Schedule> finalMission;
        /*//Compute the heuristic in order to achieve a subset of the requests
        vector<Request> planReq = computeGeometricNeighborhoodDecomposition(requests);
        mission.clear();
        //Planning dei downlink
        // *********

        //Save how much memory do we have 
        vector<Schedule> downlinks;
        //How much memory are we going to use
        vector<float> requiredMemory(downlinks.size()); 
        //How much memory can we downlink at each station
        vector<float> freedMemory(downlinks.size()); 
        //Create an initial solution 
        size_type nReq = planReq.size();
        requiredMemory[0] = currMemory;
        for(size_type i = 0; i < nReq; i++){
            vector<Schedule> possibleSchedules = computeVisibilites(planReq[i]);
            bool scheduled = false;
            //Future work, what to do if we can't schedule?
            for(size_type j = 0; j  < possibleSchedules.size() && !scheduled; j++){
                for(int k = 0; k < downlinks.size() && ! scheduled; k++){
                    if(requiredMemory[k] - freedMemory[k] < maxMemory){
                        bool overlap = false;
                        for(size_type z = 0; z < finalMission.size() && !overlap z++){
                            if(possibleSchedules[j].overlap(finalMission[z])) overlap = true;
                        }
                        if(!overlap){
                            requiredMemory[k] += possibleSchedules[j].request.reqMemory;
                            finalMission.push_back(possibleSchedules[j]);
                            scheduled = true;
                    }
                    //if(end.isBeforeOrEqual(s) || e.isBeforeOrEqual(start)) return false;
                }
            }
        }
        if(!scheduled) lost++;
    }
        

        int maxIterations = 500;
        for(int i = 0; i < maxIterations; i++){
            for(int j = 0; j < totOrbitNeighborhood; j++){
                //broker.sendMessage(Message(index, 
                //                    orbitNeighborhoods[j], 
                //                    orbitIndex, 
                //                    finalMission));
                int x = 0;
            }
            random_shuffle(planReq.begin(), planReq.end());
        }
*/
    }

    //Available request should be a class property if we want to separate things correctly
    //This function should be called after the orbitMission is updated ! 
    void improveMissionQuality(vector<Request> availableRequest){
        size_type numOfReq = availableRequest.size();
        //Here we save how many agents are currently scheduling each request
        std::map<size_type, size_type> coverage;
        for(size_type i = 0; i < biasMissions.size(); i++){
            vector<Schedule> agentMission = biasMissions[i];
            size_type numOfSchedules = agentMission.size();
            for(size_type j = 0; j < numOfSchedules; j++)
                coverage[agentMission[j].request.id]++;
        }
        //Now we perform the stochasticUpdate 
        random_shuffle(availableRequest.begin(), availableRequest.end());
        for(size_type i = 0; i < numOfReq; i++){
            //Check if the mission is already assigned
            bool alreadyAssigned = false;
            for(size_type j = 0; j < mission.size(); j++){
                if(mission[j].request.id == availableRequest[i].id) alreadyAssigned = true;
            }

            if(!alreadyAssigned && coverage[availableRequest[i].id] == 0){
                //Schedule request r
            } else if(alreadyAssigned){
                bool stillAssigned = true;
                float sample = rand();
                if(coverage[availableRequest[i].id] == 0) stillAssigned = sample < 0.7f;
                else stillAssigned = sample < (coverage[availableRequest[i].id] - 1) / coverage[availableRequest[i].id];
                //if(!stillAssigned) //unschedule request
            }

        }
    }
   

    //The logic to determine if a ground target is visibile from the satellite position
    //The first check is to avoid those cases where the target is behind earth and might have 
    //a angle inferior of the nadir angle 
    //The second check is acutally to see if the ground target is within the nadir angle
    bool isVisible(Vector3 satellitePos, Vector3 targetPos){
        //float zoneHeight = radius*EARTH_RADIUS / (radius+EARTH_RADIUS);

        //cout << "L'ho visto"<< endl;
        //Vector3 coneEnd = Vector3Scale(Vector3Normalize(satellitePos), 10.0f);
        //float coneRadius = Vector3Distance(satellitePos, coneEnd)*tanf(nadir);
        //return Vector3Distance(Vector3Normalize(satellitePos), Vector3Normalize(targetPos)) <= coneRadius + 0.5f;
        Vector3 dist = Vector3Normalize(Vector3Subtract(satellitePos, targetPos));
        return (Vector3Angle(Vector3Normalize(satellitePos), Vector3Normalize(targetPos)) <= PI/4) 
            &&(Vector3Angle(Vector3Normalize(satellitePos), dist) <= this->nadir);
    }


vector<Schedule> computeVisibilites(Request request, float theta, float angularSpeed, const Matrix& rotationZ){
        Vector3 destination = getCartesian(request.target.lon, request.target.lat, EARTH_RADIUS);
        vector<Schedule> schedules;
        TimePoint requestStart = request.start;
        TimePoint requestEnd = request.end;
        int diff = requestStart.computeTimeDifference(requestEnd).toMinutes();
        cout << "Start "<< TextFormat("%d : %d : %d", requestStart.day, requestStart.hour, requestStart.minute) << endl;
        cout << "End  "<< TextFormat("%d : %d : %d", requestEnd.day, requestEnd.hour, requestEnd.minute) << endl;
         cout << "diff  "<< diff << endl;           
        Vector3 satellitePos = position;
        Vector3 targetPos = destination;
        float simTheta = theta;
        float simRotation = 0.0f;
        //Save weather or not we are already in an observation
        bool obs = false;
        size_type startStep;
        TimePoint startObservationTime;
        // We use the geometry and the time difference between now and the 
        // start of the request to compute the agent and target position
        // at start. 
        size_type skip = (now.computeTimeDifference(request.start).toMinutes() % 1440);
        cout << "skip  = " << skip << endl;
        if(skip > 0){
            simTheta = fmod(simTheta + angularSpeed*skip, 2*PI);
            satellitePos = computePosition(simTheta, rotationZ);
            simRotation = fmod(EARTH_ROTATION_RATE*skip, 2*PI);
            targetPos = Vector3Transform(destination, MatrixRotateY(simRotation));
        }               
        size_type step = 0;
        for(; step < diff || obs; step++){
            if(isVisible(satellitePos, targetPos)){
                if(!obs){
                    obs = true;
                    startStep = step;
                    startObservationTime = requestStart.computeDateAfterElapsedTime(step);
                }
            }else if(obs){
                    obs = false;
                    TimePoint endObservationTime = startObservationTime.computeDateAfterElapsedTime(step - startStep);
                    //cout << diff << endl;
                    
                    cout << "Lo inizio a vedere alle "<< TextFormat("%d : %d : %d", startObservationTime.day, startObservationTime.hour, startObservationTime.minute) << endl;
                    cout << "Lo finisco di vedere alle "<< TextFormat("%d : %d : %d", endObservationTime.day, endObservationTime.hour, endObservationTime.minute) << endl;
                    Schedule schedule(request, startObservationTime, endObservationTime);
                    schedules.push_back(schedule);
            }
            //We again predict at each step where we are going to be
            //but this time we also check the portion of earth that will be visible
            simTheta = fmod(simTheta + angularSpeed, 2*PI);
            satellitePos = computePosition(simTheta, rotationZ);
            simRotation = fmod(simRotation + EARTH_ROTATION_RATE, 2*PI);
            targetPos = Vector3Transform(destination, MatrixRotateY(simRotation));
        }
        
        return schedules;
    }
//Overload for computeVisibilites(Request request, float theta, const Matrix& rotationZ)
    vector<Schedule> computeVisibilites(Request request){
        return computeVisibilites(request, this->theta, this->angularSpeed, this->rotationZ);
    }

    /*
    vector<Schedule> computeVisibilitesOLD(Request request, float theta, float angularSpeed, const Matrix& rotationZ){
        Vector3 destination = getCartesian(request.target.lon, request.target.lat, EARTH_RADIUS);
        vector<Schedule> schedules;
        //How many times we are going to see the objective
        int counter = 0;
        Vector3 satellitePos = {0};
        Vector3 targetPos = {0};
        float simTheta = theta;
        float simRotation = 0.0f;
        //Save weather or not we are already in an observation
        bool obs = false;
        TimePoint startTime;
        size_type startStep;
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
                            startStep = step;
                            counter++;
                            startTime = now.computeDateAfterElapsedTime(step);
                            //cout << index << "| req: "<< request.id << " | Start step "<< step << endl;
                            
                        }
                    }else if(obs){
                            obs = false;
                            TimePoint end = startTime.computeDateAfterElapsedTime(step - startStep);
                            //cout << index << "| req: "<< request.id << " | End step "<< step << endl;
                            cout << index << "| "<<request.id << ": Lo inizio a vedere alle "<< TextFormat("%d : %d : %d", startTime.day, startTime.hour, startTime.minute) << endl;
                            cout << index << "| "<<request.id << ": Lo finisco di vedere alle "<< TextFormat("%d : %d : %d", end.day, end.hour, end.minute) << endl;
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
            Schedule schedule(request, startTime, startTime.computeDateAfterElapsedTime(step - startStep));
            schedules.push_back(schedule);
        }
        return schedules;
    }


    //Overload for computeVisibilitesOLD(Request request, float theta, const Matrix& rotationZ)
    vector<Schedule> computeVisibilites(Request request){
        return computeVisibilites(request, this->theta, this->angularSpeed, this->rotationZ);
    }

    */
    
    //Compute the Geometric Neiborhood Decomposition heuristic described in the paper
    vector<Request> computeGeometricNeighborhoodDecomposition(vector<Request> request){
        int numOfReq = request.size();
        int numOfOrbits = constellationMap.size();

        //Compute global supply 
        //The global supply for a request r is defined as the sum of a all supply(r, k)
        //for each orbit k.
        //supply(r, k) is defined as the time intervals of the observation opportunities times the number of agents in the orbit 
        //divided by the periodicity of the orbit
        vector<vector<float>> supply(numOfReq);
        vector<float> totalSupply(numOfReq);
        for(size_type i = 0; i < numOfReq;  i++){
            for(size_type j = 0; j < numOfOrbits; j++){
                vector<Schedule> schedule1 = computeVisibilites(request[i], 0, constellationMap[j].angularSpeed, MatrixRotateZ(constellationMap[j].inclination));
                vector<Schedule> schedule2 = computeVisibilites(request[i], PI, constellationMap[j].angularSpeed, MatrixRotateZ(constellationMap[j].inclination));
                float interval = 0.0f;
                //size_type i;
                for(size_type k = 0; k < schedule1.size(); k++) interval += schedule1[k].duration().toMinutes();
                for(size_type k = 0; k < schedule2.size(); k++) interval += schedule2[k].duration().toMinutes();
                interval /= 2;
                //Here the formula for supply(r, k) Da Cambiare !!
                float avgSupply = interval * constellationMap[j].numOfSatellite / constellationMap[j].orbitPeriod;
                supply[i].push_back(avgSupply);
                //Save the total as we iterate
                totalSupply[i] += avgSupply;
            }
            //Save also the total in order to help us with the sorting later
            supply[i].push_back(totalSupply[i]);
            supply[i].push_back(request[i].id);
        } 
        

std::cout << "controllo valori di supply" <<std::endl;
        for(size_type v = 0; v < numOfOrbits; v++){
            for(int j= 0; j < numOfOrbits; j++){    
                std::cout << supply[v][j] << " || ";
            }
            std::cout << std::endl;
        }

        //Compute Inter-Neighborhood Delegation
        //We start by sorting the requests by global supply 
        std::sort(totalSupply.begin(), totalSupply.end());
        for(size_type i = 0; i < numOfReq - 1;  i++){
            float tot = totalSupply[i];
            bool found = false;
            for(size_type k = i + 1; k < numOfReq && !found; k++){
                if(supply[k][numOfOrbits] == tot){
                    swap(supply[k], supply[i]);
                    found = true;
                }
            }   
        } 
/** 

        //Then we iterate to assign each request to a specific 
        //partition (here we divide the problem and the agents into sets effectively)
        vector<vector<Request>> partitions(numOfOrbits);
        for(size_type i = 0; i < numOfReq;  i++){
            vector<float> heuristc(numOfOrbits);
            bool indexFound = false;
            size_type requestPos = 0;
            for(size_type h = 0; h < numOfReq && !indexFound; h++){
                if(request[i].id == supply[h][numOfOrbits+1]){
                    indexFound = true;
                    requestPos = h;
                }
            }
            for(size_type j = 0; j < numOfOrbits; j++){
                //Calcolare l'overlap
                int overlaps = 0;
                int numOfReqInPartition = partitions[j].size();
                for(size_type k = 0; k < numOfReqInPartition; k++) if(request[requestPos].overlap(partitions[j][k])) overlaps++;
                heuristc.push_back(supply[requestPos][j] / overlaps); 
            }
            //Choose the best n neighborhoods 
            //We use alreadySelected to store the previous values in order to avoid duplicates
            vector<size_type> alreadySelected(totOrbitNeighborhood);
            for(size_type k = 0; k < totOrbitNeighborhood; k++){
                bool exclude = false;
                size_type maxPos = 0;
                for(size_type v = 0; v < numOfOrbits; v++){
                    for(size_type z = 0; z < k; z++) if(alreadySelected[z] == v) exclude = true;
                    if(heuristc[v] > heuristc[maxPos] && !exclude) maxPos = v;   
                }
                //insert the request to the best partitions
                alreadySelected.push_back(maxPos);
                partitions[maxPos].push_back(request[i]);
            } 
                
        }
std::cout << "controllo delegazione tra orbite" <<std::endl;
        for(size_type v = 0; v < numOfOrbits; v++){
            std::cout << v<<std::endl;
            for(int j= 0; j < partitions[v].size(); j++){    
                std::cout << partitions[v][j].id << " || ";
            }
            std::cout << std::endl;
        }

        //Compute Intra-Neighborhood Delegation
        vector<Request> partitionRequests = partitions[orbitIndex];
        int numOfOrbitReq = partitionRequests.size();
        //vector<vector<float>> heuristicBias(numOfOrbitReq);
        std::map<size_type, float> heuristicPeriodic;
        
        for(size_type i = 0; i < numOfOrbitReq; i++){
            for(size_type j = 0; j < totAgentsInOrbit; j++){
                float latBias = static_cast<int>(partitionRequests[i].target.lat * 10) % agentPeriodicity;
                //The formula 1 - pow((bias - latBias), 2) / pow(latBias, 2) is again a personal choise
                //the idea is that bigger bias differences lead to a worse lat/lon bias overall
                latBias = 1 - (pow((satelliteBias - latBias), 2) / pow(latBias, 2));
                float lonBias = static_cast<int>(partitionRequests[i].target.lon * 10) % agentPeriodicity;
                lonBias = 1 - pow((satelliteBias - lonBias), 2) / pow(lonBias, 2);
                //heuristicBias[i].push_back(latBias + lonBias);
                heuristicPeriodic[j % agentPeriodicity] += latBias + lonBias; 
            }
        }

        std::map<size_type, vector<Request>> finalPartition;
        vector<size_type> neighborhoods(totOrbitNeighborhood);
        for(size_type i = 0; i < numOfOrbitReq; i++){
            for(size_type j = 0; j < totOrbitNeighborhood; j++){
                size_type maxPos = 0;
                bool exclude = false;
                for(size_type k = 0; k < agentPeriodicity; k++){
                    for(size_type z = 0; z < j; z++) if(neighborhoods[z] == k) exclude = true;
                    if(heuristicPeriodic[k] > heuristicPeriodic[maxPos] && !exclude) maxPos = k;
                }
                neighborhoods[j] = maxPos;
                finalPartition[maxPos].push_back(partitionRequests[i]);
            }
        }

        vector<Request> computedRequest = finalPartition[satelliteBias];
        std::cout << " Computed : ";
        for(int  i = 0; i < computedRequest.size(); i++)std::cout << computedRequest[i].id << " ";
        std::cout << std::endl;
        //Free the memory
*/
vector<Request> computedRequest;
        return computedRequest;
        
    }

    /*
    #####################################################################
    ###############         SATELLITE MONITORING        #################
    #####################################################################
    */
    

    /**
    This function is useful since we have to discretize time(continuous) into steps(discrete), so 
    we can measure performances for observations that occur between them avoiding discretization related
    problems.
    */
    void checkMissionStatus(TimePoint now){
        vector completed = checkCompletedObs(now);
        size_type numCompleted = completed.size();
        if(numCompleted > 0){
            //Calculate the total memory used to perform the observations
            float usedMem = 0;
            for(size_type i = 0; i < numCompleted; i++){
                usedMem += completed[i].request.reqMemory;
            }
            //Subtract to the current available memory quantity
            currMemory -= usedMem;
        }

        //Calculate how much memory was freed if any
    }

    
    /**
    Check all the observations that were completed up until now.

    Future development: we still have to iterate each schedule, would be nice to use a set with a partial order
    over the elements or something like that
    */
    vector<Schedule> checkCompletedObs(TimePoint now){
        vector<Schedule> completed;
        size_type numOfSchedules = mission.size();
        for(size_type i = 0; i < numOfSchedules; i++){
            //Simply determine if a schedule end time is before or equal to now
            if(mission[i].observationEnd.isBeforeOrEqual(now)) completed.push_back(mission[i]);
        }
        return completed;
    }

};