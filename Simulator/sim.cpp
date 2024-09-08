#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <string>

#include "../Simulator/containers.h"

#include <SDL2/SDL.h>


using namespace std;

random_device dev;
mt19937 rng(dev());

class Body {
    public:
        float x, y, vx, vy;
        RGBColor color;

        Body(int R, int G, int B, int _x, int _y, double _vx = 0, double _vy = 0) : color{R, G, B} {
            x = _x;
            y = _y;
            vx = _vx;
            vy = _vy;
        }
};

struct Options {
    int WINDOW_WIDTH = 1000;
    int WINDOW_HEIGHT = 1000;
    int SIM_WIDTH = 1000;
    int SIM_HEIGHT = 1000;
    int UPDATE_BODIES_EVERY = 2;
    double dt = 100;
    double elasticity = 0.5;

    int BODY_MASS = 10;
    int BODY_RADIUS = 3;
    int BODY_N = 1000;

    int MAX_GRAV_DIST = 2000;

    RGBColor BG_COLOR = {0, 0, 0, 255};

    const char* BodyCreationType = "RANDOM";
};


class Simulator {
    public:

        Options opt;

        Body** bodies;

        Simulator(Options* options) : opt(*options)
        {
            cout << "Initiating SDL...\n";
            initSDL();
            cout << "Initiating body vector\n";
            initBodies();
            cout << "Done\n";

            G_MASS_MASS = G*opt.BODY_MASS*opt.BODY_MASS;
            MIN_GRAV_DIST = 2*opt.BODY_RADIUS;
        }

        int mainloop()
        {
            bool running = true;
            int fps = 0;
            int total_frames = 0;
            time_t t0 = time(nullptr);

            SDL_Event event;
            
            while (running) {
                fps++;
                total_frames++;
                time_t t1 = time(nullptr);
                if (t1 - t0 >= 1) {
                    cout << fps << "\n";
                    fps = 0;
                    t0 = t1;
                }

                while (SDL_PollEvent(&event) > 0) {
                    switch (event.type)
                    {
                        case SDL_QUIT:
                            running = false;
                            break;
                        case SDL_KEYDOWN:
                            switch (event.key.keysym.sym)
                            {
                                case SDLK_w:
                                    offset_y += 10/zoom;
                                    break;
                                case SDLK_a:
                                    offset_x += 10/zoom;
                                    break;
                                case SDLK_s:
                                    offset_y -= 10/zoom;
                                    break;
                                case SDLK_d:
                                    offset_x -= 10/zoom;
                                    break;
                                case SDLK_q:
                                    if (opt.BODY_RADIUS*(zoom - 0.05) >= 1) zoom -= 0.05;
                                    break;
                                case SDLK_e:
                                    zoom += 0.05;
                                    break;
                                case SDLK_p:
                                    ispaused = !ispaused;
                                    break;
                            }
                    }
                }

                if (total_frames % opt.UPDATE_BODIES_EVERY == 0 && !ispaused) gravityFunc();
                clear_screen();
                update_screen();
            }

            SDL_DestroyWindow(win);
            SDL_DestroyRenderer(renderer);
            SDL_Quit();
            free(bodies);

            return 0;

        }

 
    private:
        SDL_Renderer* renderer;
        SDL_Window* win;
        SDL_Surface* surf;

        double G_MASS_MASS;
        int MIN_GRAV_DIST;

        int offset_x = 0, offset_y = 0;
        float zoom = 1;
        bool ispaused = false;

        const double epsilon = 0.001, G = 6.67e-6, CORRECT_OVERLAP_BY = 0.8;

        void initSDL()
        {
            if (SDL_Init(SDL_INIT_VIDEO) < 0) // Init SDL, specifically video (there is audio and others too)
            {
                cout << "Could not init SDL";
                exit(-1);
            }
            
            // Create the SDL window
            // "test" is the name of the window, next 2 arguments are the x and y placement of the window on the screen (leaving them undefined lets the os place it)
            // next 2 are width and height, final are any flags (HW ACCELERATION and the like)
            win = SDL_CreateWindow("test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, opt.WINDOW_WIDTH, opt.WINDOW_HEIGHT, 0);
            renderer = SDL_CreateRenderer(win, -1, 0); // Create the renderer, the -1 tells it to use whatever graphics devcie it finds first
            surf = SDL_GetWindowSurface(win); // create the drawing surface
        }

        void initBodies()
        {

            uniform_int_distribution<int> getRandomX(0, opt.SIM_WIDTH);
            uniform_int_distribution<int> getRandomY(0, opt.SIM_HEIGHT);
            uniform_int_distribution<int> getRandomRGBValue(0, 255);

            bodies = (Body**)malloc(sizeof(Body*)*opt.BODY_N); //idk if this is allowed but it works

            if (opt.BodyCreationType == "RANDOM") {
                for (int i = 0; i < opt.BODY_N; i++) {
                    int x = getRandomX(rng), y = getRandomY(rng), R = getRandomRGBValue(rng), G = getRandomRGBValue(rng), B = getRandomRGBValue(rng);
                    
                    bodies[i] = new Body(R, G, B, x, y);
                }
            }
        }
        
        inline void clear_screen()
        {
            SDL_SetRenderDrawColor(renderer, opt.BG_COLOR.r, opt.BG_COLOR.g, opt.BG_COLOR.b, opt.BG_COLOR.a); // set drawing color
            SDL_RenderClear(renderer); // clear screen (with the drawing color)
        }

        inline int getDisplayX(int x)
        {
            return round((x + offset_x - opt.WINDOW_WIDTH/2)*zoom + opt.WINDOW_WIDTH/2);
        }

        inline int getDisplayY(int y)
        {
            return round((y + offset_y - opt.WINDOW_HEIGHT/2)*zoom + opt.WINDOW_HEIGHT/2);
        }

        inline int getDisplayRadius(int r)
        {
            return round(r*zoom);
        }

        inline void update_screen()
        {
            for (int i = 0; i < opt.BODY_N; i++) {
                Body& b = *bodies[i];
                int x = getDisplayX(b.x);
                int y = getDisplayY(b.y);
                if (x < 0 || y < 0 || x > opt.WINDOW_WIDTH || y > opt.WINDOW_HEIGHT) continue;
                
                SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, b.color.a);
                drawFunc(x, y, getDisplayRadius(opt.BODY_RADIUS));
            }
            SDL_RenderPresent(renderer); // draw all stuff on screen
        }

