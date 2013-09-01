#include "project.h"

// TODO :
// - allow full screen
// - rewrite render_planets and render_scale with proper matrix usage ?
//   (see http://www.developpez.net/forums/d1108488/applications/developpement-2d-3d-jeux/api-graphiques/opengl/dessiner-repere-xyz/)
// - video record
// - menus


SDL_Surface *screen = NULL;
svector<GPFont *> fonts;
bool rendering_history = false;

vec angle(0,0,0);
int w = 1024, h = 768;

int syzygy_rendermillis = 0;

void init_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
        fprintf(stderr, "Erreur lors de l'initialisation de la SDL : %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    screen = SDL_SetVideoMode(w, h, 32, SDL_OPENGL | SDL_HWSURFACE | SDL_SWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);

    SDL_WM_SetCaption("Orbit", NULL);
    TTF_Init();
    load_fonts();
}

void load_fonts()
{
    fonts.add(new GPFont("consola.ttf", 12));
    //fonts.add(new GPFont("arial.ttf", 12));
    //fonts.add(new GPFont("consola.ttf", 12));
}

void auto_ortho(double scale, double z_scale)
{
    double x_scale = scale, y_scale = scale;
    if(w>h) y_scale *= double(h)/double(w);
    else x_scale *= double(h)/double(w);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-x_scale, x_scale, -y_scale, y_scale, -z_scale, z_scale);
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    if(print_frame)
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glPushMatrix();
        glOrtho(0, w, 0, h, -1000, 1000);
        glColor3ub(255, 255, 255);
        glBegin(GL_QUADS);
            glVertex2i(0,0);
            glVertex2i(w,0);
            glVertex2i(w,h);
            glVertex2i(0,h);
        glEnd();
        glPopMatrix();
    }

    // 3D scene

    render_planets();
    //render_scale();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1000, 1000);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 2D scene
    draw_misc();
    draw_planet_state(curplanet);
    
    if(!print_frame) draw_buttons();

    glFlush();
    SDL_GL_SwapBuffers();
    
    if(print_frame)
    {
        print_frame = false;
        char buf[64] = ""; sprintf(buf, "../images/print_%d.bmp", render_millis.current); takeScreenshot(buf);
    }
}

void render_scale()
{
    vec scalepos(100, 100, 0);

    glPushMatrix();

    glTranslated(scalepos.x, scalepos.y, scalepos.z);

    glBegin(GL_LINES);
    glColor3ub(0,0,255);
    glVertex2d(0, 0);
    glVertex2d(50, 0);

    glColor3ub(0,255,0);
    glVertex2d(0, 0);
    glVertex2d(0, 50);

    glColor3ub(255,0,0);
    glVertex2d(0, 0);
    glVertex3d(0, 0, 50);

    glEnd();
    glPopMatrix();

    draw_text("x", scalepos.x+55, scalepos.y, scalepos.z);
    draw_text("y", scalepos.x, scalepos.y + 55, scalepos.z);
    draw_text("z", scalepos.x, scalepos.y, scalepos.z + 55);
}

bool skip_hists = true, draw_accel = false, draw_area = false;

void draw_vector(vec pos, vec dir, double radius)
{
    double arrowsize = (radius/8);
    dir.normalize();
    vec target = vec(dir).mul(radius).add(pos), arrowbase = vec(dir).mul(radius - arrowsize).add(pos), spoke;
    spoke.orthogonal(dir);
    spoke.normalize();
    spoke.mul(arrowsize);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex3dv(pos.v);
    glVertex3dv(target.v);
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glVertex3dv(target.v);
    loop(j, 5)
    {
        vec p(spoke);
        p.rotate(2*D_PI*j/4.0f, dir);
        p.add(arrowbase);
        glVertex3dv(p.v);
    }
    glEnd();
    glLineWidth(1);
}

void draw_accel_vec(Body *b)
{
    Planet *pl_ref = planets.inrange(reference) ? planets[reference] : planets[0];
    vec ref = pl_ref->pos;

    glColor3f(float(b->r)/255.0f, float(b->g)/255.0f, float(b->b)/255.0f);
    double radius = scalefactor/20;
    
    vec pos = b->pos-ref;
    vec dir = b->a;
    if(pl_ref) dir.sub(pl_ref->a);
    draw_vector(pos, dir, radius);
}

void draw_push_vec(Satellite *s)
{
    Planet *pl_ref = planets.inrange(reference) ? planets[reference] : planets[0];
    vec ref = pl_ref->pos;

    vec dir = s->push;
    vec pos = s->pos-ref;

    double radius = scalefactor/20;

    glColor3f(float(s->r)/255.0f, float(s->g)/255.0f, float(s->b)/255.0f);
    draw_vector(pos, dir, radius);
}

