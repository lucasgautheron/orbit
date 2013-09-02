// max values by output file
#define OUTPUT_MAX_VAL 1000
#define OUTPUT_CSV_SEP ";"

#define MAXFPS 40
#define MAXCPS 500

#define DEFAULT_PLANETS_FILE "planets.txt"

struct date
{
    int centuries5;
    double timestamp;

    date() : centuries5(0), timestamp(0.0) {}
};

extern date simul;

struct OrbitDatas
{
    double aphelion, perihelion, period;
    bool hasperiod, approaching;
    vec aph, per;
    vec aph_ref, per_ref;
    double prec;
    OrbitDatas() : aphelion(0), perihelion(0), period(0), hasperiod(false), approaching(false), prec(0.0) { loop(i, 3) aph.v[i] = per.v[i] = aph_ref.v[i] = per_ref.v[i] = 0; }
};

enum { BODY_PL = 0, BODY_NUM };

class Body
{
     public: 
        string name;
        vec pos, v, a, push;
        double mass; // kg
        double radius, histupdate;
        short int r, g, b;
        int type;
    
     Concurrency::concurrent_vector<vec *> hist;
     //std::vector<vec *> hist;
     //svector<vec *> hist;
     //Concurrency::concurrent_vector<vec *> tmp_hist;
};

struct Syzygy;

class Planet : public Body
{
    public:
        vec refpos, refposcenter; // position, first position, velocity, acceleration
        bool histshift, showhist, in_syzygy, ignore_gravity_impact;
        double approaching, curtime;
        vec curangle, curref;
        date refposdate;
        double angle, area;

        svector<OrbitDatas> orbits;
        svector<Syzygy *> syzygies;

    void resethist();

    Planet();
    ~Planet();
};

struct Syzygy
{
    Planet *from, *obstacle, *to;
    date beginning, end;
    double prec; // the step time (precision) used to compute the current syzygy has to be stored
    double ob_diam, to_diam;

    Syzygy() : from(NULL), obstacle(NULL), to(NULL), prec(0.0), ob_diam(0.0), to_diam(0.0) {}
};

extern Syzygy current_syzygy;

struct threadmillis
{
    int last, current, diff, fpserror, maxfps;
    threadmillis() : last(0), current(0), diff(0), fpserror(0), maxfps(100) {}
};

extern threadmillis render_millis, compute_millis;
extern int syzygy_rendermillis;

bool load_planets(const char *filename);
void save_planets();
void save_datas();
void clear_hist();
void move_planets(int it);
double calc_angle(Planet *pl);
double calc_area(Planet *pl);

extern threadmillis render_millis, compute_millis;
extern int lasthistclear;
extern int reference;
extern double scalefactor;
extern bool running, print_frame;
extern char *planets_file;
extern Planet *curplanet;
extern svector<Planet *> planets;
extern double precision, speed, seconds_per_dot;
extern bool skip_hists, draw_accel, draw_area;

// i/o controls

struct Zone
{
    int x, y, xs, ys;
    
    Zone() : x(0), y(0), xs(0), ys(0) {}
    Zone(int x, int y, int xs, int ys) : x(x), y(y), xs(xs), ys(ys) {}
};

#define BUTTON_DEFAULT_WIDTH 32
#define BUTTON_DEFAULT_HEIGHT 32

enum { B_STARTPAUSE = 0, B_FASTER, B_SLOWER,
    B_ZOOMIN, B_ZOOMOUT,
    B_PREVREF, B_NEXTREF,
    B_MOREDOTS, B_LESSDOTS,
    B_STUDYNEXT,
    B_REWIND,
    B_NUM };

struct Button
{
    string name;
    string label;
    bool show, available;
    int tex_x, tex_y, width, height, x, y;
    void (*action)(void);
    bool pressed, hover;
};

void init_buttons();
void start_pause(int force);
void start_pause();
void increase_speed();
void lower_speed();
void zoom_in();
void zoom_out();
void prev_ref();
void next_ref();
void smoother();
void harder();
void studied();
void rewind();

extern Button buttons[B_NUM];
extern GLuint buttons_tex;
extern int buttons_width;

extern int mouse_x, mouse_y;
void mouse_click(short int mouse_button, int mx, int my, bool isdown);