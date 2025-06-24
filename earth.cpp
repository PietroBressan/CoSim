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

#define RAYGUI_SUPPORT_ICONS
#include <../include/raygui.h>
#include "Satellite.h"

//PARAMETERS
//Earth parameters
float earthRotation = 0.0f;
// useless float earthInclination = 0.4014257f;
//Problem instance parameters
int numOfSatellite = 1;
int numOfPlanes = 3;
int numOfTargets = 2;
Vector3* targets = nullptr;
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
bool stop = false;
float timeSpeed = 1.0f;
int step = 0;
int minutes = 0;
int hours = 0;
int days = 0;
bool showMore = false;
float panelWidth = 200.0f;
int counter = 0;
bool obs = false;
bool infoAnimation = false;
size_type infoSlide = 0;
std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

/**
https://en.wikipedia.org/wiki/Spherical_coordinate_system

TODO List:

- finire l'algoritmo
- implementare le ground station per lo scarico dei dati + comunicazione

- migliorare il metodo isVisible
- aggiungere fmod per il round degli angoli
- Aggiungere un raggio ai target, attualmente sono dei punti

*/



/**
Generate a campaign composed of many requests each with a time interval to satisfy
*/
void generateTargets(Vector3*& targets, int numberOfTargets, float EARTH_RADIUS){
    if (targets != nullptr) delete[] targets;
    targets = new Vector3[numberOfTargets];
    float randomTheta = 0.0f;
    float randomPhi = 0.0f;
    for(int i = 0; i < numberOfTargets; i++){
        randomTheta = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX))*2*PI;
        randomPhi = -PI/2 + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX))*PI;
        Vector3 randPos = getCartesian(randomTheta, randomPhi, EARTH_RADIUS);    
        targets[i].x = randPos.x;
        targets[i].y = randPos.y;
        targets[i].z = randPos.z;   
    }
}

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
        if (IsKeyPressed(KEY_X) ) timeSpeed /= 2;
        //Handle time
        if(!stop){
            //To rotate the earth of 360 degrees it takes 1800 frames with a fixed rotation rate of 0.2f.
            //We have 60 frames per seconds at speed 1
            //for a total of 30 seconds to complete a full rotation (one solar day = 30 secs with timeSpeed = 1)
            step += 60 / timeSpeed;
            if (step >= 86400){

            
             std::cout << "=================================" << std::endl;
                std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::seconds> (end - begin).count() << "[s]" << std::endl;
                begin = std::chrono::steady_clock::now();
                days++;
                step = step - 86400;
            }
            hours = step / 3600;
            minutes = step % (3600) / 60;
        }
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
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
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
        if (IsKeyPressed(KEY_M)) generateTargets(targets, numOfTargets, EARTH_RADIUS);
        

}

void handleInfos(){
    if(!infoAnimation && showMore && infoSlide == 0) infoAnimation = true;
    if(infoAnimation && !showMore && infoSlide == 0) infoAnimation = false;
    if(infoSlide < 1000 && showMore && infoAnimation) infoSlide += 50;
    if(infoSlide >= 0 && !showMore && infoAnimation) infoSlide -= 50;
    DrawRectangle(0,0,infoSlide, 700, {255,255,255,200});
    if (infoSlide == 1000 && GuiButton((Rectangle){ 24, 24, 120, 30 }, "#141#Show Less")) showMore = !showMore;
}


