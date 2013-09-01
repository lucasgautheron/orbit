#include "project.h"

int reference = 0;
double scalefactor = 1e12;
bool running = false;
double precision = 50.0, speed = 20.0, seconds_per_dot = 86400.0;

Planet *curplanet = NULL;
Syzygy current_syzygy;

date simul;
static tm reftime;

void updatetimestamp(date &d, double add)
{
    d.timestamp += add;
    tm *time;
    time_t t = (time_t)d.timestamp;
    time = gmtime(&t);
    if(time->tm_year >= reftime.tm_year+500)
    {
        d.timestamp -= 500 * 365.25 * 86400;
        d.centuries5++;
    }
}

// Planet

Planet::Planet() : histshift(false), showhist(true), in_syzygy(false), ignore_gravity_impact(false), approaching(0), curtime(0), angle(0), area(0)
{
    histupdate = 0;
    type = BODY_PL;
    mass = radius = 0.0;
    r = g = b = 0;
    name = "";
    loop(i, 3) pos.v[i] = v.v[i] = a.v[i] = refpos.v[i] = refposcenter.v[i] = curangle.v[i] = curref.v[i] = 0.0;
}

void Planet::resethist()
{   
    refpos = hist.size() ? *hist.back() : refpos;
    refposdate = simul;
    refposcenter = planets[0]->pos;
    histupdate = 0.0;
    approaching = 0;
    histshift = false;
    
    loopvector(i, hist) DELETEP(hist[i]);

    hist.clear();
}

Planet::~Planet()
{
    resethist();
    syzygies.deletecontents();
    syzygies.shrink(0);
}

Satellite::Satellite() : free_flight(0)
{
    orbiting = false;
    type = BODY_SAT;
    state = SAT_ACCEL;
    histupdate = 0;
    mass = radius = 0.0;
    r = g = b = 0;
    name = "";
    pl = NULL;
    loop(i, 3) pos.v[i] = v.v[i] = a.v[i] = push.v[i] = 0.0;
}

void Satellite::resethist()
{   
    histupdate = 0.0;
    int s = hist.size();
    loop(i, s) { DELETEP(hist[i]); }
    hist.clear();
}

Satellite::~Satellite()
{
    resethist();
}

svector<Planet *> planets;
svector<Satellite *> satellites;

bool load_planets(const char *filename)
{
    planets.deletecontents();
    planets.shrink(0);
    FILE *fp = fopen(filename, "r");
    if(fp == NULL)
    {
        dbgoutf("impossible de lire le fichier \"%s\"", filename);
        return false;
    }

    // read header
    char hdrbuf[128] = "";
    if(fgets(hdrbuf, sizeof(hdrbuf), fp) == NULL) { return false; }
    tm time;
    time.tm_isdst = false;
    if(!sscanf(hdrbuf, "# %d/%d/%d %d:%d:%d\n", &time.tm_mday, &time.tm_mon, &time.tm_year, &time.tm_hour, &time.tm_min, &time.tm_sec))
    {
        dbgoutf("impossible de lire la date d'en tete");
        return false;
    }
    time.tm_year -= 1900;
    time.tm_mon -= 1;
    reftime = time;
    simul.timestamp = (double)mktime(&time);
    
    // file format : 1 planet / line :
    // name mass pos_x pos_y pos_z v_x v_y v_z

    for(;;)
    {
        char name[256] = "", buf[512] = "";
        double mass, radius;
        char px[32] = "", py[32] = "", pz[32] = "", vx[32] = "", vy[32] = "", vz[32] = "";
        short int r = 0, g = 0, b = 0, ignore_gravity = 0;
        if(fgets(buf, sizeof(buf), fp) == NULL) break;
        if(sscanf(buf, "%s %lf %lf %s %s %s %s %s %s %d %d %d %d\n", name, &mass, &radius, px, py, pz, vx, vy, vz, &r, &g, &b, &ignore_gravity))
        {
            Planet *curplanet = new Planet();
            curplanet->name.assign(name);
            curplanet->mass = mass;
            curplanet->radius = radius*UA;
            curplanet->pos.x = strtod(px, NULL)*UA; curplanet->pos.y = strtod(py, NULL)*UA; curplanet->pos.z = strtod(pz, NULL)*UA;
            curplanet->v.x = strtod(vx, NULL)*UA/DAY; curplanet->v.y = strtod(vy, NULL)*UA/DAY; curplanet->v.z = strtod(vz, NULL)*UA/DAY;
            curplanet->a = vec(0, 0, 0);
            curplanet->r = r; curplanet->g = g; curplanet->b = b;
            curplanet->refpos = curplanet->pos;
            curplanet->refposdate = simul;
            curplanet->ignore_gravity_impact = ignore_gravity ? true : false;
            dbgoutf("ajout planete \"%s\", dist soleil %.3lf UA, vitesse %.3lf km/s", name, curplanet->pos.magnitude()/UA, curplanet->v.magnitude()/1000, ignore_gravity);
            planets.add(curplanet);
        }
    }
    
    int count = planets.length();
    loopv(i, planets)
    {
        planets[i]->refposcenter = planets[0]->pos;
        loop(j, count)
        {
            OrbitDatas &orbit = planets[i]->orbits.add();
            orbit.aphelion = orbit.perihelion = planets[i]->pos.dist(planets[j]->pos);
            orbit.aph = orbit.per = planets[i]->pos;
            orbit.aph_ref = orbit.per_ref = planets[i]->pos;
        }
    }
    fclose(fp);
    return true;
}

