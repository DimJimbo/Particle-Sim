#include <iostream>
#include <random>
#include <ctime>
#include <cmath>

#include "../Simulator/containers.h"

#include <SDL2/SDL.h>


using namespace std;

random_device dev;
mt19937 rng(dev());

class Body {
    public:
        Vector2 position, velocity, acceleration;
        RGBColor color;

        Body(Vector2 _position, Vector2 _velocity, RGBColor _color) : color(_color), position(_position), velocity(_velocity), acceleration{0, 0} {}
};

struct Options {
    int WINDOW_WIDTH = 1300;
    int WINDOW_HEIGHT = 1000;
    long SIM_WIDTH = 40000;
    long SIM_HEIGHT = 40000;

    int BODY_MASS = 100;
    int BODY_RADIUS = 3;
    int BODY_N = 1000;

    const char* BodyCreationType = "COLLISION";
};

class Display {
private:
    SDL_Renderer* renderer;
    SDL_Window* win;

    Vector2 offset;
    float ZOOM = 1.0;

    int getDisplayX(int x)
    {
        return round((x + offset.x)*ZOOM + WIDTH/2*(1 - ZOOM));
    }

    int getDisplayY(int y)
    {
        return round((y + offset.y)*ZOOM + HEIGHT/2*(1 - ZOOM));
    }

    int getDisplayRadius(int r)
    {
        return round(r*ZOOM);
    }

    void draw(int cx, int cy, int r) // just bresenhams, although chatgpt made this so watch out
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

public:
    int WIDTH, HEIGHT, MOVE_BY = 10;
    float ZOOM_BY = 0.05;

    RGBColor BGCOLOR;

    Display() : WIDTH(0), HEIGHT(0), BGCOLOR(0, 0, 0) {}

    Display(int width, int height, RGBColor bgcolor) : WIDTH(width), HEIGHT(height), BGCOLOR(bgcolor)
    {
        Init(width, height, bgcolor);
    }

    void Init(int width, int height, RGBColor bgcolor)
    {
        WIDTH = width;
        HEIGHT = height;
        BGCOLOR = bgcolor;

        if (SDL_Init(SDL_INIT_VIDEO) < 0) // Init SDL, specifically video (there is audio and others too)
        {
            cout << "Could not init SDL";
            exit(-1);
        }
        
        // Create the SDL window
        // "test" is the name of the window, next 2 arguments are the x and y placement of the window on the screen (leaving them undefined lets the os place it)
        // next 2 are width and height, final are any flags (HW ACCELERATION and the like)
        win = SDL_CreateWindow("test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, 0);
        renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED); // Create the renderer, the -1 tells it to use whatever graphics devcie it finds first
    }

    void End()
    {
        SDL_DestroyWindow(win);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
    }

    void update_screen(vector<Body*> bodies, int BODY_RADIUS)
    {
        int dr = getDisplayRadius(BODY_RADIUS);
        for (int i = 0; i < bodies.size(); i++) {
            Body& b = *bodies[i];
            int x = getDisplayX(b.position.x);
            int y = getDisplayY(b.position.y);
            
            if (x < 0 || y < 0 || x > WIDTH || y > HEIGHT) continue;
            // if (i == 900) {
            //     cout << x << ", " << round((b.position.x + offset_x)*zoom + opt.WIDTH/2*(1 - zoom)) << "\n";
            // }

            SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, b.color.a);
            draw(x, y, dr);
        }
        SDL_RenderPresent(renderer); // draw all stuff on screen
    }

    void clear_screen()
    {
        SDL_SetRenderDrawColor(renderer, BGCOLOR.r, BGCOLOR.g, BGCOLOR.b, BGCOLOR.a); // set drawing color
        SDL_RenderClear(renderer); // clear screen (with the drawing color)
    }

    void setOffset(Vector2 new_offset)
    {
        offset = new_offset;
    }

    void setOffset(int new_offset_x, int new_offset_y)
    {
        offset.x = new_offset_x;
        offset.y = new_offset_y;
    }

    void offsetBy(Vector2 add_offset)
    {
        offset += add_offset/ZOOM;
    }

    void offsetBy(int add_x, int add_y)
    {
        offset.x += add_x/ZOOM;
        offset.y += add_y/ZOOM;
    }

    void moveRight()
    {
        offset.x -= MOVE_BY/ZOOM;
    }

    void moveLeft()
    {
        offset.x += MOVE_BY/ZOOM;
    }

    void moveUp()
    {
        offset.y += MOVE_BY/ZOOM;
    }

    void moveDown()
    {
        offset.y -= MOVE_BY/ZOOM;
    }

    void zoom()
    {
        ZOOM += ZOOM_BY;
    }

    void unzoom()
    {
        if (ZOOM - ZOOM_BY > 0) ZOOM -= ZOOM_BY;
    }
};

