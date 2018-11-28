
#define LINEAR(x,y) (x)+(y)*get_global_size(0)

#define M_PI 3.14159265358979323846

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

/***********************************************************/
/******************** Kernel functions  ********************/
/***********************************************************/

__kernel void find_peaks(
        __global const int * counter,
        __global uchar * counter_image,
        int a_ammount,
        int p_ammount,
        __global uchar * img,
        int width,
        int height,
        int threshold
    ) {
    /* TODO: explain kernel functionality */

    const int window = a_ammount*0.1;

    const int i = get_global_id(0);
    const int j = get_global_id(1);

    int val = counter[LINEAR(i,j)];

    float pithagoras = width*width + height*height;
    float p_max = native_sqrt(pithagoras);
    float p_min = -p_max;
    float p_step = (p_max-p_min)/p_ammount;
    float a_max = M_PI;
    float a_min = 0;
    float a_step = (a_max-a_min)/a_ammount;
   

    if (val > threshold){

            for(int dy = j-window/2; dy < j+window/2; dy++){
                for(int dx = i-window/2; dx < i+window/2; dx++){
                    if(dy > 0 && dx > 0 
                        && dy < p_ammount 
                        && dx < a_ammount 
                        && counter[LINEAR(dx,dy)] > val){
                        return;
                    }
                }
            }
            /* mark it on counter */
            uchar3 rgb = {0,0,255};
            vstore3(rgb, LINEAR(i,j), counter_image);


            /* overlay line in image */
            int ai = i;
            int pi = j;
            float p = p_min+p_step*pi;
            float m2 = native_sin(a_min+a_step*ai)/(native_cos(a_min+a_step*ai)+0.00001);
            float m1 = -1.0 / (m2 + 0.00001);
            float b = p / (native_sin(a_min+a_step*ai) + 0.00001);
            for (int xx = 0; xx < width; ++xx){
                int yy = b+xx*m1;
                if (yy < height && yy >= 0){
                    uchar3 red = {0,0,255};
                    vstore3(red, yy*width+xx, img);
                }
            }
            for (int yy = 0; yy < height; ++yy){
                int xx = (yy-b)/m1;
                if (xx < width && xx >= 0){
                    uchar3 red = {0,0,255};
                    vstore3(red, yy*width+xx, img);
                }
            }
    }

}
__kernel void draw_counter(
        __global const int * counter,
        __global uchar * counter_image
    ) {
    /* TODO: explain kernel functionality */

    const int i = get_global_id(0);
    const int j = get_global_id(1);
    const int MAX = 500;
    int count = counter[LINEAR(i,j)]*255/MAX;
    count = count > 255 ? 255 : count;
    uchar3 rgb = {count, 0,0};

    vstore3(rgb, LINEAR(i,j), counter_image);

}
__kernel void edges_counter(
        __global const uchar * src,
        __global int * counter,
        int a_ammount,
        int p_ammount,
        __global int * max_count
    ) {

    /* TODO: explain kernel functionality */

    const int i = get_global_id(0);
    const int j = get_global_id(1);

    const int width = get_global_size(0);
    const int height = get_global_size(1);

    if(i < 5 || j < 5) return;
    if(i > width-5 || j >  height - 5) return;

    uchar3 rgb = vload3(LINEAR(i,j), src);
    
    float pithagoras = width*width + height*height;
    float p_max = native_sqrt(pithagoras);
    float p_min = -p_max;
    float p_step = (p_max-p_min)/p_ammount;
    float a_max = M_PI;
    float a_min = 0;
    float a_step = (a_max-a_min)/a_ammount;

    int max_count_local = 0;
    if(rgb.x > 0){
        for (int ai = 0; ai < a_ammount; ++ai){
            float p = i*native_cos(a_min+a_step*ai)+j*native_sin(a_min+a_step*ai);
            int p_index = round((p-p_min)/p_step);
            int index = ai+p_index*a_ammount;
            atomic_inc(counter+index);
            max_count_local = max(max_count_local, *(counter+index));
        }
    }

    atomic_max(max_count, max_count_local);

}

