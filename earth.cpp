#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <iostream>
#include <stdlib.h>
#include <ctime>

#include <cmath>
#include <chrono>
#include <queue>
#include <thread>
#define RAYGUI_IMPLEMENTATION
#ifndef ICONS   
#define ICONS
#define RAYGUI_SUPPORT_ICONS
#endif
#include <../include/raygui.h>
#include "Satellite.h"
#include "Plot.h"
//PARAMETERS
const int WIDTH = 1000;
const int HEIGHT = 730;

//Earth parameters
float earthRotation = 0.0f;
// useless float earthInclination = 0.4014257f;
//Problem instance parameters
int numOfSatellite = 150;
int numOfPlanes = 15;
float spacing = 0.5f * 360 / numOfSatellite * DEG2RAD;
int numOfReq = 2;
std::vector<Request> request;
//Camera and time parameters
int zoom = 0;
float cameraTheta = 0;
float cameraPhi = 0;
float cameraRadius = 40.0f;
bool track = false;
Vector2 startMouseTrack = {0};
Vector2 oldMousePos = {0};
float mouseSensitivity = 0.0025f;
float mouseSpeed = 0.0f;
bool showPlanes = false;
//30 seconds = 24 hours at timeSpeed = 1.0f;
float timeSpeed = 1.0f;
int step = 0;
int minutes = 0;
int hours = 0;
int days = 0;
bool showMore = false;
Plot leftRight({100.0f,100.0f}, {250.0f, 250.0f});
float panelWidth = 200.0f;
int counter = 0;
bool obs = false;
bool infoAnimation = false;
size_type infoSlide = 0;
MessageBroker broker;
std::chrono::steady_clock::time_point beginPoint = std::chrono::steady_clock::now();

bool gui = false;
bool stop = true;
bool restart = false;
bool newConstellation = false;
std::vector<float> constInclination;
std::vector<int> constSize;
char inclinationInput[64] = "";
float numOfAgentInOrbit = 0.0f;
bool isActive = true;
Rectangle panelRec = { (WIDTH/3)+10, (HEIGHT/2)+30, (WIDTH/3)-20, 100};
Rectangle panelContentRec = {0, 0, (WIDTH/3)-20, 200};
Rectangle panelView = { 0 };
Vector2 panelScroll = { 0, 0 };


bool newCampaign = false;
float newNumOfReq;

bool randomConfiguration = false;
/**
https://en.wikipedia.org/wiki/Spherical_coordinate_system

TODO List:

- finire l'algoritmo
- migliorare isVisibile
- implementare le ground station per lo scarico dei dati + comunicazione

- migliorare il metodo isVisible
- aggiungere fmod per il round degli angoli
- Aggiungere un raggio ai target, attualmente sono dei punti

*/

