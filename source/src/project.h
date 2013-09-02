#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <stdarg.h>
#include <math.h>

#if defined(WIN32)
    #include <ppl.h>
    #include <concurrent_vector.h>
    #include <windows.h>
    #include <Mmsystem.h>
    #include <time.h>
#else
    #include <sys/time.h>
#endif

#include <gl/gl.h>
#include <gl/glu.h>

#include <SDL/SDL.h>
#include "SDL/SDL_ttf.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_thread.h"

#include "zlib/zlib.h"

#include "tools.h"
#include "stream.h"
#include "callbacks.h"
#include "maths.h"
#include "engine.h"
#include "render.h"
