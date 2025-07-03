#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>

#ifndef UTILS   
#define UTILS
#include "utils.h"
#endif


class Plot{
    private: 
        size_type counter;
        Vector2 endPos;

        float mapValue(int a, int b, int c, int d, int value){
            return (value-a)*(d-c)/(b-a);
            //return ((value - a) / (b - a) * (d - c)) + c;
        }

    public:
        Vector2 startPos;
        Vector2 size;
        const int xAxisBin = 10;
        const int yAxisBin = 10;
        float tickLen;
        
        Plot(Vector2 startPos, Vector2 size)
            :startPos(startPos),
            size(size),
            tickLen(size.x / 100),
            counter(counter),
            endPos(Vector2Add(startPos, size)){}

        void draw(std::vector<float> &xValues, std::vector<float> &yValues){
// std::cout << "endPos: " << endPos.x << "  " << endPos.y <<std::endl;
// std::cout << "TickLen: " << tickLen << std::endl;
// std::cout << "========================" <<std::endl;
// std::cout << "size  di X: " << xValues.size() <<std::endl;
// std::cout << "size  di Y: " << yValues.size() <<std::endl;
// std::cout << "Valori di X: " << std::endl;
//     for(int i = 0; i < xAxisBin; i++) std::cout << xValues[i] << " ";
//     std::cout << std::endl;
// std::cout << "Valori di Y: " << std::endl;
//     for(int i = 0; i < yAxisBin; i++) std::cout << yValues[i] << " ";
//     std::cout << std::endl;
                int minY = yValues[0];
                int maxY = yValues[0];
                for(int i = 0; i < xAxisBin; i++){
                    if(yValues[i] < minY) minY = yValues[i];
                    if(yValues[i] > maxY) maxY = yValues[i];
                }
                //We adjust the max value and "round it" to the next value divisible by yAxisBin
                maxY = ((maxY + minY) % yAxisBin == 0) ? 
                maxY : 
                maxY + yAxisBin - ((maxY + minY) % yAxisBin);
                int tickDiscretization = (maxY + minY) / yAxisBin; 

                //Y
                DrawLine(startPos.x, startPos.y, startPos.x, endPos.y, BLACK);
                //X
                DrawLine(startPos.x, endPos.y, endPos.x, endPos.y, BLACK);
                //Draw the y and x values along the axis 
                for(int i = 1; i <= xAxisBin; i++) {
                    DrawLine(startPos.x+(size.x/xAxisBin)*i, 
                        endPos.y-tickLen, 
                        startPos.x+(size.x/xAxisBin)*i, 
                        endPos.y+tickLen, BLACK);
                    DrawLine(startPos.x-tickLen, 
                        endPos.y-(size.y/yAxisBin)*i,
                        startPos.x+tickLen, 
                        endPos.y-(size.y/yAxisBin)*i, 
                        BLACK);
                    DrawText(TextFormat("%d", static_cast<int>(xValues[(i-1)])), startPos.x+(size.x/xAxisBin)*i, endPos.y+tickLen+5.0f, 15,BLACK);
                    DrawText(TextFormat("%d", tickDiscretization*i), startPos.x-tickLen-25.0f, endPos.y-(size.y/yAxisBin)*i, 15,BLACK);
                    DrawLine(startPos.x, endPos.y-(size.y/yAxisBin)*i, endPos.x, endPos.y-(size.y/yAxisBin)*i, { 200, 200, 200, 200 });
                }   
            
            float targetY = endPos.y;
            float newTargetY = 0;
            //Draw the actual data 
            for(int i = 0; i < xAxisBin; i++) {
                newTargetY = mapValue(minY, maxY, startPos.y, endPos.y, yValues[i]);
                DrawLine(startPos.x+(size.x/xAxisBin)*i, 
                        targetY,
                        startPos.x+(size.x/xAxisBin)*(i+1), 
                        endPos.y - newTargetY, 
                        RED);
                DrawCircle(startPos.x+(size.x/xAxisBin)*(i+1), endPos.y - newTargetY, 3.0f, RED); 
                float value = yValues[i];
                DrawText(TextFormat("%.2f",yValues[i]), startPos.x+(size.x/xAxisBin)*(i+1) - 22.0f, endPos.y - newTargetY - 10.0f, 5, RED);
                targetY = endPos.y - newTargetY;
            }
            if(counter == 60) counter = 0;
            counter++;
        }
};