/**
Handle commands for camera movement and time control
*/
void handleCommands(Camera& camera){

    
    //Pause the simulation
        if (IsKeyPressed(KEY_SPACE)){
            stop = !stop;
        }
        //Speed up or slow down the scene
        if (IsKeyPressed(KEY_Z) && timeSpeed < 1.0f) timeSpeed *= 2;
        if (IsKeyPressed(KEY_X) && timeSpeed > 0.5f) timeSpeed /= 2;
        if (IsKeyPressed(KEY_X) ) timeSpeed /= 2;
        if (IsKeyPressed(KEY_Z) ) timeSpeed *= 2;
        //Handle time
        if(!stop){
            //To rotate the earth of 360 degrees it takes 1800 frames with a fixed rotation rate of 0.2f.
            //We have 60 frames per seconds at speed 1
            //for a total of 30 seconds to complete a full rotation (one solar day = 30 secs with timeSpeed = 1)
            step += 60 / timeSpeed;
            if (step >= 86400){

            
             std::cout << "=================================" << std::endl;
                std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::seconds> (end - beginPoint).count() << "[s]" << std::endl;
                beginPoint = std::chrono::steady_clock::now();
                days++;
                step = step - 86400;
            }
            hours = step / 3600;
            minutes = step % (3600) / 60;
        }
        if(GetMousePosition().y < 640 && !gui){
        //Zooming
        zoom = GetMouseWheelMove();
        if(zoom){
            float newDepth = (cameraRadius-zoom);
            newDepth *= newDepth;
            if(newDepth > 110.0f && newDepth < 2500.0f ) {
                cameraRadius -= zoom;
                camera.position = getCartesian(cameraTheta, cameraPhi, cameraRadius);
                UpdateCamera(&camera, CAMERA_PERSPECTIVE);
            }  
        } 
        //Move the camera around
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !showMore){
            Vector2 mousePos = GetMousePosition();
            if(track){
                Vector2 diff = Vector2Normalize(Vector2Subtract(mousePos, oldMousePos));
                mouseSpeed = Vector2Length(Vector2Subtract(mousePos, oldMousePos));
                if(mouseSpeed > 0.05f){
                    
                    cameraTheta -= (diff.x)*mouseSpeed*mouseSensitivity;
                    cameraPhi += (diff.y)*mouseSpeed*mouseSensitivity;
                    if(cameraTheta > 2*PI) cameraTheta = 0;
                    if(cameraTheta < 0) cameraTheta = 2*PI;
                    if(cameraPhi < -1.4f) cameraPhi = -1.4f;
                    else if(cameraPhi > 1.4f) cameraPhi = 1.4f;
                    camera.position = getCartesian(cameraTheta, cameraPhi, cameraRadius);
                UpdateCamera(&camera, CAMERA_PERSPECTIVE);
                }
            }
            oldMousePos = mousePos;
            if(!track){
                startMouseTrack = mousePos;
                track = true;
            }
        }else track = false;
        //Reset position in space
        if (IsKeyPressed(KEY_R)) {
            cameraTheta = 0;
            cameraPhi = 0;
            cameraRadius = 40.0f;
            camera.position = getCartesian(cameraTheta, cameraPhi, cameraRadius);
            UpdateCamera(&camera, CAMERA_PERSPECTIVE);
        }
        //Show / Hide orbital trajectories (circles)
        if (IsKeyPressed(KEY_P)) showPlanes = !showPlanes;
        //Generate new targets
        if (IsKeyPressed(KEY_M)) generateRequests(request, numOfReq, TimePoint(days, hours, minutes), TimePoint(days+2, hours, minutes), 60);
    }

}

void handleInfos(){
    if(!infoAnimation && showMore && infoSlide == 0) infoAnimation = true;
    if(infoAnimation && !showMore && infoSlide == 0) infoAnimation = false;
    if(infoSlide < WIDTH && showMore && infoAnimation) infoSlide += 50;
    if(infoSlide >= 0 && !showMore && infoAnimation) infoSlide -= 50;
    DrawRectangle(0,0,infoSlide, HEIGHT, {255,255,255,200});
    // std::vector<float> data {1.0f, 2.0f,3.0f, 444.0f,25.0f, 6.0f,7.0f, 8.0f,8.0f, 2.0f};
    // std::vector<float> data {1.0f, 2.0f,3.0f, 4.0f,5.0f, 6.0f,7.0f, 8.0f,9.0f, 10.0f};
    std::vector<float> dataX {0.0f, 1.0f, 2.0f,3.0f, 4.0f,5.0f, 6.0f,7.0f, 8.0f,9.0f, 10.0f};
    std::vector<float> dataY {0.0f, 1.0f, 2.0f,3.0f, 4.0f,5.0f, 6.0f,7.0f, 8.0f,9.0f, 10.0f};
    if(infoSlide == WIDTH && showMore) leftRight.draw(dataX, dataY);
    if (infoSlide == WIDTH && GuiButton((Rectangle){ 24, 24, 120, 30 }, "#141#Show Less")) showMore = !showMore;
}

void generateConstellation(vector<Satellite>& satellite, int numOfSatellite, int numOfPlanes, float spacing){
    if(!satellite.empty()) satellite.clear();
    broker.clear(); 
    float inclinationDiff = PI / numOfPlanes;
    int numOfSatellitesPerOrbit = numOfSatellite / numOfPlanes;
    int diff = numOfSatellite - (numOfSatellitesPerOrbit * numOfPlanes);
    std::map<size_type, OrbitParameters> constellation; 
    size_type index = 0;
    
    for(size_type i = 0; i < numOfPlanes; i++){
        int iterations = numOfSatellitesPerOrbit;
        int period = 14+(static_cast<int>(rand()) / static_cast<float>(RAND_MAX))*6;
        // int period = 16;
        bool clockwise = rand() > 0.5;
        if(diff > 0){
            iterations++;
            diff--;
        }
        float satelliteDistance = 2 * PI / iterations;
        constellation[i] = OrbitParameters(i, inclinationDiff*i, static_cast<float>(period) * (2*PI) / 1440, iterations, period);
        for(size_type j = 0; j < iterations; j++){
            Satellite agent = Satellite(inclinationDiff*i, 
                (satelliteDistance*j) + (spacing*i), 
                TimePoint(),
                j,
                clockwise,
                period, 
                i,
                1,
                1,
                0,
                1,
                broker);
            satellite.push_back(agent);
            index++;
        }
    }

    for(size_type i = 0; i < numOfSatellite; i++) satellite[i].updateConstellationMap(constellation);
}

