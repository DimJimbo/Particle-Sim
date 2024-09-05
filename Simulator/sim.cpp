#include <iostream>
#include <vector>
#include <random>
#include <ctime>

#include "containers.h>"

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800


using namespace std;

random_device dev;
mt19937 rng(dev());
uniform_int_distribution<int> getRandomX(0, WINDOW_WIDTH);
uniform_int_distribution<int> getRandomY(0, WINDOW_HEIGHT);
uniform_int_distribution<int> getRandomRGBValue(0, 255);

class Body {
    public:
        float x, y, vx, vy;
        RGBColor color;

        Body(int R, int G, int B, int _x, int _y, double _vx = 0, double _vy = 0) : x(_x), y(_y), vx(_vx), vy(_vy), color{R, G, B} {}
};



class Simulator {
    public:
        SDL_Renderer* renderer;
        SDL_Window* win;
        SDL_Surface* surf;

        int BG_COLOR[4] = {0, 0, 0, 255};

        double dt;
        double G = 6.67e-11;

        float elasticity;

        double G_MASS_MASS;
        int MIN_GRAV_DIST, MAX_GRAV_DIST;

        int N, BODY_RADIUS, BODY_MASS;

        void (Simulator::*drawBodyFunc)(int, int);

        vector<Body*> bodies = {};

