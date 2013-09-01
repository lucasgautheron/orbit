#include "project.h"

int buttons_width = 0;

Button buttons[B_NUM] =
{
    { "startpause", "Demarrer la simulation", true, true, 6*32, 0, 32, 32, 0, 0, start_pause, false, false },

    { "faster", "Accelerer (x2)",  true, true, 32, 0, 32, 32, 32, 0, increase_speed, false, false },
    { "slower", "Ralentir (x0.5)",  true, true, 64, 0, 32, 32, 2*32, 0, lower_speed, false, false },

    { "zoomin", "Zoom +", true, true, 0, 0, 32, 16, 32*3, 16, zoom_in, false, false },
    { "zoomout", "Zoom -", true, true, 0, 16, 32, 16, 32*3, 0, zoom_out, false, false },

    { "prevref", "Referentiel prec.", true, true, 3*32, 16, 32, 16, 32*4, 16, prev_ref, false, false },
    { "nextref", "Referentiel suiv.", true, true, 3*32, 0, 32, 16, 32*4, 0, next_ref, false, false },

    { "smoother", "Points +20%", true, true, 4*32, 16, 32, 16, 32*5, 16, smoother, false, false},
    { "harder", "Points -20%", true, true, 4*32, 0, 32, 16, 32*5, 0, harder, false, false},

    { "studied", "Etudier planete suiv.", true, true, 5*32, 0, 32, 32, 6*32, 0, studied, false, false},

    { "rewind", "Retour etat initial", true, true, 0, 32, 32, 32, 7*32, 0, rewind, false, false},
};

GLuint buttons_tex;

void start_pause(int force)
{
    if(force >= 0) running = force ? true : false;
    else running = !running;
    
    if(running) { buttons[B_STARTPAUSE].label = "Pause"; buttons[B_STARTPAUSE].tex_x = 7*32; }
    else { buttons[B_STARTPAUSE].label = "Demarrer la simulation"; buttons[B_STARTPAUSE].tex_x = 6*32; }
}

void start_pause() { start_pause(-1); }

void increase_speed() { if(speed < 3000 ){ speed *= 2.0; precision *= 2.0; } }
void lower_speed() { if (speed >= 0.01) { speed /= 2.0; precision /= 2.0; } }
void zoom_in() { scalefactor = max(scalefactor/2.5, 1e7);  }
void zoom_out() { scalefactor = min(scalefactor*2.5, 1e15); }
void prev_ref()
{
    reference--;
    if(reference < 0) reference = planets.length()-1;
    
    bool was_running = running;
    running = false;
    SDL_Delay(5);

    loopv(i, planets) planets[i]->resethist();
    loopv(i, satellites) satellites[i]->resethist();

    running = was_running;
}
void next_ref()
{
    reference++;
    if(reference >= planets.length()) reference = 0;

    bool was_running = running;
    running = false;
    SDL_Delay(5);

    loopv(i, planets) planets[i]->resethist();
    loopv(i, satellites) satellites[i]->resethist();

    running = was_running;
}

void smoother()
{
    if(seconds_per_dot > 0.05*86400)
    {
        bool was_running = running;
        running = false;
        SDL_Delay(5);

        loopv(i, planets) planets[i]->resethist();
        loopv(i, satellites) satellites[i]->resethist();

        seconds_per_dot /= 1.2;

        running = was_running;
    }
}

void harder()
{
    if(seconds_per_dot < 5*86400)
    {
        bool was_running = running;
        running = false;
        SDL_Delay(5);

        loopv(i, planets) planets[i]->resethist();
        loopv(i, satellites) satellites[i]->resethist();

        seconds_per_dot *= 1.2;

        running  = was_running;
    }
}

void studied()
{
    int n = planets.find(curplanet);
    if(planets.inrange(++n)) curplanet = planets[n];
    else curplanet = planets.inrange(0) ? planets[0] : NULL;
}

void rewind()
{
    dbgoutf("Retour aux conditions initiales...");
    start_pause(0);
    bool planets_loaded = false;

    SDL_Delay(10);

    loopv(i, planets) planets[i]->resethist();
    loopv(i, satellites) satellites[i]->resethist();

    if(planets_file && planets_file[0] != '\0') planets_loaded = load_planets(planets_file);
    if(!planets_loaded)
    {
        planets_loaded = load_planets(DEFAULT_PLANETS_FILE);
        if(!planets_loaded)
        {
            dbgoutf("Impossible de charger les planetes. Fermeture du programme.");
            callback_quit();
        }
    }

    curplanet = planets[0];
}

const int basex = 0;


void init_buttons()
{
    buttons_tex = loadTexture("../pck/boutons.jpg", false);
    loop(i, B_NUM) if(buttons[i].x+buttons[i].width > buttons_width) buttons_width = buttons[i].x+buttons[i].width;
}

void mouse_click(short int mouse_button, int mx, int my, bool isdown)
{
    if(!isdown)
    {
        loop(i, B_NUM) if(buttons[i].pressed) buttons[i].pressed = false;
    }
    else
    {
        loop(i, B_NUM)
        {
            int transx = (w-buttons_width)/2;
            int x = transx+buttons[i].x, y = 25+buttons[i].y, xs = x+buttons[i].width, ys = y+buttons[i].height;
            if(mx >= x && mx <= xs
                && my >= y && my <= ys)
            {
                if(buttons[i].action) buttons[i].action();
                buttons[i].pressed = true;
                break;
            }
        }
    }
}