void render_planets()
{
    // planets
    Planet *pl_ref = planets.inrange(reference) ? planets[reference] : planets[0];

    vec ref = pl_ref->pos;
    
    glPushMatrix();

    auto_ortho(scalefactor, scalefactor*100);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glRotated(angle.x,1,0,0);
    glRotated(angle.y,0,1,0);
    glRotated(angle.z,0,0,1);

    glPointSize(5.0);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBegin(GL_POINTS);
    glColor3d(1.0, 1.0, 0);
    
    // draw planets
    loopv(i, planets)
    {
        ref = !planets.inrange(reference) ? planets[0]->pos : planets[reference]->pos;
        glColor3f(float(planets[i]->r)/255.0f, float(planets[i]->g)/255.0f, float(planets[i]->b)/255.0f);
        glVertex3dv((planets[i]->pos-ref).v);
    }

    loopv(i, satellites)
    {
        ref = !planets.inrange(reference) ? planets[0]->pos : planets[reference]->pos;
        glColor3f(float(satellites[i]->r)/255.0f, float(satellites[i]->g)/255.0f, float(satellites[i]->b)/255.0f);
        glVertex3d(satellites[i]->pos.x-ref.x, satellites[i]->pos.y-ref.y, satellites[i]->pos.z-ref.z);
        glVertex3d(satellites[i]->goal_p.x+satellites[i]->pl->pos.x-ref.x, satellites[i]->goal_p.y+satellites[i]->pl->pos.y-ref.y, satellites[i]->pos.z+satellites[i]->pl->pos.z-ref.z);
    }
    glEnd();

    // draw acceleration vectors

    if(draw_accel)
    {
        loopv(i, planets) draw_accel_vec((Body *)planets[i]);
        loopv(i, satellites)
        {
            draw_accel_vec((Body *)satellites[i]);
            draw_push_vec(satellites[i]);
        }
    }

    // trace history (planets) 
    // TODO : merge planets' and satellite' history rendering code
    ref = vec(0,0,0);
    loopv(i, planets)
    {
        if(planets[i] == pl_ref || !planets[i]->showhist) continue;

        OrbitDatas &orbit = planets[i]->orbits[planets.find(pl_ref)];
        glColor3f(float(planets[i]->r)/255.0f, float(planets[i]->g)/255.0f, float(planets[i]->b)/255.0f);
        
        glBegin(GL_LINE_STRIP);
        ref = !planets.inrange(reference) ? planets[0]->pos : planets[reference]->pos;

        int s = (pl_ref ? min(pl_ref->hist.size(), planets[i]->hist.size()) : planets[i]->hist.size())-1;
        vec lastpos = planets[i]->pos;
        if(s > 0) looprev(j, s)
        {
            if(pl_ref == planets[0] && skip_hists && planets[i]->histshift && (s-j) > orbit.period/seconds_per_dot) break;
            vec p, r;
            p = *planets[i]->hist.at(j);
            r = *pl_ref->hist.at(j);

            glVertex3dv((p-r).v);
        }
        glEnd();
    }

    loopv(i, satellites)
    {
        glBegin(GL_LINE_STRIP);
        ref = !planets.inrange(reference) ? planets[0]->pos : planets[reference]->pos;
        glVertex3dv((satellites[i]->pos-ref).v);
        glColor3f(float(satellites[i]->r)/255.0f, float(satellites[i]->g)/255.0f, float(satellites[i]->b)/255.0f);
        int s = (pl_ref ? min(pl_ref->hist.size(), satellites[i]->hist.size()) : satellites[i]->hist.size())-1;
        ref = !planets.inrange(reference) ? planets[0]->pos : planets[reference]->pos;
        if(s > 0) looprev(j, s) glVertex3dv((*satellites[i]->hist.at(j)-*pl_ref->hist.at(j)).v);
        glEnd();
    }

    // area covered by day
    if(draw_area)
    {
        glColor3ub(255, 0, 0);
        glBegin(GL_TRIANGLES);
            glVertex3d(0, 0, 0);
            glVertex3dv((curplanet->pos-pl_ref->pos).v);
            //glVertex3d(curplanet->pos.x-(curplanet->v.x-pl_ref->v.x)*86400-pl_ref->pos.x, curplanet->pos.y-(curplanet->v.y-pl_ref->v.y)*86400-pl_ref->pos.y, curplanet->pos.z-(curplanet->v.z-pl_ref->v.z)*86400-pl_ref->pos.z);
            glVertex3dv((curplanet->pos-(curplanet->v-pl_ref->v)*86400-pl_ref->pos).v);
        glEnd();
    }

    glPopMatrix();
}

