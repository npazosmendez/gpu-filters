#include <iostream>
extern "C" {
  #include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"
#include <chrono>
#include <TextTable.h>
#include <iomanip> // setprecision
#include <sstream> // stringstream

#define REPEAT(N) for(int _i=0; _i<(N); _i++)

using namespace std;


int width;
int height;
char * image;
char * image2;

int a_ammount;
int p_ammount;
char * counter;

vec * flow;

void run_canny_CL(){
    CL_canny(image, width, height, 60, 30);
}
void run_canny_C(){
    canny(image, width, height, 60, 30);
}

void run_hough_C(){
    hough(image, width, height, a_ammount, p_ammount, counter);
}
void run_hough_CL(){
    CL_hough(image, width, height, a_ammount, p_ammount, counter);
}

void run_kanade_C(){
    kanade(width, height, image, image2, flow, 2);
}
void run_kanade_CL(){
    CL_kanade(width, height, image, image2, flow, 2);
}

string double_to_string(double val){
    stringstream stream;
    stream << fixed << setprecision(2) << val;
    return stream.str();
}

void print_filter_measurements(string filter_name, double CL_miliseconds, double C_miliseconds, TextTable& text_table){

    text_table.add( filter_name );
    text_table.add("");
    text_table.add( double_to_string(C_miliseconds) + "ms" );
    text_table.add( double_to_string(1000.0/C_miliseconds) + " FPS" );
    text_table.add( double_to_string(CL_miliseconds) + "ms" );
    text_table.add( double_to_string(1000.0/CL_miliseconds) + " FPS" );
    text_table.add( double_to_string(C_miliseconds / CL_miliseconds) + "x" );
    text_table.endOfRow();
}


void time_filter(string filter_name, int warmup_iterations, int time_iterations, double &C_miliseconds_duration, double &CL_miliseconds_duration){
    cout << "Timing " << filter_name << "..." << endl;
    auto begin = chrono::high_resolution_clock::now();
    auto end = chrono::high_resolution_clock::now();
    long int C_microsec_duration;
    long int CL_microsec_duration;
    void (*C_function)();
    void (*CL_function)();
    if (filter_name == "canny"){
        C_function = run_canny_C;
        CL_function = run_canny_CL;
    }else if(filter_name == "hough"){
        C_function = run_hough_C;
        CL_function = run_hough_CL;
    }else if(filter_name == "kanade"){
        C_function = run_kanade_C;
        CL_function = run_kanade_CL;
    }
    REPEAT(warmup_iterations) C_function();
    begin = chrono::high_resolution_clock::now();
    REPEAT(time_iterations) C_function();
    end = chrono::high_resolution_clock::now();
    C_microsec_duration = chrono::duration_cast<chrono::microseconds>(end-begin).count();

    REPEAT(warmup_iterations) CL_function();
    begin = chrono::high_resolution_clock::now();
    REPEAT(time_iterations) CL_function();
    end = chrono::high_resolution_clock::now();
    CL_microsec_duration = chrono::duration_cast<chrono::microseconds>(end-begin).count();

    C_miliseconds_duration = C_microsec_duration / time_iterations / 1000.0;
    CL_miliseconds_duration = CL_microsec_duration / time_iterations / 1000.0;
}

int main(int argc, char** argv) {
    initCL(false);

    TextTable text_table( '-', '|', '+' );

    text_table.add( "Filter" );
    text_table.add("");
    text_table.add( "Frame time (C)" );
    text_table.add( "FPS (C)" );
    text_table.add( "Frame time (OpenCL)" );
    text_table.add( "FPS (OpenCL)" );
    text_table.add( "x" );
    text_table.endOfRow();

    text_table.setAlignment( 2, TextTable::Alignment::RIGHT );
    text_table.setAlignment( 4, TextTable::Alignment::RIGHT );

    width = 640;
    height = 360;
    image = (char*)malloc(width*height*3*sizeof(char));
    image2 = (char*)malloc(width*height*3*sizeof(char));
    flow = (vec*) malloc(sizeof(vec) * width * height);
    
    a_ammount = 100;
    p_ammount = 100;
    counter = (char*)malloc(a_ammount*p_ammount*3*sizeof(char));

    

    int warmup_iterations = 10;
    int time_iterations = 10;
    double C_miliseconds_duration;
    double CL_miliseconds_duration;

    time_filter("canny", warmup_iterations, time_iterations, C_miliseconds_duration, CL_miliseconds_duration);
    print_filter_measurements("Canny", CL_miliseconds_duration, C_miliseconds_duration, text_table);

    time_filter("hough", warmup_iterations, time_iterations, C_miliseconds_duration, CL_miliseconds_duration);
    print_filter_measurements("Hough", CL_miliseconds_duration, C_miliseconds_duration, text_table);

    time_filter("kanade", warmup_iterations, time_iterations, C_miliseconds_duration, CL_miliseconds_duration);
    print_filter_measurements("Kanade", CL_miliseconds_duration, C_miliseconds_duration, text_table);
    cout << text_table;
    return 0;
}