int main(){
    srand(time(0));
    generateRequests(request, numOfReq, TimePoint(days, hours, minutes), TimePoint(days+2, hours, minutes), 60);
    vector<Satellite> satellite;
    request[0].target.lat = 0;
    request[0].target.lon = PI/2;
    request[0].id = 33 ;
    request[0].target.position = getCartesian(PI/2, 0, EARTH_RADIUS);
    request[0].start = TimePoint(0,0,0);
     request[0].end = TimePoint(1,0,0);
     request[1].start = TimePoint(0,0,0);
     request[1].end = TimePoint(1,0,0);
    request[1].target.lat = PI/2;
    request[1].id = 10;
    request[1].target.lon = PI/2;
    request[1].target.position = getCartesian(PI/2, PI/2, EARTH_RADIUS);
    generateConstellation(satellite, numOfSatellite, numOfPlanes, spacing);
    //[1] = getCartesian(0,0,EARTH_RADIUS); 
    satellite[0].color = LIME;
    //satellite[0].initialize(0, 0, 16, false);
    satellite[0].index = 9;
    //satellite[1].index = 321;
    //Request r(t, 10.0f, 5.0f, TimePoint().computeDateAfterElapsedTime(100), TimePoint().computeDateAfterElapsedTime(1540));
    Vector3 groundStation = getCartesian(0,0,EARTH_RADIUS-2.0f);
    InitWindow(WIDTH, HEIGHT, "Earth Model");
    SetWindowMinSize(WIDTH, HEIGHT);
    SetWindowMaxSize(WIDTH, HEIGHT);
    Camera camera = {getCartesian(cameraTheta, cameraPhi, cameraRadius), { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, CAMERA_PERSPECTIVE};
    SetTargetFPS(60); 
    TimePoint DIFF = TimePoint(days, hours, minutes).computeDateAfterElapsedTime(TimePoint(2, hours, minutes).toMinutes());
    // std::cout << "MIN "<< TimePoint(2, hours, minutes).toMinutes() << std::endl;
    // std::cout << "DIFF "<< TextFormat("%d : %d : %d", DIFF.day, DIFF.hour, DIFF.minute) << std::endl;
    newNumOfReq = numOfReq;
    bool quit = false;
    //satellite[0].computeGeometricNeighborhoodDecomposition(reqs);
bool check = true;

        
        satellite[1].computeVisibilites(request[1]);
        // satellite[1].computeVisibilites(request[1]);
        vector<Request> test;   
        // test = satellite[0].computeGeometricNeighborhoodDecomposition(request);
    while(!WindowShouldClose() || quit){


if(!stop){
Vector3 dist = Vector3Normalize(Vector3Subtract(satellite[1].position, request[1].target.position));
if(satellite[1].isVisible(satellite[1].position, request[1].target.position) && check){
    check = false;  
    if(days == 0 && minutes == 0 && hours == 0)   std::cout << "Found !!!"<< TextFormat("%d : %d : %d", days, hours, minutes) << std::endl;
    else std::cout << "Found "<< TextFormat("%d : %d : %d", days, hours, minutes) << std::endl;
    }else check = true;
// Vector3 dist = Vector3Normalize(Vector3Subtract(satellite[0].position, request[0].target.position));
// if(satellite[0].isVisible(satellite[0].position, request[0].target.position) && check){
//     check = false;    
//     std::cout << "Found "<< TextFormat("%d : %d : %d", days, hours, minutes) << std::endl;
//     }else check = true;
}


        //Resume/Pause simulation
        if(!stop){
            //For each satellite orbiting we call the updatePosition method
            for(int i = 0; i < numOfSatellite; i++){
                satellite[i].updatePosition(timeSpeed);
            }
            earthRotation += EARTH_ROTATION_RATE*RAD2DEG/timeSpeed;
            
             for(int i = 0; i < numOfReq; i++){
                request[i].target.position = Vector3Transform(request[i].target.position, MatrixRotateY(EARTH_ROTATION_RATE/timeSpeed));
            }
            groundStation = Vector3Transform(groundStation, MatrixRotateY(EARTH_ROTATION_RATE/timeSpeed));
            //Return to 0 to avoid overflow (just in case)
            if(earthRotation >= 360.0f) earthRotation = earthRotation - 360.0f;
      
        }


        BeginDrawing();
            ClearBackground(BLACK);
            BeginMode3D(camera);
            //Draw Earth
            rlPushMatrix();
            rlRotatef(earthRotation, 0.0, 1.0, 0);    
            //DrawSphereWires({0.0f,0.0f,0.0f}, EARTH_RADIUS, 50, 50, BLUE);
           // DrawSphereEx({0.0f,0.0f,0.0f}, EARTH_RADIUS, 25,25, { 0, 121, 241, 220 });
            DrawSphere({0.0f,0.0f,0.0f}, EARTH_RADIUS, { 0, 121, 241, 220 });
            DrawLine3D({0.0f,0.0f,0.0f}, {15.0f,0.0f,0.0f}, YELLOW);
            DrawLine3D({0.0f,0.0f,0.0f}, {0.0f,15.0f,0.0f}, GREEN);
            DrawLine3D({0.0f,0.0f,0.0f}, {0.0f,0.0f,15.0f}, WHITE);  
            
            //Draw Ground Stations
            rlRotatef(90, 1.0f, 0.0f, 0.0f); 
            DrawCylinderWires(getCartesian(0.0f, PI/2, 8.0f), 1.55f, 1.13f, 3.0f, 60, RED);
            for(float i = 1; i < 20; i++) DrawCircle3D(getCartesian(0.0f, PI/2, 11.0f), 1.55f-(0.1f*i), {1.0f,0.0f,0.0f}, 90, RED);

            rlPopMatrix();

            
            for(int i = 0; i < numOfReq; i++){
                bool red = false;
                //for(int j = 0; j < test.size(); j++) //if(test[j].id == request[i].id) red = true;
                red = true;
                DrawCapsule(request[i].target.position, request[i].target.position, 0.1f, 1, 1, red ? RED : GREEN );
            }
            
            //Draw satellites
            for(int i = 0; i < numOfSatellite; i++){
                DrawCube(satellite[i].position, 0.2f, 0.2f, 0.2f, satellite[i].color);
                Vector3 coneEnd = Vector3Scale(Vector3Normalize(satellite[i].position), EARTH_RADIUS);
                float radiusEnd = Vector3Distance(satellite[i].position, coneEnd)*tanf(satellite[i].nadir);
                DrawCylinderEx(satellite[i].position,coneEnd,0.01f, radiusEnd, 128, Fade(YELLOW, 0.5));
            }

            EndMode3D();


            //GUI
            if (!infoAnimation && GuiButton((Rectangle){ 24, 24, 120, 30 }, "#141#Show Statistics")) showMore = !showMore;
            if(infoAnimation || showMore){
                handleInfos();
            }
            DrawRectangle(800,650,200, 50, {255,255,255,128});
            DrawRectangle(0, 640 , WIDTH, HEIGHT, { 255, 203, 0, 255 });
            DrawText(TextFormat("%d : %d : %d", days, hours, minutes), 800, 35, 30, (showMore && infoAnimation) ? DARKGREEN : GREEN);
            DrawText("Time Speed", 18, 665, 15, BLACK);
            if(GuiButton((Rectangle){ 44, 685, 30, 30 }, (stop)?"#131#":"#132#")) stop = !stop;
            if(GuiButton((Rectangle){ 12, 685, 30, 30 }, "#129#") && timeSpeed < 2.0f) timeSpeed *= 2;
            if(GuiButton((Rectangle){ 76, 685, 30, 30 }, "#134#") && timeSpeed > 0.25f) timeSpeed /= 2;
            if(timeSpeed == 2.0f) DrawRectangle(12, 685, 30, 30, {255, 255, 255, 100});
            if(timeSpeed == 0.25f) DrawRectangle(76, 685, 30, 30, {255, 255, 255, 100});
            if (GuiButton((Rectangle){ 130, 655, 120, 30 }, "#211#Restart")) stop = !stop;
            if (GuiButton((Rectangle){ 130, 690, 120, 30 }, "#157#New Constellation")) newConstellation = !newConstellation;
            if (GuiButton((Rectangle){ 275, 655, 120, 30 }, "#66#New Campaign")) newCampaign = !newCampaign;
            if (GuiButton((Rectangle){ 275, 690, 120, 30 }, "#78#Random Setup")) stop = !stop;
            
            if(newConstellation){
                gui = true;
                DrawRectangle(0,0, WIDTH, HEIGHT, {255,255,255,200});
                int result = GuiMessageBox((Rectangle){ WIDTH/3,HEIGHT/4, WIDTH/3, HEIGHT/2},
                    "#191#Generate New Constellation","", "BACK;CONFIRM");
                GuiLabel((Rectangle){ 355, 300, 70, 30 },"Inclination\n(degrees)");
                GuiTextBox((Rectangle){ 435, 300, 150, 30 }, inclinationInput, 64, true);
                int orbitInclination = TextToInteger(inclinationInput);
                GuiLabel((Rectangle){ 600, 300, 70, 30 },"[-90,+90]");
                GuiLabel((Rectangle){ 355, 250, 70, 30 },"Number of\nagents");
                //GuiSpinner((Rectangle){ 425, 300, 120, 30 }, "", &numOfAgentInOrbit, 1, 50, false);
                GuiSlider((Rectangle){ 435, 250, 150, 30 }, "", "", &numOfAgentInOrbit, 1.0f, 500.0f);
                int orbitSize = static_cast<int>(numOfAgentInOrbit);
                GuiLabel((Rectangle){ 600, 250, 150, 30 },TextFormat("%d", static_cast<size_type>(numOfAgentInOrbit)));
                if (GuiButton((Rectangle){ 450, 355, 100, 20 }, "Add Orbit")){
                    std::cout << "Hello" << std::endl;
                    constInclination.push_back(orbitInclination);
                    constSize.push_back(orbitSize);
                }
                bool comma = false;
                for(int i = 0; i < 64; i++) {
                    if(i == 0 && inclinationInput[0] == '-')inclinationInput[0] = '-';
                    else if(inclinationInput[i] < '0' || inclinationInput[i] > '9') {
                        if(inclinationInput[i] == ',' && !comma) comma = true;
                        else inclinationInput[i] = '\0';
                    }
                }
                panelContentRec.height = constSize.size()*30;
                GuiScrollPanel(panelRec, NULL, panelContentRec, &panelScroll, &panelView);

                BeginScissorMode(panelView.x, panelView.y, panelView.width, panelView.height);
                //GuiGrid((Rectangle){panelRec.x + panelScroll.x, panelRec.y + panelScroll.y, panelContentRec.width, panelContentRec.height}, NULL, 16, 3, NULL);
                for(int i = 0; i < constSize.size(); i++){
                    DrawRectangle(panelRec.x + panelScroll.x, panelRec.y + panelScroll.y + (30*i), panelContentRec.width, 30, {255,0,0,128});
                }
                EndScissorMode();

                if(result == 1 || result == 0) {
                    newNumOfReq = numOfReq;
                    newConstellation = false;
                    gui = false;
                }else if(result == 2){
                    newConstellation = false;
                    gui = false;
                    numOfReq = newNumOfReq;
                }

            }

            if(newCampaign){
                gui = true;
                DrawRectangle(0,0, WIDTH, HEIGHT, {255,255,255,200});
                int result = GuiMessageBox((Rectangle){ WIDTH/3,HEIGHT/3, WIDTH/3, HEIGHT/3},
                    "#191#Generate New Campain","", "BACK;CONFIRM");
                GuiLabel((Rectangle){ 355, 305, 50, 30 },"Number\nof\nrequests");
                GuiSlider((Rectangle){ 435, 300, 150, 30 }, "", "", &newNumOfReq, 1.0f, 1000.0f);
                GuiLabel((Rectangle){ 615, 305, 150, 30 },TextFormat("%d", static_cast<size_type>(newNumOfReq)));
                if(result == 1 || result == 0) {
                    newNumOfReq = numOfReq;
                    newCampaign = false;
                    gui = false;
                }else if(result == 2){
                    newCampaign = false;
                    gui = false;
                    numOfReq = newNumOfReq;
                    generateRequests(request, numOfReq, TimePoint(days, hours, minutes), TimePoint(days+2, hours, minutes), 60);
                }

            }
        EndDrawing();

        handleCommands(camera);
    }
    CloseWindow();
    return 0;
}