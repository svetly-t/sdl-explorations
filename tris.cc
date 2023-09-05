#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cstdlib>
#include <vector>

struct v2d {
    float x = 0;
    float y = 0;
};

struct v2d_int {
    int x = 0;
    int y = 0;
};

struct edge {
    v2d &v1;
    v2d &v2;
    edge(v2d _v1, v2d _v2) : v1(_v1), v2(_v2) {}
};

struct tri {
    const v2d *v1;
    const v2d *v2;
    const v2d *v3;
    tri(const v2d *_v1, const v2d *_v2, const v2d *_v3) : v1(_v1), v2(_v2), v3(_v3) {}
    float area() {
        // A = (1/2) |x1(y2 − y3) + x2(y3 − y1) + x3(y1 − y2)|
        float a = (v1->x * (v2->y - v3->y) + v2->x * (v3->y - v1->y) + v3->x * (v1->y - v2->y));
        if (a < 0) a = -a;
        return 0.5 * a;
    }
    bool contains(v2d &pt) {
        return (tri(v1, v2, &pt).area() + tri(v1, &pt, v3).area() + tri(&pt, v2, v3).area()) <= area(); 
    }
};

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

    // Get renderer
    // SDL_Renderer *sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_SOFTWARE);

    SDL_Surface *sdl_surface = SDL_GetWindowSurface(sdl_window);
    if (!sdl_surface) {
        std::cout << "No surface " << SDL_GetError() << std::endl;
        abort();
    }

    SDL_Renderer *sdl_renderer = SDL_CreateSoftwareRenderer(sdl_surface);

    // triangle
    v2d verts[3];
    verts[0].x = kWindowX / 2;
    verts[0].y = kWindowY / 2 - 40;
    verts[1].x = kWindowX / 2 + 40;
    verts[1].y = kWindowY / 2 + 30;
    verts[2].x = kWindowX / 2 - 40;
    verts[2].y = kWindowY / 2 + 30;
    tri triangle(&verts[0], &verts[1], &verts[2]);

    // mouse pos
    v2d cursor;

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
                    cursor.x = sdl_event.motion.x;
                    cursor.y = sdl_event.motion.y;
                    break;
            }
        }

        // overlap?
        bool overlap = triangle.contains(cursor);

        // clear surface
        SDL_FillRect(sdl_surface, NULL, SDL_MapRGB(sdl_surface->format, 0, 0, 0));

        uint8_t gb = overlap ? 0 : 255;
        SDL_SetRenderDrawColor(sdl_renderer, 255, gb, gb, 255);

        // draw cross at cursor pos
        SDL_RenderDrawLine(sdl_renderer, cursor.x - 5.0, cursor.y, cursor.x + 5.0, cursor.y);
        SDL_RenderDrawLine(sdl_renderer, cursor.x, cursor.y - 5.0, cursor.x, cursor.y + 5.0);
        // SDL_RenderDrawLine(sdl_renderer, verts[0].x, verts[0].y, verts[1].x, verts[1].y);

        // tri -> array of SDL points
        SDL_Point points[4];
        points[0].x = triangle.v1->x;
        points[0].y = triangle.v1->y;
        points[1].x = triangle.v2->x;
        points[1].y = triangle.v2->y;
        points[2].x = triangle.v3->x;
        points[2].y = triangle.v3->y;      
        points[3].x = triangle.v1->x;
        points[3].y = triangle.v1->y;

        // draw tri
        SDL_RenderDrawLines(sdl_renderer, points, 4);  

        SDL_UpdateWindowSurface(sdl_window);

        // sleep
        SDL_Delay(16);
    }

    return EXIT_SUCCESS;
}