void save_planets()
{
    tm *time;
    time_t t = (time_t)simul.timestamp;
    time = gmtime(&t);

    Planet *pl_ref = planets[0];

    char filename[64] = "";
    sprintf(filename, "../positions/%02d_%02d_%d_%02d.%02d.%02d.txt", time->tm_mday, time->tm_mon+1, time->tm_year+1900+simul.centuries5*500, time->tm_hour+1, time->tm_min, time->tm_sec);

    stream *f = openrawfile(filename, "w+");
    if(!f)
    {
        dbgoutf("impossible de creer le fichier \"%s\"", filename);
        return;
    }

    f->printf("# %02d/%02d/%d %02d:%02d:%02d\n", time->tm_mday, time->tm_mon+1, time->tm_year+1900+simul.centuries5*500, time->tm_hour+1, time->tm_min, time->tm_sec);

    loopv(i, planets)
    {
        Planet *pl = planets[i];
        f->printf("%s %.5e %.13lf %.13lf %.13lf %.13lf %.13lf %.13lf %d %d %d\n", pl->name.c_str(), pl->mass, (pl->pos.x-pl_ref->pos.x)/UA, (pl->pos.y-pl_ref->pos.y)/UA, (pl->pos.z-pl_ref->pos.z)/UA, (pl->v.x-pl_ref->v.x)/UA*DAY, (pl->v.y-pl_ref->v.y)/UA*DAY, (pl->v.z-pl_ref->v.z)/UA*DAY, pl->r, pl->g, pl->b);
    }

    f->close();

}

