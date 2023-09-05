#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cstdlib>

int main() {
    const int kWindowX = 800; 
    const int kWindowY = 600; 

    SDL_Init(SDL_INIT_EVERYTHING);
    
    SDL_Window *sdl_window = SDL_CreateWindow("Newboy", 
                                          SDL_WINDOWPOS_UNDEFINED, 
                                          SDL_WINDOWPOS_UNDEFINED,
                                          kWindowX,
                                          kWindowY,
                                          SDL_WINDOW_ALLOW_HIGHDPI);
    if (!sdl_window) {
        std::cout << "Could not create window " << SDL_GetError() << std::endl;
        abort();
    }

    // Constrain mouse to screen
    SDL_SetRelativeMouseMode(SDL_TRUE);

    SDL_Surface *sdl_surface = SDL_GetWindowSurface(sdl_window);
    if (!sdl_surface) {
        std::cout << "No surface " << SDL_GetError() << std::endl;
        abort();
    }

    // star texture
    SDL_Surface *sdl_img = IMG_Load("resources/star.png");
    if (!sdl_img) {
        std::cout << "Img load fail " << SDL_GetError() << std::endl;
        abort(); 
    }
    SDL_Rect sdl_src = {0, 0, 8, 8};

    // camera pos
    SDL_Rect camera = { 0, 0, 0, 0 };

    // star positions
    SDL_Rect sdl_stars[128];
    for (size_t s = 0; s < 128; ++s) {
        sdl_stars[s].x = ((rand() * kWindowX) & INT_MAX) % kWindowX;
        sdl_stars[s].y = ((rand() * kWindowY) & INT_MAX) % kWindowY;
    }
    
    SDL_Event sdl_event;
    bool exit = false;
    for (;!exit;) {
        // Event loop
        for (;SDL_PollEvent(&sdl_event) > 0;) {
            switch (sdl_event.type) {
                case SDL_QUIT:
                    exit = true;
                    break;
                case SDL_MOUSEMOTION:
                    camera.x += sdl_event.motion.xrel;
                    camera.y += sdl_event.motion.yrel;
                    break;
            }
        }

        // update sprite src
        sdl_src.x = (sdl_src.x + 8) % sdl_img->w;

        // draw
        SDL_FillRect(sdl_surface, NULL, SDL_MapRGB(sdl_surface->format, 0, 0, 0));

        for (size_t s = 0; s < 128; ++s) {
            SDL_Rect sdl_star_pos = sdl_stars[s];
            sdl_star_pos.x += camera.x;
            sdl_star_pos.y += camera.y;

            SDL_BlitSurface(sdl_img, &sdl_src, sdl_surface, &sdl_star_pos);
        }
        
        SDL_UpdateWindowSurface(sdl_window);

        // sleep
        SDL_Delay(16);
    }

    return EXIT_SUCCESS;
}