void draw_text(const char *text, double x, double y, double z, int r, int g, int b, int font)
{
    if(print_frame)
    {
        r = g = b = 0;
    }
    glPushMatrix();

    glTranslated(x, y, z);
    glColor3ub(r, g, b);

    GPFont *ttf_font = fonts.inrange(font) ? fonts[font] : fonts.last();
    if(!ttf_font) return;
    ttf_font->Print2D(text);
    glPopMatrix();
}

void draw_text_recursive(const char *text, double x, double y, double z, int r, int g, int b, int font)
{
    string tmp = "";
    bool in_exp = false;
    int dx = 0;
    while(*text != '\0')
    {
        if(!in_exp)
        {
            if(*text != '^') tmp += *text;
            else
            {
                draw_text(tmp.c_str(), x+dx, y, z, r, g, b, font);
                dx += 9 * tmp.length();
                in_exp = true;
                tmp = "";
            }
        }
        else
        {
            if(isdigit(*text) || *text == '-') tmp += *text;
            else
            {
                draw_text(tmp.c_str(), x+dx, y+4, z, r, g, b, font);
                dx += 9 * tmp.length();
                in_exp = false;
                tmp = *text;
            }
        }

        ++text;
    }
    if(tmp.length()) draw_text(tmp.c_str(), x+dx, y+(in_exp ? 4 : 0), z, r, g, b, font);
}


void draw_misc()
{
    // fps
    char tmp[32] = "";
    float cps = abs(1000.0f/float(compute_millis.diff));
    sprintf(tmp, "%03.1f (%03.1f) fps, %.1lf pts/j", abs(1000.0f/float(render_millis.diff)), max(0, cps), 86400.0/seconds_per_dot);
    draw_text(tmp, w-10-strlen(tmp)*9, 25, 0);

    // date
    tm *time;
    time_t t = (time_t)simul.timestamp;
    time = gmtime(&t);

    sprintf(tmp, "%02d/%02d/%d %02d:%02d:%02d", time->tm_mday, time->tm_mon+1, time->tm_year+1900+simul.centuries5*500, time->tm_hour+1, time->tm_min, time->tm_sec);
    draw_text(tmp, w-10-strlen(tmp)*9, 75, 0);

    sprintf(tmp, "%.2lf j/s (dt %.1lf s)", speed, precision);
    draw_text(tmp, w-10-strlen(tmp)*9, 50, 0);

    // referential 
    Planet *pl_ref = planets.inrange(reference) ? planets[reference] : NULL;
    sprintf(tmp, "Referentiel: %s", pl_ref ? pl_ref->name.c_str() : "Soleil");
    draw_text(tmp, 10, h-20, 0);

    // copyright
    //draw_text("L. GAUTHERON (c) 2012", 10, 25, 0);
    draw_text("L.G.", 10, 25, 0);
}


