#include "sdldglutils/sdlglutils.h"
#include "fonts.h"

struct color
{
    short unsigned int r, g, b, a;
};

extern SDL_Surface *screen;
extern vec angle;
extern int w, h;

enum { FONT_DEFAULT = 0 };

void init_sdl();
void load_fonts();
void render();
void render_scale();
void render_background();
void render_planets();

// HUD
void draw_text(const char *text, double x, double y, double z, int r = 255, int g = 255, int b = 255, int font = FONT_DEFAULT);
void draw_text_recursive(const char *text, double x, double y, double z, int r = 255, int g = 255, int b = 255, int font = FONT_DEFAULT);
void draw_misc();
void draw_planet_state(Planet *pl);
void draw_buttons();