        inline void drawFunc(int cx, int cy, float r) // just bresenhams, although chatgpt made this so watch out
        {
            int x = 0;
            int y = r;
            int d = 3 - 2 * r;

            while (y >= x) {
                SDL_RenderDrawLine(renderer, cx - x, cy + y, cx + x, cy + y);
                SDL_RenderDrawLine(renderer, cx - x, cy - y, cx + x, cy - y);
                SDL_RenderDrawLine(renderer, cx - y, cy + x, cx + y, cy + x);
                SDL_RenderDrawLine(renderer, cx - y, cy - x, cx + y, cy - x);

                if (d <= 0) {
                    d = d + 4 * x + 6;
                } else {
                    d = d + 4 * (x - y) + 10;
                    y--;
                }
                x++;
            }
        }

        void collisionFunc(Body& b1, Body& b2)
        {
            double dx = b2.x - b1.x;
            double dy = b2.y - b1.y;
            double dist = sqrt(dx*dx + dy*dy);

            double sin = dy/dist, cos = dx/dist;
            double overlap = 2*opt.BODY_RADIUS - dist;
                        
            b1.x -= overlap*cos/2*CORRECT_OVERLAP_BY;
            b1.y -= overlap*sin/2*CORRECT_OVERLAP_BY;

            b2.x += overlap*cos/2*CORRECT_OVERLAP_BY;
            b2.y += overlap*sin/2*CORRECT_OVERLAP_BY;
            
            // find the relative velocity of the bodies

            double dvx = b2.vx - b1.vx;
            double dvy = b2.vy - b1.vy;

            // find the normal component of the relative velocity

            double dv_norm = dvx*cos + dvy*sin;

            if (dv_norm > 0) return; // means that they are moving away from eachother, sooooo no collision needed, although idk if its needed

            // find impulse

            double J = (1 + opt.elasticity)*dv_norm/2; // normally this need to be multiplied by BODY_MASS, but since we would have divided later...

            double J_collision_x = J*cos;
            double J_collision_y = J*sin;

            // apply shit

            b1.vx += J_collision_x;
            b1.vy += J_collision_y;

            b2.vx -= J_collision_x;
            b2.vy -= J_collision_y;
        }

        // seems like my Vector2 implementation is slow af, so I'll stick with this
        // Known Bug: if dist is 0, everything crashes, which pretty much only happens due to bad placement during body generation
        void gravityFunc()
        {
            for (int i = 0; i < opt.BODY_N; i++) {
                Body& b1 = *bodies[i];

                for (int j = i + 1; j < opt.BODY_N; j++) {
                    Body& b2 = *bodies[j];

                    double dx = b2.x - b1.x;
                    double dy = b2.y - b1.y;
                    double dist = sqrt(dx*dx + dy*dy);

                    double sin = dy/dist;
                    double cos = dx/dist;
                    
                    if (dist <= MIN_GRAV_DIST) {
                        collisionFunc(b1, b2);
                    } else if (dist > opt.MAX_GRAV_DIST) {
                        continue;
                    } else {

                        double F_gravity = G_MASS_MASS/dist/dist;
                        double F_gravity_x = F_gravity*cos;
                        double F_gravity_y = F_gravity*sin;

                        double Vx = F_gravity_x/opt.BODY_MASS*opt.dt;
                        double Vy = F_gravity_y/opt.BODY_MASS*opt.dt;

                        b1.vx += Vx;
                        b1.vy += Vy;
                        
                        b2.vx -= Vx;
                        b2.vy -= Vy;
                    }
                    
                }
                b1.x += b1.vx;
                b1.y += b1.vy;
            }

        }

};

int main()
{
    Options* options = new Options();
    Simulator sim(options); 
    sim.mainloop();
}
