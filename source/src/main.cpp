// main.cpp
// program start / quit
#include "project.h"

void init();
void close();

bool keyreleased = true;
bool mousedown = false;
bool print_frame = false;

int mouse_x = 0, mouse_y = 0;

SDL_Thread *compute_thread = NULL;
threadmillis render_millis, compute_millis;

char *planets_file = NULL;

void limitfps(threadmillis tm)
{
    if(!tm.maxfps) return;
    int delay = 1000/tm.maxfps - (tm.diff);
    if(delay < 0) tm.fpserror = 0;
    else
    {
        tm.fpserror += 1000%tm.maxfps;
        if(tm.fpserror >= tm.maxfps)
        {
            ++delay;
            tm.fpserror -= tm.maxfps;
        }
        if(delay > 0)
        {
            SDL_Delay(delay);
        }
    }
}

void compute(int diff)
{
    double timetocompute = speed * 86400.0 * double(diff)/1000.0;
    int n = int(timetocompute/precision);
    move_planets(n);
}

int compute_thread_func(void *data)
{
    for(;;)
    {
        compute_millis.last = compute_millis.current;
        compute_millis.current = SDL_GetTicks();
        compute_millis.diff = compute_millis.current-compute_millis.last;

        compute(compute_millis.diff);
        limitfps(compute_millis);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    init();
    dbgoutf("Initialisation terminee! Chargement des planetes...");

    bool planets_loaded = false;
    if(argc > 1 && argv[1] && *argv[1] != '\0')
    {
        char *filename = argv[1];
        planets_file = newstring(argv[1]);
        dbgoutf("Lecture du fichier \"%s\"", filename);
        planets_loaded = load_planets(filename);
    }
    if(!planets_loaded)
    {
        planets_loaded = load_planets(DEFAULT_PLANETS_FILE);
        if(!planets_loaded)
        {
            dbgoutf("Impossible de charger les planetes. Fermeture du programme.");
            callback_quit();
            return EXIT_FAILURE;
        }
    }

    compute_thread = SDL_CreateThread(compute_thread_func, NULL);

    SDL_Event event;

    if(invecrange(planets, 0)) curplanet = planets[0];

    render_millis.maxfps = MAXFPS;
    compute_millis.maxfps = MAXCPS;

    // main loop

    for(;;)
    {
        render_millis.last = render_millis.current; 
        render_millis.current = SDL_GetTicks();
        render_millis.diff = render_millis.current-render_millis.last;

        SDL_PollEvent(&event);

        bool pressed = false;
        switch(event.type)
        {
            case SDL_KEYDOWN:
            {
                pressed = true; // a key has been pressed
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE: callback_quit(); break;
                    case SDLK_s: start_pause(1); break;
                    case SDLK_p: start_pause(0); break;

                    case SDLK_7:
                    case SDLK_KP7: if(keyreleased) precision /= 1.1; break;

                    case SDLK_8:
                    case SDLK_KP8: if(keyreleased) precision *= 1.1; break;

                    case SDLK_F1: if(keyreleased) smoother(); break;

                    case SDLK_F2: if(keyreleased) harder(); break;

                    case SDLK_o: { if(keyreleased) save_planets(); } break;
                    case SDLK_F4: { if(keyreleased) save_datas(); } break;

                    //case SDLK_F11: { if((keyreleased || 1) && !print_frame) print_frame = true; } break;
                    case SDLK_F12: { char buf[64] = ""; sprintf(buf, "../images/screenshot_%d.bmp", render_millis.current); takeScreenshot(buf); } break;

                    case SDLK_KP_PLUS: if(keyreleased) zoom_in(); break;
                    case SDLK_KP_MINUS: if(keyreleased) zoom_out(); break;

                    case SDLK_KP0: if(keyreleased) skip_hists = !skip_hists; break;

                    case SDLK_F3: if(keyreleased) draw_area = !draw_area; break;
                    
                    case SDLK_q:
                    case SDLK_a: if(keyreleased) draw_accel = !draw_accel; break;
                    
                    case SDLK_f: SDL_WM_ToggleFullScreen(screen); break;

                    case SDLK_4:
                    case SDLK_KP4:
                        if(keyreleased) increase_speed();
                    break;

                    case SDLK_5:
                    case SDLK_KP5:
                        if(keyreleased) lower_speed(); break;
                    
                    case SDLK_RETURN: if(keyreleased) studied(); break;

                    case SDLK_t: if(keyreleased) curplanet->showhist = !curplanet->showhist; break;

                    case SDLK_1:
                    case SDLK_KP1:
                        if(keyreleased) prev_ref();
                    break;

                    case SDLK_2:
                    case SDLK_KP2:
                        if(keyreleased) next_ref();
                    break;
                    case SDLK_RIGHT:
                    {
                        angle.y -= 20.0*double(render_millis.diff)/1000.0;
                        angle.y = angle.y > 360 ? int(angle.y) % 360 : angle.y;
                    }
                    break;
                    case SDLK_LEFT:
                    {
                        angle.y += 20.0*double(render_millis.diff)/1000.0;
                        angle.y = angle.y > 360 ? int(angle.y) % 360 : angle.y;
                    }
                    case SDLK_UP:
                    {
                        angle.z += 20.0*double(render_millis.diff)/1000.0;
                        angle.z = angle.z > 360 ? int(angle.z) % 360 : angle.z;
                    }
                    break;
                    case SDLK_DOWN:
                    {
                        angle.z -= 20.0*double(render_millis.diff)/1000.0;
                        angle.z = angle.z > 360 ? int(angle.z) % 360 : angle.z;
                    }
                    default:
                    {
                        
                    }
                }
            }
            break;
            case SDL_QUIT: callback_quit(); break;
            case SDL_VIDEORESIZE:
            {
                w = event.resize.w;
                h = event.resize.h;
                glViewport(0, 0, w, h);
                start_pause(0);
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                if(!mousedown && event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT) mouse_click(event.button.button, event.button.x, h-event.button.y, true);
                if(event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT) mousedown = true;
            }
            break;

            case SDL_MOUSEBUTTONUP:
            {
                if(event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT && mousedown)
                {
                    mousedown = false;
                    mouse_click(event.button.button, event.button.x, h-event.button.y, false);
                }
            }
            break;

            case SDL_MOUSEMOTION:
            {
                mouse_x = event.motion.x;
                mouse_y = h-event.motion.y;
            }
            break;
            
        }
        
        if(pressed && keyreleased) keyreleased = false;
        else if(!pressed && !keyreleased) keyreleased = true;
        limitfps(render_millis);
        
        render();
    }

    

    system("pause");
    callback_quit();
    return EXIT_SUCCESS;
}

void init()
{
    srand ( (unsigned int)time(NULL) );
    dbgoutf("Initialisation en cours...");
    init_sdl();
    init_buttons();
}

void callback_quit()
{
    TTF_Quit();
    glDeleteTextures( 1, &buttons_tex );
    SDL_KillThread(compute_thread);
    SDL_Quit();
    exit(EXIT_SUCCESS);
}

void callback_none()
{
    dbgoutf("no action specified");
}