        Simulator(
            int BODY_N, int _BODY_MASS, int _BODY_RADIUS = 1, 
            float _dt = 0.01, float _elasticity = 1,  
            int MAX_GRAVITY_APPLICATION_DISTANCE = 500
            ) : N(BODY_N), BODY_MASS(_BODY_MASS), BODY_RADIUS(_BODY_RADIUS), dt(_dt), elasticity(_elasticity), MAX_GRAV_DIST(MAX_GRAVITY_APPLICATION_DISTANCE)
        {
            cout << "Initiating SDL...\n";
            initSDL();
            cout << "Initiating body vector\n";
            initBodies();
            cout << "Done\n";

            G_MASS_MASS = G*BODY_MASS*BODY_MASS;
            MIN_GRAV_DIST = 2*BODY_RADIUS;

            switch (BODY_RADIUS) {
                case 1:
                    drawBodyFunc = &Simulator::drawBody1;
                    break;
                case 2:
                    drawBodyFunc = &Simulator::drawBody2;
                    break;
                case 3:
                    drawBodyFunc = &Simulator::drawBody3;
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
            win = SDL_CreateWindow("test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 800, 0);
            renderer = SDL_CreateRenderer(win, -1, 0); // Create the renderer, the -1 tells it to use whatever graphics devcie it finds first
            surf = SDL_GetWindowSurface(win); // create the drawing surface
        }

        void initBodies()
        {
            for (int i = 0; i < N; i++)
            {
                bodies.push_back(new Body(getRandomRGBValue(rng), getRandomRGBValue(rng), getRandomRGBValue(rng), getRandomX(rng), getRandomY(rng)));
            }
        }
        
        void clear_screen()
        {
            SDL_SetRenderDrawColor(renderer, BG_COLOR[0], BG_COLOR[1], BG_COLOR[2], BG_COLOR[3]); // set drawing color
            SDL_RenderClear(renderer); // clear screen (with the drawing color)
        }

        void update_screen()
        {
            for (int i = 0; i < N; i++) {
                Body* b = bodies[i];
                int x = round(b->x);
                int y = round(b->y);

                if (x < 0 || y < 0 || x > WINDOW_WIDTH || y > WINDOW_HEIGHT) continue;
                
                SDL_SetRenderDrawColor(renderer, b->color.r, b->color.g, b->color.b, b->color.a);
                (this->*drawBodyFunc)(x, y);
            }
            SDL_RenderPresent(renderer); // draw all stuff on screen
        }

        void drawBody1(int x, int y)
        {
            SDL_RenderDrawPoint(renderer, x, y);
        }
        void drawBody2(int x, int y)
        {
            SDL_RenderDrawPoint(renderer, x, y);
            SDL_RenderDrawPoint(renderer, x + 1, y);
            SDL_RenderDrawPoint(renderer, x, y - 1);
            SDL_RenderDrawPoint(renderer, x + 1, y - 1);
        }
        void drawBody3(int x, int y)
        {
            SDL_RenderDrawPoint(renderer, x, y);
            SDL_RenderDrawPoint(renderer, x, y - 1);
            SDL_RenderDrawPoint(renderer, x, y - 2);
            SDL_RenderDrawPoint(renderer, x - 1, y - 1);
            SDL_RenderDrawPoint(renderer, x + 1, y - 1);
        }

        void drawBodyAll(int cx, int cy)
        {
            int r_sq = BODY_RADIUS*BODY_RADIUS - BODY_RADIUS;
            for (int x = -BODY_RADIUS + 1; x < BODY_RADIUS; x++) {
                int x_sq = x*x;
                for (int y = -BODY_RADIUS + 1; y < BODY_RADIUS; y++) {
                    if (x_sq + y*y <= r_sq) SDL_RenderDrawPoint(renderer, x + cx, y + cy);
                }
            }
        }

        int mainloop()
        {
            bool running = true;
            int fps = 0;
            time_t t0 = time(nullptr);

            SDL_Event event;
            
            while (running) {
                fps++;
                time_t t1 = time(nullptr);
                if (t1 - t0 >= 1) {
                    cout << fps << "\n";
                    fps = 0;
                    t0 = t1;
                }

                while (SDL_PollEvent(&event) > 0) {
                    if (event.type == SDL_QUIT)
                    {
                        running = false;
                    }
                }
                basicGravity();
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
            for (int i = 0; i < N; i++) {
                Body* b1 = bodies[i];

                for (int j = i + 1; j < N; j++) {
                    Body* b2 = bodies[j];

                    double dx = b2->x - b1->x;
                    double dy = b2->y - b1->y;
                    double dist = sqrt(dx*dx + dy*dy);

                    double sin = dy/dist;
                    double cos = dx/dist;

                    double Vx, Vy; // Will be added to both bodies velocities (but for b2 it will be negative)

                    // Also handle collisions because why not
                    
                    if (dist <= MIN_GRAV_DIST) {
                        // Find the overlap between the bodies, and move them so they dont overlap

                        double overlap = 2*BODY_RADIUS - dist;
                        b1->x -= overlap*cos/2;
                        b1->y -= overlap*sin/2;

                        b2->x += overlap*cos/2;
                        b2->y += overlap*sin/2;
                        

                        // find the relative velocity of the bodies

                        double dvx = b2->vx - b1->vx;
                        double dvy = b2->vy - b1->vy;

                        // find the normal component of the relative velocity

                        double dv_norm = dvx*cos + dvy*sin;

                        if (dv_norm > 0) continue; // means that they are moving away from eachother, sooooo no collision needed

                        // find impulse

                        double arg = (1 + elasticity)*dv_norm/2; // normally this need to be multiplied by BODY_MASS, but since we would have divided later...

                        double J_collision_x = arg*cos;
                        double J_collision_y = arg*sin;

                        // apply shit

                        Vx = J_collision_x;
                        Vy = J_collision_y;

                        b1->vx += Vx;
                        b1->vy += Vy;
                        
                        b2->vx -= Vx;
                        b2->vy -= Vy;


                    } else if (dist > MAX_GRAV_DIST) {
                        continue;
                    } else {

                        double F_gravity = G_MASS_MASS/dist/dist;
                        double F_gravity_x = F_gravity*cos;
                        double F_gravity_y = F_gravity*sin;

                        double Vx = F_gravity_x/BODY_MASS*dt;
                        double Vy = F_gravity_y/BODY_MASS*dt;

                        b1->vx += Vx;
                        b1->vy += Vy;
                        
                        b2->vx -= Vx;
                        b2->vy -= Vy;
                    }
                }
                b1->x += b1->vx*dt;
                b1->y += b1->vy*dt;
            }
        }
};

int main()
{
    Simulator sim(1000, 100000000, 3, 1, 0.5); 
    sim.mainloop();
}