void save_datas()
{
    tm *time;
    time_t t = (time_t)simul.timestamp;
    time = gmtime(&t);

    Planet *pl_ref = !planets.inrange(reference) ? planets[0] : planets[reference];

    char filename[64] = "";
    sprintf(filename, "../positions/etat_%02d_%02d_%d_%02d.%02d.%02d.xml", time->tm_mday, time->tm_mon+1, time->tm_year+1900+simul.centuries5*500, time->tm_hour+1, time->tm_min, time->tm_sec);

    stream *f = openrawfile(filename, "w+");
    if(!f)
    {
        dbgoutf("impossible de creer le fichier \"%s\"", filename);
        return;
    }

    f->printf("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>");
    f->printf("<datas>");
    f->printf("\n    <date>%02d/%02d/%d %02d:%02d:%02d</date>", time->tm_mday, time->tm_mon+1, time->tm_year+1900+simul.centuries5*500, time->tm_hour+1, time->tm_min, time->tm_sec);
    f->printf("\n    <ref>");
    f->printf("\n        <name>%s</name>", pl_ref->name.c_str());
    f->printf("\n        <mass>%.8e</mass>", pl_ref->mass);
    f->printf("\n    </ref>");
    f->printf("\n    <planets>");
    
    loopv(i, planets)
    {
        Planet *pl = planets[i];
        if(pl == pl_ref) continue;

        OrbitDatas &orbit = pl->orbits[planets.find(pl_ref)];

        f->printf("\n        <planet>");
        f->printf("\n            <name>%s</name>", pl->name.c_str());
        f->printf("\n            <mass>%.8e</mass>", pl->mass);
        f->printf("\n            <period>%d</period>", int(orbit.period));
        f->printf("\n            <aph>%.12e</aph>", orbit.aphelion);
        f->printf("\n            <per>%.12e</per>", orbit.perihelion);

        f->printf("\n            <px>%.12e</px>", pl->pos.x-pl_ref->pos.x);
        f->printf("\n            <py>%.12e</py>", pl->pos.y-pl_ref->pos.y);
        f->printf("\n            <pz>%.12e</pz>", pl->pos.z-pl_ref->pos.z);

        f->printf("\n            <vx>%.12e</vx>", pl->v.x-pl_ref->v.x);
        f->printf("\n            <vy>%.12e</vy>", pl->v.y-pl_ref->v.y);
        f->printf("\n            <vz>%.12e</vz>", pl->v.z-pl_ref->v.z);

        f->printf("\n            <ax>%.12e</ax>", pl->a.x-pl_ref->a.x);
        f->printf("\n            <ay>%.12e</ay>", pl->a.y-pl_ref->a.y);
        f->printf("\n            <az>%.12e</az>", pl->a.z-pl_ref->a.z);

        f->printf("\n            <syzygies>");
        loopv(j, pl->syzygies) if(pl->syzygies[j]->from == pl)
        {
            tm *time;
            time_t t = (time_t)pl->syzygies[j]->beginning.timestamp;
            time = gmtime(&t);

            f->printf("\n                <syzygy>");
            f->printf("\n                    <dateFrom>%02d/%02d/%d %02d:%02d:%02d</dateFrom>", time->tm_mday, time->tm_mon+1, time->tm_year+1900+pl->syzygies[j]->beginning.centuries5*500, time->tm_hour+1, time->tm_min, time->tm_sec);

            t = (time_t)pl->syzygies[j]->end.timestamp;
            time = gmtime(&t);

            f->printf("\n                    <dateTo>%02d/%02d/%d %02d:%02d:%02d</dateTo>", time->tm_mday, time->tm_mon+1, time->tm_year+1900+pl->syzygies[j]->end.centuries5*500, time->tm_hour+1, time->tm_min, time->tm_sec);
            f->printf("\n                    <bgPlanet>%s</bgPlanet>", pl->syzygies[j]->to->name.c_str());
            f->printf("\n                    <obstacle>%s</obstacle>", pl->syzygies[j]->obstacle->name.c_str());
            f->printf("\n                    <bgDiam>%lf</bgDiam>", pl->syzygies[j]->to_diam*RAD2DEG*3600);
            f->printf("\n                    <obDiam>%lf</obDiam>", pl->syzygies[j]->ob_diam*RAD2DEG*3600);
            f->printf("\n                    <prec>%.5e</prec>", pl->syzygies[j]->prec);
            f->printf("\n                </syzygy>");
        }
        f->printf("\n            </syzygies>");

        f->printf("\n        </planet>");
    }

    f->printf("\n    </planets>");
    f->printf("\n</datas>");
    f->close();
}

vec gravaccel(Body *a, Body *b)
{
    vec dist = b->pos - a->pos;
    double distance = dist.magnitude();
    if(a->type == BODY_PL && a->type == BODY_PL && distance < (a->radius + b->radius))
    {
        dbgoutf("collision entre %s et %s !", a->name.c_str(), b->name.c_str());
        if(a->mass > b->mass)
        {
            a->v = (b->v * b->mass + a->v * a->mass) / (a->mass+b->mass);
            a->mass += b->mass;
            b->mass = 0;
            planets.remove(planets.find((Planet *)b));
        }
        else
        {
            b->v = (b->v * b->mass + a->v * a->mass) / (a->mass+b->mass);
            b->mass += a->mass;
            a->mass = 0;
            planets.remove(planets.find((Planet *)a));
        }
    }
    vec acc = dist;
    acc.normalize();
    acc.mul((G * b->mass) / (distance * distance));
    return acc;
}

