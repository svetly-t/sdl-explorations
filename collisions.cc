#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cstdlib>
#include <vector>

struct vec2d {
    float x = 0;
    float y = 0;
};

struct vec2d_int {
    int x = 0;
    int y = 0;
};

struct aabb {
    vec2d current;
    float hlen;

    static inline void init_verts(aabb b, vec2d *verts) {
        verts[0].x = b.current.x + b.hlen;
        verts[0].y = b.current.y + b.hlen;
        verts[1].x = b.current.x + b.hlen;
        verts[1].y = b.current.y - b.hlen;
        verts[2].x = b.current.x - b.hlen;
        verts[2].y = b.current.y + b.hlen;
        verts[3].x = b.current.x - b.hlen;
        verts[3].y = b.current.y - b.hlen;
    }
};

bool aabb_overlap(aabb p, aabb q) {
    vec2d p_verts[4];
    vec2d q_verts[4];

    aabb::init_verts(p, p_verts);
    aabb::init_verts(q, q_verts);

    for (size_t i = 0; i < 4; ++i) {
        if (q_verts[i].x <= p.current.x + p.hlen && q_verts[i].x >= p.current.x - p.hlen &&
            q_verts[i].y <= p.current.y + p.hlen && q_verts[i].y >= p.current.y - p.hlen)
            return true;
    }

    return false;
}

SDL_Rect aabb_to_rect(aabb box) {
    SDL_Rect sdl_rect;
    sdl_rect.x = box.current.x - box.hlen;
    sdl_rect.y = box.current.y - box.hlen;
    sdl_rect.w = 2*box.hlen;
    sdl_rect.h = 2*box.hlen;

    return sdl_rect;
}

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

    // Colliders
    struct aabb box1;
    box1.current.x = 250.0;
    box1.current.y = 200.0;
    box1.hlen = 50.0;
    struct aabb box2;
    box2.current.x = 300.0;
    box2.current.y = 300.0;
    box2.hlen = 50.0;

    // camera pos
    SDL_Rect camera = { 0, 0, 0, 0 };
    
    SDL_Event sdl_event;
    bool exit = false;
    for (;!exit;) {
        vec2d_int motion;
        aabb boxA = box1;

        // Event loop
        for (;SDL_PollEvent(&sdl_event) > 0;) {
            switch (sdl_event.type) {
                case SDL_QUIT:
                    exit = true;
                    break;
                case SDL_MOUSEMOTION:
                    motion.x = sdl_event.motion.xrel;
                    motion.y = sdl_event.motion.yrel;
                    boxA.current.x += sdl_event.motion.xrel;
                    boxA.current.y += sdl_event.motion.yrel;
                    break;
            }
        }

        bool overlap = aabb_overlap(boxA, box2);
        if (overlap) {
            for (;motion.x != 0;) {
                int d = motion.x > 0 ? 1 : -1;
                box1.current.x += d;
                if (aabb_overlap(box1, box2)) {
                    box1.current.x -= d;
                    break;
                }
                motion.x -= d;
            }
            for (;motion.y != 0;) {
                int d = motion.y > 0 ? 1 : -1;
                box1.current.y += d;
                if (aabb_overlap(box1, box2)) {
                    box1.current.y -= d;
                    break;
                }
                motion.y -= d;
            }
        } else {
            box1 = boxA;
        }

        // draw
        SDL_FillRect(sdl_surface, NULL, SDL_MapRGB(sdl_surface->format, 0, 0, 0));

        // box struct to sdl native rect
        SDL_Rect sdl_rect1 = aabb_to_rect(box1);
        SDL_Rect sdl_rect2 = aabb_to_rect(box2);

        uint8_t gb = overlap ? 0 : 255;
        
        SDL_SetRenderDrawColor(sdl_renderer, 255, gb, gb, 255);
        SDL_RenderDrawRect(sdl_renderer, &sdl_rect1);
        SDL_RenderDrawRect(sdl_renderer, &sdl_rect2);

        SDL_UpdateWindowSurface(sdl_window);

        // sleep
        SDL_Delay(16);
    }

    return EXIT_SUCCESS;
}