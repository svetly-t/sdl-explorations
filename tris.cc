#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>

#include <cstdlib>
#include <vector>

struct v2d {
    float x = 0;
    float y = 0;
    float distance(const v2d &o) {
        float dist;
        dist  = (x - o.x) * (x - o.x);
        dist += (y - o.y) * (y - o.y);
        dist = sqrt(dist);
        return dist;
    }
    float magnitude() {
        return sqrt(x * x + y * y);
    }
};

struct vert {
    v2d old;
    v2d pos;
    v2d acc;
    void init() {
        acc.x = 0.0;
        acc.y = 300.0;
        old.x = pos.x;
        old.y = pos.y;
    }
};

struct edge {
    vert &v1;
    vert &v2;
    float dist;
    edge(vert &_v1, vert &_v2) : v1(_v1), v2(_v2) {
        dist = v1.pos.distance(v2.pos);
    }
    void constrain() {
        float d = v1.pos.distance(v2.pos);
        
        float push = (d - dist) / 2.0;

        v2d dir;
        dir.x = v1.pos.x - v2.pos.x;
        dir.y = v1.pos.y - v2.pos.y;
        float mag = dir.magnitude();

        v1.pos.x -= dir.x / mag * push;
        v1.pos.y -= dir.y / mag * push;
        v2.pos.x += dir.x / mag * push;
        v2.pos.y += dir.y / mag * push;
    }
};