void draw_planet_state(Planet *pl)
{
    if(!pl) return;

    Planet *pl_ref = planets.inrange(reference) ? planets[reference] : planets[0];
    OrbitDatas &orbit = pl->orbits[planets.find(pl_ref)];

    int y = h-20;
    char tmp[128] = "";

    draw_text(pl->name.c_str(), w-10-pl->name.length()*9, y, 0, pl->r, pl->g, pl->b);
    y -= 22;

    sprintf(tmp, "Masse: %.3e kg", pl->mass);
    draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
    y -= 22;

    if(pl_ref == pl) return;

    sprintf(tmp, "Distance: %.5e km", (pl->pos-pl_ref->pos).magnitude()/1000.0);
    draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
    y -= 22;

    sprintf(tmp, "Vitesse: %.2lf km/s", (pl->v-pl_ref->v).magnitude()/1000.0);
    draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
    y -= 22;

    char pattern[64] = "";
    double a = (pl->a-pl_ref->a).magnitude();
    int n = 1;
    if(a > 0) n = -int(log10(a));
    sprintf(pattern, "Accel: %%.%dlf mm/s²", n);
    sprintf(tmp, pattern, a*1000.0);
    draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
    y -= 22;
    
    sprintf(tmp, "Aph: %.5e km%s", orbit.aphelion/1000.0, orbit.hasperiod ? "" : " (?)");
    draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
    y -= 22;

    sprintf(tmp, "Peri: %.5e km%s", orbit.perihelion/1000.0, orbit.hasperiod ? "" : " (?)");
    draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
    y -= 22;

    double exc = (orbit.aphelion - orbit.perihelion)/(orbit.aphelion + orbit.perihelion);
    sprintf(tmp, "Exc: %.4lf%s", exc, orbit.hasperiod ? "" : " (?)");
    draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
    y -= 22;

    int years = 0;
    double days = 0;

    if(!draw_area)
    {
        years = orbit.period > 365 * 86400 * 1.1 ? int(orbit.period/(365.25*86400)) : 0;
        days = (orbit.period-365.25*86400*years)/86400.0;
        sprintf(pattern, orbit.hasperiod ? "Per: %%d an %s j" : "Per: ?", (days>10 ? "%.0lf" : "%.1lf"));
        sprintf(tmp, pattern, years, days);
        draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
        y -= 22;

        double angle = RAD2DEG*calc_angle(pl);
        angle = min(angle, 180-angle);
        sprintf(tmp, "Angle: %.2lf °", angle);
        draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
        y -= 22;
    }
    else
    {
        vec cinetic_momentum = ((pl->v-pl_ref->v) * (pl->pos-pl_ref->pos)) * pl->mass;
        double orbit_a = (orbit.aphelion + orbit.perihelion)/2;
        double average_r = orbit_a*sqrt(sqrt(1-exc*exc));
        double period = 2*D_PI/(cinetic_momentum.magnitude()/(average_r*average_r*pl->mass));
        double area = calc_area(pl);

        sprintf(tmp, "Aire: %.3e km²/j", area*0.0864);
        draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
        y -= 22;

        sprintf(tmp, "Moment cin.: %.3e SI", cinetic_momentum.magnitude());
        draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
        y -= 22;

        years = period > 365 * 86400 * 1.1 ? int(period/(365.25*86400)) : 0;
        days = (period-365.25*86400*years)/86400.0;
        sprintf(pattern, "Per(th): %%d an %s j%s", (days>10 ? "%.0lf" : "%.1lf"), orbit.hasperiod ? "" : "(?)");
        sprintf(tmp, pattern, years, days);
        draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
        y -= 22;

        sprintf(tmp, "Dist. moy: %.3e km", average_r/1000);
        draw_text(tmp, w-10-strlen(tmp)*9, y, 0);
    }

    if(current_syzygy.from && current_syzygy.to && current_syzygy.obstacle) // possible to render
    {
        if(!syzygy_rendermillis) syzygy_rendermillis = render_millis.current;
        if(render_millis.current-syzygy_rendermillis<1500) // syzygy hasn't timed out
        {
            sprintf(tmp, "Passage de %s entre %s et %s", current_syzygy.obstacle->name.c_str(), current_syzygy.from->name.c_str(), current_syzygy.to->name.c_str());
            draw_text(tmp, (w-strlen(tmp)*9)/2, h-20, 0);
        }
    }
}

void draw_label(string text, int x, int y)
{
    int width = text.length() * 9, height = 25;

    draw_text(text.c_str(), (w-width)/2, 10, 0, 255, 255, 255);
}

void draw_buttons()
{
    glPushMatrix();

    int transx = (w-buttons_width)/2;
    glTranslatef((float)transx, 25, 0);

    bool hashover = false;

    loop(i, B_NUM)
    {
        const double tex_w = 256, tex_h = 256;
        Button button = buttons[i];
        if(!button.show) continue;

        double tex_x = double(button.tex_x)/tex_w, tex_y = double(button.tex_y)/tex_h,
            tex_xs = double(button.width)/tex_w+tex_x, tex_ys = double(button.height)/tex_h+tex_y;

        int x = transx+buttons[i].x, y = 25+buttons[i].y, xs = x+buttons[i].width, ys = y+buttons[i].height;
                if(mouse_x >= x && mouse_x <= xs
            && mouse_y >= y && mouse_y <= ys)
        {
            buttons[i].hover = true;
        } else buttons[i].hover = false;


        if(!button.available) glColor3ub(80, 80, 80);
        else if(button.pressed) glColor3ub(255, 100, 100);
        else if(button.hover && !hashover) { glColor3ub(210, 210, 0); hashover = true; }
        else glColor3ub(255, 255, 255);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, buttons_tex);

        glBegin(GL_QUADS);
            glTexCoord2d(tex_x, tex_y);
            glVertex2i(button.x, button.y);

            glTexCoord2d(tex_xs, tex_y);
            glVertex2i(button.x+button.width, button.y);

            glTexCoord2d(tex_xs, tex_ys);
            glVertex2i(button.x+button.width, button.y+button.height);

            glTexCoord2d(tex_x, tex_ys);
            glVertex2i(button.x, button.y+button.height);
        glEnd();

        glDisable(GL_TEXTURE_2D);

        glBegin(GL_LINE_STRIP);
            glVertex2i(button.x, button.y);
            glVertex2i(button.x+button.width, button.y);
            glVertex2i(button.x+button.width, button.y+button.height);
            glVertex2i(button.x, button.y+button.height);
            glVertex2i(button.x, button.y);
        glEnd();
    }

    glPopMatrix();

    loop(i, B_NUM) if(buttons[i].hover) { draw_label(buttons[i].label, buttons[i].x, buttons[i].y); break; }
}