int main(){
    srand(time(0));
    generateTargets(targets, numOfTargets, EARTH_RADIUS);
    targets[0] = getCartesian(PI/2, 0, EARTH_RADIUS);
    Satellite satellite[numOfSatellite];
    targets[0] = getCartesian(PI/4,0,EARTH_RADIUS);
    //[1] = getCartesian(0,0,EARTH_RADIUS);
    satellite[0].color = LIME;
    satellite[0].initialize(0, 0, 16, false);
    satellite[0].index = 2;
    Request r(Target(targets[0]), 10.0f, 5.0f, TimePoint(), TimePoint());
    Vector3 groundStation = getCartesian(0,0,EARTH_RADIUS-2.0f);
    InitWindow(1000, 700, "Earth Model");
    Camera camera = {getCartesian(cameraTheta, cameraPhi, cameraRadius), { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, CAMERA_PERSPECTIVE};
    SetTargetFPS(60); 

bool check = true;
    //Initialization
    std::cout << "MMMM "<< TextFormat("%d : %d : %d", days, hours, minutes) << std::endl;
                
TimePoint now;
TimePoint later = now.computeDateAfterElapsedTime(1695);
std::cout << "STIMA "<< TextFormat("%d : %d : %d", later.day, later.hour, later.minute) << std::endl;

    satellite[0].computeVisibilites(r);
    //
    while(!WindowShouldClose()){
        handleCommands(camera);
        //Resume/Pause simulation
        if(!stop){
            //For each satellite orbiting we call the updatePosition method
            for(int i = 0; i < numOfSatellite; i++){
                satellite[i].updatePosition(timeSpeed);
            }
            earthRotation += EARTH_ROTATION_RATE*RAD2DEG/timeSpeed;
            
             for(int i = 0; i < numOfTargets; i++){
                targets[i] = Vector3Transform(targets[i], MatrixRotateY(EARTH_ROTATION_RATE/timeSpeed));
            }
            groundStation = Vector3Transform(groundStation, MatrixRotateY(EARTH_ROTATION_RATE/timeSpeed));
            //Return to 0 to avoid overflow (just in case)
            if(earthRotation >= 360.0f) earthRotation = earthRotation - 360.0f;
      
        }


        BeginDrawing();
            ClearBackground(BLACK);
            BeginMode3D(camera);
            //Earth
            rlPushMatrix();
            rlRotatef(earthRotation, 0.0, 1.0, 0);    
            DrawSphereWires({0.0f,0.0f,0.0f}, EARTH_RADIUS, 50, 50, BLUE);
            //DrawSphereEx({0.0f,0.0f,0.0f}, EARTH_RADIUS, 25,25, { 0, 121, 241, 220 });
            DrawLine3D({0.0f,0.0f,0.0f}, {10.0f,0.0f,0.0f}, YELLOW);
            DrawLine3D({0.0f,0.0f,0.0f}, {0.0f,10.0f,0.0f}, GREEN);
            DrawLine3D({0.0f,0.0f,0.0f}, {0.0f,0.0f,10.0f}, WHITE);  
            

            DrawCircle3D({0.0f,0.0f,EARTH_RADIUS-0.3f}, 2.5f, {1.0f,0.0f,0.0f}, 180, RED);
            for(float i = 1; i < 10; i++) DrawCircle3D({0.0f,0.0f,EARTH_RADIUS-0.3f+(0.05f*i)}, 2.5f, {1.0f,0.0f,0.0f}, 180, RED);
            for(float i = 1; i < 20; i++) DrawCircle3D({0.0f,0.0f,EARTH_RADIUS-0.3f+(0.05f*9)}, 2.5f-(0.2f*i), {1.0f,0.0f,0.0f}, 180, RED);
            rlPopMatrix();
            for(int i = 0; i < numOfTargets; i++){
                DrawCapsule(targets[i], targets[i], 0.1f, 1, 1, GREEN );
                DrawCircle3D({0.0f,0.0f,EARTH_RADIUS-0.3f}, 2.5f, targets[i], Vector3Angle({0.0f,0.0f,EARTH_RADIUS}, targets[i])*RAD2DEG, GREEN);
                //for(float i = 1; i < 10; i++) DrawCircle3D({0.0f,0.0f,EARTH_RADIUS-0.3f+(0.05f*i)}, 1.0f,{1.0f,0.0f,0.0f}, Vector3Angle({0.0f,0.0f,EARTH_RADIUS}, targets[i]), GREEN);
                //for(float i = 1; i < 20; i++) DrawCircle3D({0.0f,0.0f,EARTH_RADIUS-0.3f+(0.05f*9)}, 1.0f-(0.2f*i), {1.0f,0.0f,0.0f}, Vector3Angle({0.0f,0.0f,EARTH_RADIUS}, targets[i]), GREEN);
            }
            //DrawCapsuleWires(groundStation,groundStation, 3.0f, 10, 10, { 211, 176, 131, 255 } );
for(int i = 0; i < numOfSatellite; i++){
            DrawCube(satellite[i].position, 0.1f, 0.1f, 0.1f, satellite[i].color);
            Vector3 coneEnd = Vector3Scale(Vector3Normalize(satellite[i].position), EARTH_RADIUS);
            float radiusEnd = Vector3Distance(satellite[i].position, coneEnd)*tanf(60*DEG2RAD/2);
            DrawCylinderEx(satellite[i].position,coneEnd,0.01f, radiusEnd, 128, Fade(YELLOW, 0.5));
if(Vector3Distance(targets[i], coneEnd) < 0.5f){
                if(!satellite[i].runningObservation){
                    satellite[i].runningObservation = true;
                    satellite[i].counter++;
                    //std::cout << "L'ho visto"<< std::endl;
                }
            }else if(satellite[i].runningObservation) satellite[i].runningObservation = false; 

}
if(!stop){
Vector3 dist = Vector3Normalize(Vector3Subtract(satellite[0].position, targets[0]));
if(satellite[0].isVisible(satellite[0].position, targets[0]) && check){
    check = false;    
    //std::cout << "Found "<< TextFormat("%d : %d : %d", days, hours, minutes) << std::endl;
    }else check = true;
}

//std::cout << "L'ho visto"<< std::endl;

//std::cout << "Days : " << position << std::endl;

            EndMode3D();
            if (!infoAnimation && GuiButton((Rectangle){ 24, 24, 120, 30 }, "#141#Show More")) showMore = !showMore;
            if(infoAnimation || showMore){
                handleInfos();
            }
            DrawRectangle(800,623,200, 50, {255,255,255,128});
            DrawText("days", 820, 625, 15, DARKGREEN);
            DrawText("hours", 865, 625, 15, DARKGREEN);
            DrawText("minutes", 916, 625, 15, DARKGREEN);
            DrawText(TextFormat("%d : %d : %d", days, hours, minutes), 830, 640, 30, GREEN);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}