#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <iostream>
#include <cmath>
#include <stdlib.h>
#include <ctime>
#include <chrono>
#include <queue>
#include <thread>
#define RAYGUI_IMPLEMENTATION

#define RAYGUI_SUPPORT_ICONS
#include <../include/raygui.h>
#include "Satellite.h"


/**
https://en.wikipedia.org/wiki/Spherical_coordinate_system

Guardare raygui per la gui

Planning delle targetsi
*/



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

void generateTargets(Vector3*& targets, int numberOfTargets, float earthRadius){
    if (targets != nullptr) delete[] targets;
    targets = new Vector3[numberOfTargets];
    float randomTheta = 0.0f;
    float randomPhi = 0.0f;
    for(int i = 0; i < numberOfTargets; i++){
        randomTheta = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX))*2*PI;
        randomPhi = -PI/2 + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX))*PI;
        Vector3 randPos = getCartesian(randomTheta, randomPhi, earthRadius);    
        targets[i].x = randPos.x;
        targets[i].y = randPos.y;
        targets[i].z = randPos.z;   
    }
}

int main(){
    InitWindow(1000, 700, "Earth Model");
    //Earth parameters
    float earthRotation = 0.0f;
    float earthRotationRate = 0.2f*DEG2RAD;
    float earthInclination = 0.4014257f;
    float earthRadius = 3.5f;

    srand(time(0));
    int numOfSatellite = 3;
    int numOfPlanes = 3;
    int numOfTargets = 10;
    Vector3* targets = nullptr;
    
    generateTargets(targets, numOfTargets, earthRadius);
    Satellite satellite[numOfSatellite];
    targets[0] = getCartesian(PI/4,0,3.5f);
    satellite[0].color = RED;
    satellite[0].initialize(PI/4, 0, 16, false);
    satellite[0].calculateVisibilites(targets[0]);
    satellite[0].id = 1;
    satellite[1].color = LIME;
    satellite[1].initialize(0, 0, 16, false);
    satellite[1].calculateVisibilites(targets[0]);
    satellite[1].id = 2;
    satellite[2].color = LIME;
    satellite[2].initialize(0, PI, 16,  false);
    satellite[2].calculateVisibilites(targets[0]);
    satellite[2].id = 3;
    //std::cout << "X : " << d.x << " | " << "Y : " << d.y << " | " << "Z : " << d.z << std::endl;
            
    //Zoom
    int zoom = 0;
    //Orbital camera position and movement 
    float cameraTheta = 0;
    float cameraPhi = 0;
    float cameraRadius = 20.0f;
    Camera camera = {getCartesian(cameraTheta, cameraPhi, cameraRadius), { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, CAMERA_PERSPECTIVE};
    bool track = false;
    Vector2 startMouseTrack = {0};
    Vector2 oldMousePos = {0};
    float mouseSensitivity = 0.005f;
    float mouseSpeed = 0.0f;
    bool showPlanes = false;
    //Time parameters 
    //30 seconds = 24 hours at timeSpeed = 1.0f;
    bool stop = false;
    float timeSpeed = 1.0f; // decrease to speed up or increase to slow down
    float oldTimeSpeed = 0.0f;
    int step = 0;
    int seconds = 0;
    int minutes = 0;
    int hours = 0;
    int days = 0;
    bool print = false;
    bool showMore = false;
    float panelWidth = 200.0f;
    int counter = 0;
    bool obs = false;
    SetTargetFPS(60); 
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();


    //Initialization

    //
    while(!WindowShouldClose()){
        //Pause the simulation
        if (IsKeyPressed(KEY_SPACE)){
            stop = !stop;
        }
        //Speed up or slow down the scene
        if (IsKeyPressed(KEY_Z) && timeSpeed < 1.0f) timeSpeed *= 2;
        if (IsKeyPressed(KEY_X) && timeSpeed > 0.5f) timeSpeed /= 2;
        //Handle time
        if(!stop){
            //To rotate the earth of 360 degrees it takes 1800 frames with a fixed rotation rate of 0.2f.
            //We have 60 frames per seconds at speed 1
            //for a total of 30 seconds to complete a full rotation (one solar day = 30 secs with timeSpeed = 1)
            step += 48 / timeSpeed;
            if (step >= 86400){
             print = true;
             for(int i = 0; i < numOfSatellite; i++){
                std::cout << satellite[i].id << " L'ho visto : "<< satellite[i].counter << "volte" << std::endl;
                    satellite[i].counter = 0;
            }
             std::cout << "=================================" << std::endl;
             for(int i = 0; i < numOfSatellite; i++){
                satellite[i].calculateVisibilites(targets[0]);
            }
                std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                //std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::seconds> (end - begin).count() << "[s]" << std::endl;
                begin = std::chrono::steady_clock::now();
                days++;
                step = step - 86400;
            }
            hours = step / 3600;
            minutes = step % (3600) / 60;

            //For each satellite orbiting we call the updatePosition method
            for(int i = 0; i < numOfSatellite; i++){
                satellite[i].updatePosition(timeSpeed);
            }
            earthRotation += 0.2f/timeSpeed;
            
             for(int i = 0; i < numOfTargets; i++){
       
            targets[i] = Vector3Transform(targets[i], MatrixRotateY(earthRotationRate/timeSpeed));
            }
            //Return to 0 to avoid overflow (just in case)
            if(earthRotation >= 360.0f) earthRotation = earthRotation - 360.0f;
        }
        //Zooming
        zoom = GetMouseWheelMove();
        if(zoom){
            float newDepth = (cameraRadius-zoom*0.5f);
            newDepth *= newDepth;
            if(newDepth > 16.0f && newDepth < 900.0f ) {
                cameraRadius -= zoom*0.5f;
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
            cameraRadius = 20.000f;
            camera.position = getCartesian(cameraTheta, cameraPhi, cameraRadius);
            UpdateCamera(&camera, CAMERA_PERSPECTIVE);
        }
        //Show / Hide orbital trajectories (circles)
        if (IsKeyPressed(KEY_P)) showPlanes = !showPlanes;
        //Generate new targets
        if (IsKeyPressed(KEY_M)) generateTargets(targets, numOfTargets, earthRadius);
        


        BeginDrawing();
            ClearBackground(BLACK);
            BeginMode3D(camera);
            //Earth
            rlPushMatrix();
            rlRotatef(earthRotation, 0.0, 1.0, 0);    
            DrawSphereWires({0.0f,0.0f,0.0f}, earthRadius, 50, 50, BLUE);
            DrawLine3D({0.0f,0.0f,0.0f}, {10.0f,0.0f,0.0f}, YELLOW);
            DrawLine3D({0.0f,0.0f,0.0f}, {0.0f,10.0f,0.0f}, GREEN);
            DrawLine3D({0.0f,0.0f,0.0f}, {0.0f,0.0f,10.0f}, WHITE);  
            rlPopMatrix();
            for(int i = 0; i < numOfTargets; i++){
       DrawCapsule(targets[i], targets[i], 0.1f, 1, 1, GREEN );
    }
            
            Vector3 satellitePos = {0};
            //for(int i = 0; i < numOfSatellite-1; i++){
            //    satellitePos = getCartesian(satellite[i].theta, satellite[i].phi, satellite[i].radius);
            //    if(satellite[i].inclination != 0) satellitePos = Vector3RotateByAxisAngle(satellitePos, {0.0f, 0.0f, 1.0f}, satellite[i].inclination);
            //    DrawCube(satellitePos, 0.1f, 0.1f, 0.1f, satellite[i].color);
            //    Vector3 coneEnd = Vector3Scale(Vector3Normalize(satellitePos), earthRadius);
            //    DrawCylinderEx(satellitePos,coneEnd,0.01f, 0.5f, 128, Fade(YELLOW, 0.5));
            //    // Da sistemare
            //    if(showPlanes) DrawCircle3D({0.0f}, 
            //        Vector3Length(Vector3Subtract(satellitePos, {0.0f})), 
            //        {0.0f,1.0f,1.0f}, 
            //        45-(satellite[i].inclination*180/PI), 
            //        satellite[i].color);
            //}
for(int i = 0; i < numOfSatellite; i++){
            DrawCube(satellite[i].position, 0.1f, 0.1f, 0.1f, satellite[i].color);
            Vector3 coneEnd = Vector3Scale(Vector3Normalize(satellite[i].position), earthRadius);
            DrawCylinderEx(satellite[i].position,coneEnd,0.01f, 0.5f, 128, Fade(YELLOW, 0.5));
if(Vector3Distance(targets[i], coneEnd) < 0.5f){
                if(!satellite[i].runningObservation){
                    satellite[i].runningObservation = true;
                    satellite[i].counter++;
                    //std::cout << "L'ho visto"<< std::endl;
                }
            }else if(satellite[i].runningObservation) satellite[i].runningObservation = false; 

}
//std::cout << "Days : " << position << std::endl;

            EndMode3D();
            if (!showMore && GuiButton((Rectangle){ 24, 24, 120, 30 }, "#141#Show More")) showMore = !showMore;
            if(showMore){
                DrawRectangle(0,0,panelWidth, 800, {255,255,255,128});
                if (GuiButton((Rectangle){ 24+panelWidth, 24, 120, 30 }, "#141#Show Less")) showMore = !showMore;
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