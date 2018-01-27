#include <SDL2/SDL.h>

int ShowRGBBuffer(SDL_Renderer * renderer, char * buffer, int width, int height);

int main(int argc, char ** argv){
	bool quit = false;
	SDL_Event event;

	SDL_Init(SDL_INIT_VIDEO);

	/* create window */
	SDL_Window * window = SDL_CreateWindow("Awesome window",
	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
	/* create renderer */
	SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, 0);

	/* some raw data to display */
	int height = 100;
	int width = 100;
	char* pixels = (char*)malloc(height*width*3);
	for (size_t i = 0; i < height*width*3; i++) {
		pixels[i] = 0;
		i++;
		pixels[i] = 0;
		i++;
		pixels[i] = 255;
	}

	/* show data until windows is closed */
	while (!quit){
		SDL_WaitEvent(&event);
		switch (event.type){
			case SDL_QUIT:
			quit = true;
			break;
		}
		ShowRGBBuffer(renderer, pixels, width, height);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}




int ShowRGBBuffer(SDL_Renderer * renderer, char * buffer, int width, int height){
	/*
	Displays data in 'buffer' as an image in 'renderer'.
	Data in 'buffer' should be pixels with RGB format with 1 byte per color,
	meaning a 24 bits depth (no alpha channel).
	Dimensions 'height' and 'width' reefer to pixels rather than bytes.
	TODO: error handling
	*/

	/* color masks */
	unsigned int rmask, gmask, bmask, amask;
	rmask = 0xff0000;
	gmask = 0x00ff00;
	bmask = 0x0000ff;
	amask = 0x000000;

	/* create surface from buffer */
	SDL_Surface* image = SDL_CreateRGBSurfaceFrom((void*)buffer, width, height, 24, 3*width,
	rmask, gmask, bmask, amask);
	// SDL_Surface * image = SDL_LoadBMP("panda.bmp");

	/* create texture and render */
	SDL_Texture * texture = SDL_CreateTextureFromSurface(renderer, image);
	SDL_Rect dstrect = { 0, 0	, height, width };
	SDL_RenderCopy(renderer, texture, NULL, &dstrect);
	SDL_RenderPresent(renderer);

	/* free memory */
	SDL_DestroyTexture(texture);
	SDL_FreeSurface(image);

	return 0;
}
