#include <iostream>
extern "C" {
  #include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"
#include <chrono>
#include <TextTable.h>
#include <iomanip>
#include <sstream>
#include <set>
#include <map>

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
    // TODO: every call should be done with the same image, not the filtered one
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


map<string, string> parse_args(int argc, const char** argv){
    map<string, string> result;
    for (int i = 0; i < argc; ++i){
        string key = argv[i];
        if (key.size() > 1 and key[0] == '-' and key[1] == '-'){
            if (i+1 < argc){
                result[key] = argv[i+1];
            }else{
                result[key] = "";
            }
        }
    }
    return result;
}

int main(int argc, const char** argv) {

    map<string, string> arguments = parse_args(argc, argv);

    if (arguments.count("--help")){
        cout << "Timing application for the filters. For custom timing try:" << endl;
        cout << "\t--filter <filter_name> (canny/hough/kanade)" << endl;
        cout << "\t--iterations <#iterations>" << endl;
        cout << "\t--help" << endl;
        return 0;
    }

    set<string> all_filters = {"canny", "hough", "kanade"};
    set<string> selected_filters;
    if (arguments.count("--filter")){
        string filter = arguments["--filter"];
        if(not all_filters.count(filter)){
            cerr << "Unkown filter '" << filter << "'" << endl;
            abort();
        }
        selected_filters.insert(filter);
    }else{
        selected_filters = all_filters;
    }

    int warmup_iterations = 10;
    int time_iterations = 10;
    if (arguments.count("--iterations")){
        time_iterations = stoi(arguments["--iterations"]);
        if (time_iterations <= 0){
            cerr << "Number of iterations should be positive" << endl;
            abort();
        }
    }

    width = 640;
    height = 360;
    image = (char*)malloc(width*height*3*sizeof(char));
    image2 = (char*)malloc(width*height*3*sizeof(char));
    flow = (vec*) malloc(sizeof(vec) * width * height);
    
    a_ammount = 100;
    p_ammount = 100;
    counter = (char*)malloc(a_ammount*p_ammount*3*sizeof(char));


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
    
    initCL(false);

    double C_miliseconds_duration;
    double CL_miliseconds_duration;

    for(string filter_name : selected_filters){
        time_filter(filter_name, warmup_iterations, time_iterations, C_miliseconds_duration, CL_miliseconds_duration);
        print_filter_measurements(filter_name, CL_miliseconds_duration, C_miliseconds_duration, text_table);
    }

    cout << text_table;
    return 0;
}