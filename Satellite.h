
#include <unordered_map>
#include <set>
#include "utils.h"
#include <algorithm>

class Satellite{
    public:

    struct Schedule{    
        Request r;
        TimePoint observationStart;
        TimePoint observationEnd;
    }; 

    unsigned int id;
    unsigned int counter = 0;
    bool runningObservation = false;
    Vector3 position = {0};
    //Lon [0, 2*PI]
    float theta = 0;
    float radius = 5.5f;
    //Used to angle the orbit while drawing with respect to the x-axis
    // [-PI/2, PI/2];
    float inclination;
    //How fast we move
    unsigned int period = 0;
    float angularSpeed = 0.0561f;
    //We use this matrix to compute the movement of the satellite
    float earthRotation = 0.2f*DEG2RAD;
    Matrix rotationZ;
    Matrix rotation;
    Color color;
    //We save the strcutre of the constellation we are part of
    //we use a mapp indexed by the orbital angle which contains
    //the number of satellites and the period of each
    typedef std::unordered_map<int, std::tuple<float, unsigned int, unsigned int>> constellationMap;
    constellationMap map;
    unsigned int  idPrevious;
    unsigned int idNext;
    unsigned int alignmentStatus = 0;
    unsigned int wait = 0;

    unsigned int topologyPos = 0;
    std::set<unsigned int> topology;
    std::unordered_map<unsigned int, std::thread> communications;
    
    Satellite(){
        Satellite(0, 0.0561f);
    }

    Satellite(float inclination){
        Satellite(inclination, 0.0561f);
    }

    Satellite(float inclination, unsigned int period){
        position.z = radius;
        this->inclination = inclination;
        this->period = period;
        this->angularSpeed = static_cast<float>(period) * (2*PI) / 1800;
        
        rotationZ = MatrixRotateZ(inclination);
    }

    void initialize(float inclination, float theta, unsigned int period, bool clockwise){
        if(clockwise) angularSpeed = -angularSpeed;
        this->theta = theta;
        this->inclination = inclination;
        this->period = period;
        this->angularSpeed = static_cast<float>(period) * (2*PI) / 1800;
        rotationZ = MatrixRotateZ(inclination);
    }

   
    //Calculate the next position accordingly with theta and the inclination angle:
    //Imagine to move the satellite along the equator first, and then we rotate it
    //with the respect to the Y axis.
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
        for(unsigned int i = 0; i < 1800; i++){
            //We predict at each step where we are going to be
            //And the portion of earth that will be visible
            simTheta += angularSpeed;
            satellitePos = computePosition(simTheta, rotationZ);
            coneEnd = Vector3Scale(Vector3Normalize(satellitePos), 3.5);
            simRotation += earthRotation;
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
    
    std::vector<Schedule> computeVisibilites(Request& request, float theta, const Matrix& rotationZ){
        Vector3 destination = request.target;
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
        for(unsigned int i = 0; i < 1800; i++){
            //We predict at each step where we are going to be
            //And the portion of earth that will be visible
            simTheta += angularSpeed;
            satellitePos = computePosition(simTheta, rotationZ);
            coneEnd = Vector3Scale(Vector3Normalize(satellitePos), 3.5);
            simRotation += earthRotation;
            destPos = Vector3Transform(destination, MatrixRotateY(simRotation)); 
            if(Vector3Distance(destPos, coneEnd) < 0.5f){
                if(!obs){
                    obs = true;
                    counter++;
                }
            }else if(obs) obs = false; 
        }
        std::cout << id << " -> lo vedrò : "<< counter << " volte" << std::endl;
        std::vector<Schedule> q;
        return q;
    }

    void computeGeometricNeighborhoodDecomposition(Request* requests, int numOfReqq, int numOfNeighborhoods){
        int numOfReq = 2;
        int numOfOrbits = map.size();

        //Compute global supply 
        //The global supply for a request r is defined as the sum of a all supply(r, k)
        //for each orbit k
        float* supply[numOfReq];
        float totalSupply[numOfReq];
        for(int i = 0; i < numOfReq;  i++){
            supply[i] = new float[numOfOrbits];
            float orbitSupply[numOfOrbits];
            for(int j = 0; j < numOfOrbits; j++){
                // Calcolare Supply(req, orbit), bisection search
                std::vector schedule1 = computeVisibilites(requests[i], 0,MatrixRotateZ(std::get<0>(map[j])));
                std::vector schedule2 = computeVisibilites(requests[i], PI,MatrixRotateZ(std::get<0>(map[j])));
                supply[i][j] = orbitSupply[j];
                //Save the total as we iterate
                totalSupply[i] += orbitSupply[j];
            }
            //Save the total in order to help us with the sorting later
            supply[i][numOfOrbits] = totalSupply[i];
        } 

        //Compute Inter-Neighborhood Delegation
        //We start by sorting the requests by global supply 
        std::sort(totalSupply, totalSupply + numOfReq);
        for(int i = 0; i < numOfReq - 1;  i++){
            float tot = totalSupply[i];
            bool found = false;
            for(int k = i + 1; k < numOfReq && !found; k++){
                if(supply[k][numOfOrbits] == tot){
                    std::swap(supply[k], supply[i]);
                    found = true;
                }
            }   
        } 
        //Then we iterate to assign each request to a specific 
        //partition (here we divide the problems and the agents into sets effectively)
        std::vector<Request> partitions[numOfOrbits];
        for(int i = 0; i < numOfReq;  i++){
            float heuristc[numOfOrbits];
            for(int j = 0; j < numOfOrbits; j++){
                //Calcolare l'overlap
                int overlaps = 1;
                int numOfReqInPartition = partitions[j].size();
                for(int k = 0; k < numOfReqInPartition; k++){
                  // if(overlap(r, request.get())) overlaps++;
                }
                heuristc[j] = supply[i][j] / overlaps; 
            }
            //Choose the best n neighborhoods 
            int neighborhoods[numOfNeighborhoods];
            for(int k = 0; k < numOfNeighborhoods; k++){
                bool exclude = false;
                int maxPos = 0;
                for(int v = 0; v < numOfOrbits; v++){
                    for(int z = 0; z < k; z++) if(neighborhoods[z] == v) exclude = true;
                    if(heuristc[v] > heuristc[maxPos] && !exclude) maxPos = v;   
                }
                //insert the request to the best partitions
                neighborhoods[k] = maxPos;
                partitions[maxPos].push_back(requests[i]);
            } 
                
        }
        
        //Compute Intra-Neighborhood Delegation
        
    }

    //Messages
    
/** 
    void updateConstellationMap(std::unordered_map<float, std::tuple<float, float, unsigned int>> orbits){
        this->constellationMap.clear();
        this->constellationMap = orbits;
    }

    void addOrbitToMap(float orbitInclination, std::tuple<float, float, unsigned int> orbitParams){
        constellationMap.insert({orbitInclination, orbitParams});
    }

    void updateOrbitMap(float orbitInclination, std::tuple<float, float, unsigned int> newOrbitParams){
        constellationMap.erase(orbitInclination);
        constellationMap.insert({orbitInclination, newOrbitParams});
    }

    void removeOrbitFromMap(float orbitInclination){
        constellationMap.erase(orbitInclination);
    }
*/
};