class Simulator {
    public:

        int CORRECT_OVERLAP_BY = 1, COLLISION_ITERS = 5;
        long MIN_GRAV_DIST, MAX_GRAV_DIST = 2000; 
        float E = 0, elasticity = 0.3, dt = 1;

        Options opt;

        Display display;
        
        vector<Body*> bodies = {};

        Simulator(Options* options) : opt(*options)
        {

            cout << "Initiating Display\n";
            display.Init(opt.WINDOW_WIDTH, opt.WINDOW_HEIGHT, RGBColor(0, 0, 0));
            cout << "Initiating body vector\n";
            initBodies();
            cout << "Done\n";

            G_MASS_MASS = G*opt.BODY_MASS*opt.BODY_MASS;
            MIN_GRAV_DIST = 2*opt.BODY_RADIUS;
        }

        Vector2 getCenterOfMass()
        {
            Vector2 sum(0, 0);
            for (int i = 0; i < opt.BODY_N; i++) {
                sum += bodies[i]->position;
            }

            return sum/opt.BODY_N;
        }

        int mainloop()
        {
            bool running = true;
            int fps = 0;
            long total_frames = 0;
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
                                    display.moveUp();
                                    break;
                                case SDLK_a:
                                    display.moveLeft();
                                    break;
                                case SDLK_s:
                                    display.moveDown();
                                    break;
                                case SDLK_d:
                                    display.moveRight();
                                    break;
                                case SDLK_q:
                                    display.unzoom();
                                    break;
                                case SDLK_e:
                                    display.zoom();
                                    break;
                                case SDLK_p:
                                    ispaused = !ispaused;
                                    break;
                                case SDLK_c:
                                    followingcom = !followingcom;
                                    break;
                                case SDLK_n:
                                    dt -= 0.1;
                                    cout << dt << "\n";
                                    break;
                                case SDLK_m:
                                    dt += 0.1;
                                    cout << dt << "\n";
                                    break;
                            }
                    }
                }

                if (!ispaused) { 
                    updateBodies();
                    handleCollisions();
                    // cout << E << "\n";
                }
                if (followingcom)
                {
                    Vector2 com = getCenterOfMass();
                    int offset_x = round(opt.WINDOW_WIDTH/2 - com.x);
                    int offset_y = round(opt.WINDOW_HEIGHT/2 - com.y);

                    display.setOffset(offset_x, offset_y);
                }
                
                display.clear_screen();
                display.update_screen(bodies, opt.BODY_RADIUS);
            }
            // cout << ((float) total_frames) / 1000 << " kFrames\n";
            display.End();

            return 0;

        }

 
    private:
        float G_MASS_MASS;
        
        bool ispaused = false, followingcom = false; // com = center of mass

        const float epsilon = 0.001, G = 6.67e-6;

        void initBodies()
        {
            
            uniform_int_distribution<int> getRandomRGBValue(255, 255);

            if (opt.BodyCreationType == "RANDOM") {
                uniform_int_distribution<int> getRandomX(0, opt.SIM_WIDTH);
                uniform_int_distribution<int> getRandomY(0, opt.SIM_HEIGHT);
                for (int i = 0; i < opt.BODY_N; i++) {
                    Vector2 position(getRandomX(rng), getRandomY(rng));
                    RGBColor color(getRandomRGBValue(rng), getRandomRGBValue(rng), getRandomRGBValue(rng));
                    bodies.push_back(
                        new Body(position, Vector2(0, 0), color)
                    );
                }
            } else if (opt.BodyCreationType == "COLLISION")
            {
                int extra_pad = 20; // extra padding between bodies
                int pbc = 2*opt.BODY_RADIUS + extra_pad; // (p)adding (b)etween the (c)enter of the circles
                
                int b_br = 500;
                int b_tl = opt.BODY_N - b_br;
                
                Vector2 v_tl(0, 0);
                Vector2 v_br(-1, -1.005);

                MAX_GRAV_DIST = sqrt(opt.SIM_WIDTH*opt.SIM_WIDTH + opt.SIM_HEIGHT*opt.SIM_HEIGHT); // so the bodies are attracted to eachother

                // (c)ircles per (s)ide of the (s)quare at each corner, the size of which is based on pbc and ratio_tl_br
                int csstl = round(sqrt(b_tl));
                int cssbr = round(sqrt(b_br));

                uniform_int_distribution<int> getRandomDeviation(0, extra_pad);

                // the turnary is for adding +1 iterations when the square thats made is less than the square that should be made (as the bottom row is not complete)
                for (int i = 0; i < (csstl + ((csstl*csstl > opt.BODY_N) ? 0 : 1)); i++) { // first square, in top left
                    for (int j = 0; j < min(csstl, b_tl - i*csstl); j++) {
                        Vector2 position(pbc*(j + 0.5) + getRandomDeviation(rng), pbc*(i + 0.5) + getRandomDeviation(rng));
                        RGBColor color(getRandomRGBValue(rng), getRandomRGBValue(rng), getRandomRGBValue(rng));
                        bodies.push_back(new Body(position, v_tl, color));
                    }
                }

                for (int i = 0; i < (cssbr + ((cssbr*cssbr > opt.BODY_N) ? 0 : 1)); i++) { // second square, in bottom right
                    for (int j = 0; j < min(cssbr, b_br - i*cssbr); j++) {
                        Vector2 position(opt.SIM_WIDTH/2 - pbc*(j + 0.5) + getRandomDeviation(rng), opt.SIM_HEIGHT/2 - pbc*(i + 0.5) + getRandomDeviation(rng));
                        RGBColor color(getRandomRGBValue(rng), getRandomRGBValue(rng), getRandomRGBValue(rng));
                        bodies.push_back(new Body( position, v_br, color));
                    }
                }
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
                    Body& b1 = *bodies[i];
                    
                    for (int j = i + 1; j < opt.BODY_N; j++) {
                        Body& b2 = *bodies[j];
                        if (b1.position.getDistanceTo(b2.position) < MIN_GRAV_DIST)
                        {
                            // They're saved as pairs
                            colliding_bodies.push_back(&b1);
                            colliding_bodies.push_back(&b2);
                            colliding_BODY_N += 2;
                        }
                    }
                }

                // then apply changes to resolve each collision
                for (int i = 0; i < colliding_BODY_N; i += 2) {

                    Body& b1 = *colliding_bodies[i];
                    Body& b2 = *colliding_bodies[i + 1];
                        
                    // find the overlap and correct it
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

            Vector2 F_gravity = G_MASS_MASS/(diff.getLengthSquared() + epsilon)*diff.normalized();

            return F_gravity;

        }

        void updateAccelerations()
        {
            for (int i = 0; i < opt.BODY_N; i++) {
                Body& b1 = *bodies[i];
                for (int j = i + 1; j < opt.BODY_N; j++) {
                    Body& b2 = *bodies[j];

                    float dist = b1.position.getDistanceTo(b2.position);
                    
                    if (dist <= MIN_GRAV_DIST || dist >= MAX_GRAV_DIST) continue;
                    
                    Vector2 A = getF(b1, b2)/opt.BODY_MASS;
                    
                    b1.acceleration += A;
                    b2.acceleration -= A;
                }
            }
        }

        // Known Bug: if dist is 0, everything crashes, which pretty much only happens due to bad placement during body generation
        void updateBodies() // Uses symplectic euler
        {
            E = 0;

            updateAccelerations();
            
            for (int i = 0; i < opt.BODY_N; i++) {
                Body& b = *bodies[i];

                b.velocity += b.acceleration*dt;
                b.position += b.velocity*dt;

                b.acceleration *= 0;
                
                E += opt.BODY_MASS*(b.velocity).getLengthSquared()/2;
            }

            

        }

};

int main()
{
    Options* options = new Options();
    Simulator sim(options); 
    sim.mainloop();
}
