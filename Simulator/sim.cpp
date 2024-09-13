#include <iostream>
#include <random>
#include <ctime>

#include <set>

#include "../Simulator/containers.h"

#include <SDL2/SDL.h>


using namespace std;

random_device dev;
mt19937 rng(dev());

class Body {
    public:
        Vector2 position, velocity, acceleration;
        RGBColor color;

        Body(int R, int G, int B, int x, int y, float vx = 0, float vy = 0) : color{R, G, B}, position{x, y}, velocity{vx, vy}, acceleration{0, 0} {}
};

struct Options {
    int WINDOW_WIDTH = 1000;
    int WINDOW_HEIGHT = 1000;
    int SIM_WIDTH = 2000;
    int SIM_HEIGHT = 2000;

    int BODY_MASS = 10;
    int BODY_RADIUS = 3;
    int BODY_N = 1000;

    const char* BodyCreationType = "RANDOM";
};


class Simulator {
    public:

        int CORRECT_OVERLAP_BY = 1, COLLISION_ITERS = 5;
        int MIN_GRAV_DIST, MAX_GRAV_DIST = 2000;
        float E = 0, elasticity = 0.5, dt = 10;

        RGBColor BG_COLOR = {0, 0, 0, 255};

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

                if (!ispaused) { 
                    updateBodies();
                    handleCollisions();
                    // cout << E << "\n";
                }
                clear_screen();
                update_screen();
            }
            // cout << ((float) total_frames) / 1000 << " kFrames\n";
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

        float G_MASS_MASS;
        
        int offset_x = 0, offset_y = 0;
        float zoom = 1.0;
        bool ispaused = false;

        const float epsilon = 0.001, G = 6.67e-6;

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
        
        void clear_screen()
        {
            SDL_SetRenderDrawColor(renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a); // set drawing color
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

        //=====================MAIN FUNCTIONS=====================//

        void update_screen()
        {
            for (int i = 0; i < opt.BODY_N; i++) {
                Body& b = *bodies[i];
                int x = getDisplayX(b.position.x);
                int y = getDisplayY(b.position.y);
                if (x < 0 || y < 0 || x > opt.WINDOW_WIDTH || y > opt.WINDOW_HEIGHT) continue;
                
                SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, b.color.a);
                drawFunc(x, y, getDisplayRadius(opt.BODY_RADIUS));
            }
            SDL_RenderPresent(renderer); // draw all stuff on screen
        }

        void drawFunc(int cx, int cy, float r) // just bresenhams, although chatgpt made this so watch out
        {
            int x = 0;
            int y = r;
            int d = 3 - 2*r;

            while (y >= x) {
                SDL_RenderDrawLine(renderer, cx - x, cy + y, cx + x, cy + y);
                SDL_RenderDrawLine(renderer, cx - x, cy - y, cx + x, cy - y);
                SDL_RenderDrawLine(renderer, cx - y, cy + x, cx + y, cy + x);
                SDL_RenderDrawLine(renderer, cx - y, cy - x, cx + y, cy - x);

                if (d <= 0) {
                    d += 4*x + 6;
                } else {
                    d += 4*(x - y) + 10;
                    y--;
                }
                x++;
            }
        }

        void handleCollisions()
        {
            for (int k = 0; k < COLLISION_ITERS; k++) {
                // first get a list of all colliding bodies
                // By doing this first, it improves cache locality and increases hits, as the generated array is (in most cases) small
                vector<Body*> colliding_bodies = {};
                int colliding_BODY_N = 0;
                for (int i = 0; i < opt.BODY_N; i++) {
                    Body* b1 = bodies[i];
                    
                    for (int j = i + 1; j < opt.BODY_N; j++) {
                        Body* b2 = bodies[j];
                        if (b1->position.getDistanceTo(b2->position) < MIN_GRAV_DIST)
                        {
                            // They're saved as pairs
                            colliding_bodies.push_back(b1);
                            colliding_bodies.push_back(b2);
                            colliding_BODY_N += 2;
                        }
                    }
                }

                // then apply changes to resolve each collision
                for (int i = 0; i < colliding_BODY_N; i += 2) {

                    Body& b1 = *colliding_bodies[i];
                    Body& b2 = *colliding_bodies[i + 1];
                        
                    // find the 
                    Vector2 diff = b2.position - b1.position;
                    Vector2 diff_normalized = diff.normalized();
                    Vector2 overlap = (2*opt.BODY_RADIUS - diff.getLength())*diff_normalized;

                    b1.position -= overlap/2*CORRECT_OVERLAP_BY;
                    b2.position += overlap/2*CORRECT_OVERLAP_BY;
                    
                    // find the relative velocity of the bodies

                    Vector2 dV = b2.velocity - b1.velocity;

                    // find the normal component of the relative velocity

                    float dV_norm = dV.dot(diff_normalized);

                    if (dV_norm > 0) continue; // means that they are moving away from eachother, sooooo no collision needed, although idk if its needed

                    // find impulse

                    Vector2 J = (1 + elasticity)*dV_norm/2*diff_normalized; // normally this need to be multiplied by BODY_MASS, but we would have divided later...

                    // apply shit

                    b1.velocity += J;
                    b2.velocity -= J;
                }
            }
        }

        Vector2 getF(Body& b1, Body& b2)
        {
            Vector2 diff = b2.position - b1.position;

            Vector2 F_gravity = G_MASS_MASS/diff.getLengthSquared()*diff.normalized();

            return F_gravity;

        }

        // Known Bug: if dist is 0, everything crashes, which pretty much only happens due to bad placement during body generation
        void updateBodies() // Uses symplectic euler
        {
            E = 0;

            for (int i = 0; i < opt.BODY_N; i++) {
                Body& b1 = *bodies[i];

                for (int j = i + 1; j < opt.BODY_N; j++) {
                    Body& b2 = *bodies[j];

                    float dist = b2.position.getDistanceTo(b1.position);
                    
                    if (dist < MIN_GRAV_DIST || dist > MAX_GRAV_DIST) continue;
                    
                    Vector2 A = getF(b1, b2)/opt.BODY_MASS;
                    
                    b1.acceleration += A;
                    b2.acceleration -= A;
                }

                b1.velocity += b1.acceleration*dt;
                b1.position += b1.velocity;
            
                b1.acceleration *= 0;
                E += opt.BODY_MASS*b1.velocity.getLengthSquared()/2;
            }
        }

};

int main()
{
    Options* options = new Options();
    Simulator sim(options); 
    sim.mainloop();
}