struct tri {
    vert &v1;
    vert &v2;
    vert &v3;
    edge e1;
    edge e2;
    edge e3;
    tri(vert &_v1, vert &_v2, vert &_v3) : 
        v1(_v1), v2(_v2), v3(_v3),
        e1(_v1, _v2), e2(_v2, _v3), e3(_v3, _v1)
        {
            v1.init();
            v2.init();
            v3.init();
        }
    static float area(const v2d &v1, const v2d &v2, const v2d &v3) {
        // A = (1/2) |x1(y2 − y3) + x2(y3 − y1) + x3(y1 − y2)|
        float a = (v1.x * (v2.y - v3.y) + v2.x * (v3.y - v1.y) + v3.x * (v1.y - v2.y));
        if (a < 0) a = -a;
        return 0.5 * a;
    }
    float area() const {
        return area(v1.pos, v2.pos, v3.pos);
    }
    bool contains(v2d &pt) const {
        return (tri::area(v1.pos, v2.pos, pt) + tri::area(v1.pos, pt, v3.pos) + tri::area(pt, v2.pos, v3.pos)) <= area() + 0.01; 
    }
    bool overlap(const tri &tr) const {
        return contains(tr.v1.pos) || contains(tr.v2.pos) || contains(tr.v3.pos) ||
               tr.contains(v1.pos) || tr.contains(v2.pos) || tr.contains(v3.pos);
    }
    void constrain() {
        e1.constrain();
        e2.constrain();
        e3.constrain();
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

    // triangle 1
    vert verts[6];
    verts[0].pos.x = kWindowX / 2;
    verts[0].pos.y = kWindowY / 2 - 40;
    verts[1].pos.x = kWindowX / 2 - 40;
    verts[1].pos.y = kWindowY / 2 + 30;
    verts[2].pos.x = kWindowX / 2 + 40;
    verts[2].pos.y = kWindowY / 2 + 30;
    tri triangle1(verts[0], verts[1], verts[2]);

    // triangle 2
    verts[3].pos.x = kWindowX / 2;
    verts[3].pos.y = kWindowY / 2 - 40;
    verts[4].pos.x = kWindowX / 2 + 40;
    verts[4].pos.y = kWindowY / 2 + 30;
    verts[5].pos.x = kWindowX / 2 - 40;
    verts[5].pos.y = kWindowY / 2 + 30;
    tri triangle2(verts[3], verts[4], verts[5]);

    // mouse pos
    v2d cursor;
    bool lmb = false;

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
                case SDL_MOUSEBUTTONDOWN:
                    lmb = true;
                    break;
                case SDL_MOUSEBUTTONUP:
                    lmb = false;
                    break;
            }
        }

        float dt = 16.0 / 1000.0;

        // put one vert at the cursor position
        if (!lmb) {
            verts[0].pos.x = cursor.x;
            verts[0].pos.y = cursor.y;
        }

        // for all verts, update
        for (size_t i = 0; i < 3; ++i) {
            float v_x = verts[i].pos.x - verts[i].old.x;
            float v_y = verts[i].pos.y - verts[i].old.y;

            verts[i].old.x = verts[i].pos.x;
            verts[i].old.y = verts[i].pos.y;

            v2d n_p;
            n_p.x = verts[i].pos.x + (v_x) + verts[i].acc.x * dt * dt;
            n_p.y = verts[i].pos.y + (v_y) + verts[i].acc.y * dt * dt;

            verts[i].pos.x = n_p.x;
            verts[i].pos.y = n_p.y;
        }

        // for all edges, constrain
        triangle1.constrain();

        for (size_t i = 0; i < 3; ++i) {
            if (triangle2.contains(verts[i].pos)) {
                verts[i].pos.x = verts[i].old.x;
                verts[i].pos.y = verts[i].old.y;
            }
        }

        for (size_t i = 3; i < 6; ++i) {
            if (triangle1.contains(verts[i].pos)) {
                // find the 2 verts with smallest area wrt v
                float a1 = tri::area(verts[0].pos, verts[1].pos, verts[i].pos);
                float a2 = tri::area(verts[0].pos, verts[i].pos, verts[2].pos);
                float a3 = tri::area(verts[i].pos, verts[1].pos, verts[2].pos);
                float min = a1;
                if (a2 < min) min = a2;
                if (a3 < min) min = a3;

                // find the normal line to those verts' edge
                v2d edge;
                if (min == a1) {
                    edge.x = verts[1].pos.x - verts[0].pos.x;
                    edge.y = verts[1].pos.y - verts[0].pos.y;
                }
                if (min == a2) {
                    edge.x = verts[0].pos.x - verts[2].pos.x;
                    edge.y = verts[0].pos.y - verts[2].pos.y;
                }
                if (min == a3) {
                    edge.x = verts[2].pos.x - verts[1].pos.x;
                    edge.y = verts[2].pos.y - verts[1].pos.y;
                }

                v2d norm;
                norm.x = edge.y;
                norm.y = -edge.x;

                float mag = edge.magnitude();
                float h = 2 * min / mag;

                for (size_t i = 0; i < 3; ++i) {
                    verts[i].pos.x += norm.x * h / mag; //verts[i].old.x;
                    verts[i].pos.y += norm.y * h / mag; //verts[i].old.y;
                }

                // v2d t2_center;
                // t2_center.x = (triangle2.v1.pos.x + triangle2.v2.pos.x + triangle2.v3.pos.x) / 3.0;
                // t2_center.y = (triangle2.v1.pos.y + triangle2.v2.pos.y + triangle2.v3.pos.y) / 3.0;

                // v2d push;
                // push.x = verts[i].pos.x - t2_center.x;
                // push.y = verts[i].pos.y - t2_center.y;

                // float mag = push.magnitude();

                // for (size_t i = 0; i < 3; ++i) {
                //     verts[i].pos.x = verts[i].old.x + push.x / mag; //verts[i].old.x;
                //     verts[i].pos.y = verts[i].old.y + push.y / mag; //verts[i].old.y;
                // }
            }
        }

        // overlap?
        bool overlap = triangle1.overlap(triangle2); // triangle.contains(cursor);

        // clear surface
        SDL_FillRect(sdl_surface, NULL, SDL_MapRGB(sdl_surface->format, 0, 0, 0));

        uint8_t gb = overlap ? 0 : 255;
        SDL_SetRenderDrawColor(sdl_renderer, 255, gb, gb, 255);

        // draw cross at cursor pos
        SDL_RenderDrawLine(sdl_renderer, cursor.x - 5.0, cursor.y, cursor.x + 5.0, cursor.y);
        SDL_RenderDrawLine(sdl_renderer, cursor.x, cursor.y - 5.0, cursor.x, cursor.y + 5.0);

        // tri1 -> array of SDL points
        SDL_Point points[4];
        points[0].x = triangle1.v1.pos.x;
        points[0].y = triangle1.v1.pos.y;
        points[1].x = triangle1.v2.pos.x;
        points[1].y = triangle1.v2.pos.y;
        points[2].x = triangle1.v3.pos.x;
        points[2].y = triangle1.v3.pos.y;      
        points[3].x = triangle1.v1.pos.x;
        points[3].y = triangle1.v1.pos.y;

        // draw tri1
        SDL_RenderDrawLines(sdl_renderer, points, 4);  

        // tri2 -> array of SDL points
        points[0].x = triangle2.v1.pos.x;
        points[0].y = triangle2.v1.pos.y;
        points[1].x = triangle2.v2.pos.x;
        points[1].y = triangle2.v2.pos.y;
        points[2].x = triangle2.v3.pos.x;
        points[2].y = triangle2.v3.pos.y;      
        points[3].x = triangle2.v1.pos.x;
        points[3].y = triangle2.v1.pos.y;

        // draw tri2
        SDL_RenderDrawLines(sdl_renderer, points, 4);  

        SDL_UpdateWindowSurface(sdl_window);

        // sleep
        SDL_Delay(16);
    }

    return EXIT_SUCCESS;
}