void move_planets(int it)
{
    vec *ref = &planets[0]->pos;

    loop(n, it)
    {
        double period = precision;
        if(!running) break;
        updatetimestamp(simul, period);

        // apply gravity to all planets.
        loopv(i, planets)
        {
            vec acc(0.0, 0.0, 0.0);
            loopv(j, planets)
            {
                if(i == j || planets[j]->ignore_gravity_impact) continue;
                acc.add(gravaccel((Body *)planets[i], (Body *)planets[j]));
            }

            planets[i]->a = acc;
            planets[i]->v += planets[i]->a * period;
        }

        // apply gravity to all satellites. Satellites' own gravity effects are negliged.
        // then, "push" satellites to force them to the desired position

        // FIXME
        loopv(i, satellites)
        {
            Satellite *s = satellites[i];
            s->orbiting = true;
            vec acc(0, 0, 0);
            loopv(j, planets)
            {
                acc.add(gravaccel((Body *)s, (Body *)planets[j]));
            }

            vec dp = s->goal_p + s->pl->pos - s->pos; // FIXME : use a static value of s->pl->pos @ SAT_ACCEL
            vec v(0, 0, 0), a(0, 0, 0);

            switch(s->state)
            {
                case SAT_ACCEL:
                {
                    // v² - v0² = 2a(x-x0)
                    // x = (-v0²)/2a
                    double stop_dist = (s->v.squaredlen())/(2*(s->max_brake/s->mass));
                    s->stop_time = s->v.magnitude()/(s->max_brake/s->mass);
                    if(stop_dist >= dp.magnitude())
                    {
                        s->state = SAT_APPROACH;
                        break;
                    }
                    v = dp; v.normalize().mul(s->max_vel);
                    a = v-s->v; a.div(period);
                    s->push = a-acc;
                    double force = s->push.magnitude()*s->mass;
                    force = min(force, s->max_push);
                    s->push.normalize().mul(force/s->mass);
                    s->a = s->push + acc;
                }
                break;

                case SAT_APPROACH:
                {
                    if(s->stop_time <= 0)
                    {
                        s->state = SAT_ORBIT;
                        break;
                    }
                    s->stop_time -= period;
                    v = dp;
                    a = v-s->v; a.div(period);
                    s->push = -a-acc;
                    s->push.normalize().mul(s->max_brake/s->mass);
                    s->a = s->push + acc;
                }
                break;

                case SAT_ORBIT:
                {
                    // TODO : wait for planet to arrive to the desired position, using a static ref to pl->pos ?!
                    v = dp; v.normalize().mul(s->goal_vel);
                    a = v-s->v; a.div(period);
                    s->push = a-acc;
                    double force = s->push.magnitude()*s->mass;
                    force = min(force, s->max_push);
                    s->push.normalize().mul(force/s->mass);
                    s->a = s->push + acc;

                    s->goal_p.rotate(2*D_PI/(s->period)*period, s->axis);
                }
                break;

            }

            // apply accel
            s->v += s->a * period;
            s->pos += s->v * period;

            // history
            s->histupdate += precision;
            if(s->histupdate > seconds_per_dot)
            {   
                vec *v = new vec();
                *v = s->pos;
                s->histupdate = 0;
                s->hist.push_back(v);
            }
        }

        Planet *pl_ref = !planets.inrange(reference) ? planets[0] : planets[reference];
        vec trace_ref = pl_ref->pos;

        vec newrefpos = pl_ref->pos;
        newrefpos.x += pl_ref->v.x; newrefpos.y += pl_ref->v.y; newrefpos.z += pl_ref->v.z;

        loopv(i, planets)
        {
            vec newpos = planets[i]->pos + planets[i]->v * period;

            if(planets[i] != pl_ref)
            {
                vec curv = planets[i]->pos - pl_ref->pos;
                vec nextv = planets[i]->pos - newrefpos;

                double dotmag = 0, diff = 0;
                if(!planets[i]->curangle.iszero()) dotmag = planets[i]->curangle.dot(nextv)/(planets[i]->curangle.magnitude()*nextv.magnitude());
                else
                {
                    dotmag = curv.dot(nextv)/(curv.magnitude()*nextv.magnitude());
                    planets[i]->curangle = curv;
                }

                diff = dotmag < 1 ? acos(dotmag) : 0;
                if(diff > 0.000000001)
                {
                    planets[i]->approaching += abs(diff);
                    planets[i]->curangle = curv;
                }
            
                OrbitDatas &orbit = planets[i]->orbits[planets.find(pl_ref)];

                // FIXME
                if(planets[i]->approaching >= 2 * D_PI)
                {
                    // finished cycle, starting a new one
                    planets[i]->approaching = 0;
                    planets[i]->histshift = true;
                    orbit.period = 86400*365.25*500*(simul.centuries5-planets[i]->refposdate.centuries5)+simul.timestamp-planets[i]->refposdate.timestamp;
                    planets[i]->refposdate = simul;
                    planets[i]->refpos = newpos;
                    orbit.hasperiod = true;
                }
                
                double prev_d = (planets[i]->pos-pl_ref->pos).magnitude();
                double d = (newpos-pl_ref->pos).magnitude();
                
                planets[i]->pos = newpos;
                if(orbit.aphelion < d)
                {
                    orbit.aphelion = d;
                    orbit.aph = planets[i]->pos-pl_ref->pos;
                    if(!orbit.hasperiod) orbit.aph_ref = orbit.aph;
                }
                else if(orbit.perihelion > d)
                {
                    orbit.perihelion = d;
                    orbit.per = planets[i]->pos-pl_ref->pos;
                    if(!orbit.hasperiod) orbit.per_ref = orbit.per;
                }

                /*if(prev_d < d)
                {
                    if(orbit.approaching && orbit.hasperiod)
                    {
                        orbit.per = planets[i]->pos-pl_ref->pos;
                        orbit.prec = orbit.per.angle(orbit.per_ref);
                    }
                    orbit.approaching = false;
                }
                else
                {
                    if(!orbit.approaching)
                    {
                        orbit.aph = planets[i]->pos-pl_ref->pos;
                    }
                    orbit.approaching = true;
                }*/
            }
            else planets[i]->pos = newpos;

            // trace history
            planets[i]->histupdate += precision;

            if(planets[i]->histupdate > seconds_per_dot)
            {   
                vec *v = new vec();
                if(!v) { dbgoutf("fatal error : could not allocate memory"); callback_quit(); return; }
                *v = planets[i]->pos;
                planets[i]->histupdate = 0;
                planets[i]->hist.push_back(v);
            }
        }

        // Syzygy detection.
        Planet *from = curplanet, *to = !planets.inrange(reference) ? planets[0] : planets[reference], *obstacle;
        
        bool found_syzygy = false;
        loopv(i, planets)
        {
            obstacle = planets[i];
            if(obstacle == from || obstacle == to) continue;

            vec axis = to->pos - from->pos;
            vec axis_unit = axis; axis_unit.normalize();
            vec from_obstacle = obstacle->pos - from->pos;
            double dist_obstacle = ((from_obstacle*axis_unit).magnitude())/(axis_unit.magnitude());
            double dist_along_axis = sqrt(from_obstacle.squaredlen()-dist_obstacle*dist_obstacle);

            double min_radius = min(from->radius, to->radius);
            double max_radius = max(from->radius, to->radius);

            double h = min_radius+(dist_along_axis*(max_radius-min_radius)/axis.magnitude());

            if(dist_obstacle-obstacle->radius <= h && axis.magnitude() > from_obstacle.magnitude() && axis.magnitude() > (to->pos - obstacle->pos).magnitude())
            {
                
                found_syzygy = true;
                syzygy_rendermillis = 0;
                current_syzygy.to = to; current_syzygy.from = from; current_syzygy.obstacle = obstacle;

                if(!from->in_syzygy)
                {
                    current_syzygy.beginning = simul;
                }
                from->in_syzygy = true;
            }
        }
        if(!found_syzygy && from->in_syzygy)
        {
            current_syzygy.end = simul;
            Syzygy *syz = new Syzygy();
            *syz = current_syzygy;
            syz->prec = precision;
            syz->ob_diam = 2*atan(syz->obstacle->radius/syz->obstacle->pos.dist(syz->from->pos));
            syz->to_diam = 2*atan(syz->to->radius/syz->to->pos.dist(syz->from->pos));
            from->syzygies.add(syz);
            from->in_syzygy = false;
        }
    }
}

double calc_angle(Planet *pl)
{
    Planet *angleref = !planets.inrange(reference) ? planets[0] : planets[reference];
    if(pl == angleref) { return pl->angle = 0.0; }
    if(angleref->hist.size() < 7 || pl->hist.size() < 7) return 0;
    
    vec ref_n = (*angleref->hist.at(angleref->hist.size()-1) - *angleref->hist.at(angleref->hist.size()-3)) * (*angleref->hist.at(angleref->hist.size()-3) - *angleref->hist.at(angleref->hist.size()-5));
    vec n = (*pl->hist.at(pl->hist.size()-1) - *pl->hist.at(pl->hist.size()-3)) * (*pl->hist.at(pl->hist.size()-3) - *pl->hist.at(pl->hist.size()-5));

    return pl->angle = ref_n.angle(n);
}

double calc_area(Planet *pl)
{
    Planet *pl_ref = !planets.inrange(reference) ? planets[0] : planets[reference];
    vec cross = (pl->pos-pl_ref->pos) * (pl->v - pl_ref->v);
    return pl->area = cross.magnitude()/2; // area covered in one second.
}