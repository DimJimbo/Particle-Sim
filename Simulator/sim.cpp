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

        Body(int R, int G, int B, int _x, int _y, double _vx = 0, double _vy = 0) : x(_x), y(_y), vx(_vx), vy(_vy), color{R, G, B} {}
};

struct Options {
    int WINDOW_WIDTH = 1000;
    int WINDOW_HEIGHT = 1000;
    int UPDATE_BODIES_EVERY = 1;
    double dt = 1;
    double elasticity = 0.8;
    double G = 6.67e-11;
    
    int BODY_MASS = 1000000;
    int BODY_RADIUS = 3;
    int BODY_N = 500;

    int MAX_GRAV_DIST = 500;

    RGBColor BG_COLOR = {0, 0, 0, 255};

    const char* BodyCreationType = "RANDOM";

};


class Simulator {
    public:
        SDL_Renderer* renderer;
        SDL_Window* win;
        SDL_Surface* surf;

        double G_MASS_MASS;
        int MIN_GRAV_DIST;

        Options opt;

        void (Simulator::*drawBodyFunc)(int, int, int);
        int offset_x = 0, offset_y = 0;
        float zoom = 1;

        vector<Body*> bodies = {};

        Simulator(Options* options) : opt(*options)
        {
            cout << "Initiating SDL...\n";
            initSDL();
            cout << "Initiating body vector\n";
            initBodies();
            cout << "Done\n";

            G_MASS_MASS = opt.G*opt.BODY_MASS*opt.BODY_MASS;
            MIN_GRAV_DIST = 2*opt.BODY_RADIUS;

            switch (opt.BODY_RADIUS) {
                // // Will be implemented later
                // case 1:
                //     drawBodyFunc = &Simulator::drawBody1;
                //     break;
                // case 2:
                //     drawBodyFunc = &Simulator::drawBody2;
                //     break;
                // case 3:
                //     drawBodyFunc = &Simulator::drawBody3;
                default:
                    drawBodyFunc = &Simulator::drawBodyAll;
            }
        }

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

            uniform_int_distribution<int> getRandomX(0, opt.WINDOW_WIDTH);
            uniform_int_distribution<int> getRandomY(0, opt.WINDOW_HEIGHT);
            uniform_int_distribution<int> getRandomRGBValue(0, 255);

            if (opt.BodyCreationType == "RANDOM") {
                for (int i = 0; i < opt.BODY_N; i++) {
                    bodies.push_back(new Body(getRandomRGBValue(rng), getRandomRGBValue(rng), getRandomRGBValue(rng), getRandomX(rng), getRandomY(rng)));
                }
            }
        }
        
        void clear_screen()
        {
            SDL_SetRenderDrawColor(renderer, opt.BG_COLOR.r, opt.BG_COLOR.g, opt.BG_COLOR.b, opt.BG_COLOR.a); // set drawing color
            SDL_RenderClear(renderer); // clear screen (with the drawing color)
        }

        int getDisplayX(int x)
        {
            return round((x + offset_x - opt.WINDOW_WIDTH/2)*zoom + opt.WINDOW_WIDTH/2);
        }

        int getDisplayY(int y)
        {
            return round((y + offset_y - opt.WINDOW_HEIGHT/2)*zoom + opt.WINDOW_HEIGHT/2);
        }

        int getDisplayRadius(int r)
        {
            return round(r*zoom);
        }

        void update_screen()
        {
            for (int i = 0; i < opt.BODY_N; i++) {
                Body& b = *bodies[i];
                int x = getDisplayX(b.x);
                int y = getDisplayY(b.y);
                if (x < 0 || y < 0 || x > opt.WINDOW_WIDTH || y > opt.WINDOW_HEIGHT) continue;
                
                SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, b.color.a);
                (this->*drawBodyFunc)(x, y, getDisplayRadius(opt.BODY_RADIUS));
            }
            SDL_RenderPresent(renderer); // draw all stuff on screen
        }

        void drawBodyAll(int cx, int cy, int r)
        {
            int r_sq = r*r - r;
            for (int x = -r + 1; x < r; x++) {
                int x_sq = x*x;
                for (int y = -r + 1; y < r; y++) {
                    if (x_sq + y*y <= r_sq) SDL_RenderDrawPoint(renderer, x + cx, y + cy);
                }
            }
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
                        case SDL_KEYUP:
                            switch (event.key.keysym.sym)
                            {
                                case SDLK_w:
                                    offset_y += 10;
                                    break;
                                case SDLK_a:
                                    offset_x += 10;
                                    break;
                                case SDLK_s:
                                    offset_y -= 10;
                                    break;
                                case SDLK_d:
                                    offset_x -= 10;
                                    break;
                                case SDLK_q:
                                    if (zoom - 0.05 > 0) zoom -= 0.05;

                                    break;
                                case SDLK_e:
                                    zoom += 0.05;
                                    break;

                            }
                    }
                }

                if (total_frames % opt.UPDATE_BODIES_EVERY == 0) basicGravity();
                clear_screen();
                update_screen();
            }

            SDL_DestroyWindow(win);
            SDL_DestroyRenderer(renderer);
            SDL_Quit();

            return 0;

        }


        // seems like my Vector2 implementation is slow af, so I'll stick with this
        void basicGravity()
        {
            for (int i = 0; i < opt.BODY_N; i++) {
                Body& b1 = *bodies[i];

                for (int j = i + 1; j < opt.BODY_N; j++) {
                    Body& b2 = *bodies[j];

                    double dx = b2.x - b1.x;
                    double dy = b2.y - b1.y;
                    double dist = sqrt(dx*dx + dy*dy);

                    // for splitting vectors to their components
                    double sin = dy/dist;
                    double cos = dx/dist;

                    // Also handle collisions because why not
                    if (dist <= MIN_GRAV_DIST) {
                        // Find the overlap between the bodies, and move them so they dont overlap
                        double overlap = 2*opt.BODY_RADIUS - dist;
                        b1.x -= overlap*cos/2;
                        b1.y -= overlap*sin/2;

                        b2.x += overlap*cos/2;
                        b2.y += overlap*sin/2;
                        
                        // find the relative velocity of the bodies

                        double dvx = b2.vx - b1.vx;
                        double dvy = b2.vy - b1.vy;

                        // find the normal component of the relative velocity

                        double dv_norm = dvx*cos + dvy*sin;

                        if (dv_norm > 0) continue; // means that they are moving away from eachother, sooooo no collision needed

                        // find impulse

                        double J = (1 + opt.elasticity)*dv_norm/2; // normally this need to be multiplied by BODY_MASS, but since we would have divided later...

                        double J_collision_x = J*cos;
                        double J_collision_y = J*sin;

                        // apply shit

                        double Vx = J_collision_x;
                        double Vy = J_collision_y;

                        b1.vx += Vx;
                        b1.vy += Vy;
                        
                        b2.vx -= Vx;
                        b2.vy -= Vy;


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