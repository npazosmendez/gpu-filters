#include <iostream>
extern "C" {
  #include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"
#include <chrono>
#include <TextTable.h>
#include <ProgressBar.hpp>
#include <iomanip>
#include <sstream>
#include <set>
#include <map>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define REPEAT(N) for(int _i=0; _i<(N); _i++)

using namespace cv;
using namespace std;


int width;
int height;

Mat source_image;

Mat image1;
Mat image2;

int a_ammount;
int p_ammount;
char * counter;

vec * flow;

bool * mask_ptr;
int * debug; // TODO: sacar esto. no es tan directo, porque se usa si #ifdef DEBUG

void run_canny_CL(){
    CL_canny((char*)image1.ptr(), width, height, 60, 30);
}
void run_canny_C(){
    canny((char*)image1.ptr(), width, height, 60, 30);
}

void run_hough_C(){
    hough((char*)image1.ptr(), width, height, a_ammount, p_ammount, counter);
}
void run_hough_CL(){
    CL_hough((char*)image1.ptr(), width, height, a_ammount, p_ammount, counter);
}

void run_kanade_C(){
    kanade(width, height, (char*)image1.ptr(), (char*)image2.ptr(), flow, 2);
}
void run_kanade_CL(){
    CL_kanade(width, height, (char*)image1.ptr(), (char*)image2.ptr(), flow, 2);
}

void run_inpainting_C(){
    inpainting((char*)image1.ptr(), width, height, mask_ptr, debug);
}
void run_inpainting_CL(){
    CL_inpainting((char*)image1.ptr(), width, height, mask_ptr, debug);
}

void initialize_mask(){
    memset(mask_ptr, 0, height * width * sizeof(bool));
    int x = width/2;
    int y = height/2;
    int side = width/30;
    for (int i = -side; i <= side; i++ ) {
        for (int j = -side; j <= side; j++) {
            int yi = y + i;
            int xj = x + j;
            // int yi = clamp(y + i, 0, height);
            // int xj = clamp(x + j, 0, width);
            mask_ptr[yi*width+xj] = true;
        }
    }
}



string double_to_string(double val){
    stringstream stream;
    stream << fixed << setprecision(2) << val;
    return stream.str();
}

void print_filter_measurements(string filter_name, vector<double> &CL_miliseconds_durations, vector<double> &C_miliseconds_durations, TextTable& text_table){

    // TODO: further analize measurements: variance, mean, outliers, etc.
    double C_miliseconds = *min_element(C_miliseconds_durations.begin(), C_miliseconds_durations.end());
    double CL_miliseconds = *min_element(CL_miliseconds_durations.begin(), CL_miliseconds_durations.end());

    text_table.add( filter_name );
    text_table.add("");
    text_table.add( double_to_string(C_miliseconds) + "ms" );
    text_table.add( double_to_string(1000.0/C_miliseconds) + " FPS" );
    text_table.add( double_to_string(CL_miliseconds) + "ms" );
    text_table.add( double_to_string(1000.0/CL_miliseconds) + " FPS" );
    text_table.add( double_to_string(C_miliseconds / CL_miliseconds) + "x" );
    text_table.endOfRow();
}


void time_filter(string filter_name, int warmup_iterations, int time_iterations, vector<double> &C_miliseconds_durations, vector<double> &CL_miliseconds_durations){
    C_miliseconds_durations = vector<double>();
    CL_miliseconds_durations = vector<double>();
    cout << "Timing " << filter_name << "..." << endl;
    auto begin = chrono::high_resolution_clock::now();
    auto end = chrono::high_resolution_clock::now();
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
    }else if(filter_name == "inpainting"){
        C_function = run_inpainting_C;
        CL_function = run_inpainting_CL;
    }

    ProgressBar bar(warmup_iterations + time_iterations);
    bar.print();
    REPEAT(warmup_iterations){
        image1 = source_image.clone();
        image2 = source_image.clone();
        C_function();
        bar.update();
    }
    for (int i = 0; i < time_iterations; ++i){
        image1 = source_image.clone();
        image2 = source_image.clone();
        initialize_mask();
        begin = chrono::high_resolution_clock::now();
        C_function();
        end = chrono::high_resolution_clock::now();
        long int C_microsec_duration = chrono::duration_cast<chrono::microseconds>(end-begin).count();
        bar.update();
        C_miliseconds_durations.push_back(C_microsec_duration / 1000.0);
    }
    bar.finish();

    bar.print();
    REPEAT(warmup_iterations){
        image1 = source_image.clone();
        image2 = source_image.clone();
        CL_function();
        bar.update();
    }
    for (int i = 0; i < time_iterations; ++i){
        image1 = source_image.clone();
        image2 = source_image.clone();
        initialize_mask();
        begin = chrono::high_resolution_clock::now();
        CL_function();
        end = chrono::high_resolution_clock::now();
        long int CL_microsec_duration = chrono::duration_cast<chrono::microseconds>(end-begin).count();
        bar.update();
        CL_miliseconds_durations.push_back(CL_microsec_duration / 1000.0);
    }
    bar.finish();

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
        cout << "\t--warmup <#iterations>" << endl;
        cout << "\t--device <device_nr>" << endl;
        cout << "\t--image <image_path> (needed)" << endl;
        cout << "\t--help" << endl;
        return 0;
    }

    set<string> all_filters = {"canny", "hough", "kanade", "inpainting"};
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

    // defaults are low because of inpainting
    int warmup_iterations = 3;
    int time_iterations = 5;
    if (arguments.count("--iterations")){
        time_iterations = stoi(arguments["--iterations"]);
        if (time_iterations <= 0){
            cerr << "Number of iterations should be positive" << endl;
            abort();
        }
    }
    if (arguments.count("--warmup")){
        warmup_iterations = stoi(arguments["--warmup"]);
        if (time_iterations <= 0){
            cerr << "Number of iterations should be positive" << endl;
            abort();
        }
    }

    int selected_device = 0;
    if (arguments.count("--device")) {
        selected_device = stoi(arguments["--device"]);
        if (selected_device < 0) {
            cerr << "Device number should be non negative" << endl;
            abort();
        }
    }
    selectDevice(selected_device);

    if (not arguments.count("--image")){
        cerr << "Need an image path through '--image'"<<endl;
        abort();
    }

    source_image = imread(arguments["--image"], CV_LOAD_IMAGE_COLOR);
    width = source_image.size().width;
    height = source_image.size().height;

    a_ammount = 100;
    p_ammount = 100;
    counter = (char*)malloc(a_ammount*p_ammount*3*sizeof(char));
    flow = (vec*) malloc(sizeof(vec) * width * height);
    mask_ptr = (bool*) malloc(width * height * sizeof(bool));
    initialize_mask();

    debug = (int*) malloc(width * height * sizeof(int));
    memset(debug, 0, width * height * sizeof(int));

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
    
    initCL(true);
    cout << "\n";

    vector<double> C_miliseconds_durations;
    vector<double> CL_miliseconds_durations;

    for(string filter_name : selected_filters){
        time_filter(filter_name, warmup_iterations, time_iterations, C_miliseconds_durations, CL_miliseconds_durations);
        print_filter_measurements(filter_name, CL_miliseconds_durations, C_miliseconds_durations, text_table);
    }

    cout << text_table;
    return 0;
}