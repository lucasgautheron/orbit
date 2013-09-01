// consts
#define D_PI (double(3.141592653589793238462643))
#define D_E (double(2.718281828459045235360287))
#define G (double(6.67300e-11))
#define UA (double(1.49597871e11))
#define DAY (double(86400))
#define C (double(299792458))
#define MS (double(1.9891e30))

#define DEG2RAD (D_PI/180.0)
#define RAD2DEG (1/DEG2RAD)

// units 
template <typename T>
inline T apply_unit_prefix(T val, char* unit)
{
    switch(*unit)
    {
        case 'G': return val * 1000000000;
        case 'M': return val * 1000000;
        case 'k': return val * 1000;
        case 'h': return val * 100;
        case 'd': return *(unit+1) == 'a' ? val * 10 : val / 10;
        case 'c': return val / 100;
        case 'm': return val / 1000;
        case 'µ': return val / 1000000;
        case 'n': return val / 1000000000;
        case 'p': return val / 1000000000000;
        case 'f': return val / 1000000000000000;
        default: return val;
    }
}

// 3-dim vector
class vec
{
public:
    union
    {
        struct { double x, y, z; };
        double v[3];
        int i[3];
    };

    vec() {x=y=z=0;}
    vec(double a, double b, double c) : x(a), y(b), z(c) {}
    vec(double *v) : x(v[0]), y(v[1]), z(v[2]) {}

    bool iszero() const;

    vec &mul(double f);
    vec &div(double f);
    vec &add(double f);
    vec &sub(double f);

    vec &add(const vec &o);
    vec &sub(const vec &o);

    double squaredlen() const;
    double sqrxy() const;

    double dot(const vec &o) const;
    double dotxy(const vec &o) const;

    double magnitude() const;
    vec &normalize();

    double distxy(const vec &e) const;
    double magnitudexy() const;

    double dist(const vec &e) const;
    double dist(const vec &e, vec &t) const;

    double angle(const vec &e) const;

    bool reject(const vec &o, double max) const;

    vec &cross(const vec &a, const vec &b);
    double cxy(const vec &a);

    void rotate_around_z(double angle);
    void rotate_around_x(double angle);
    void rotate_around_y(double angle);

    vec &rotate(double angle, const vec &d);
    vec &rotate(double c, double s, const vec &d);
    vec &orthogonal(const vec &d);

    double &operator[](int i)       { return v[i]; }
    double  operator[](int i) const { return v[i]; }

    vec  operator+ (vec v) { vec ret(*this); return ret.add(v); }
    vec& operator+=(vec v) { return this->add(v); }
    vec  operator- (vec v) { vec ret(*this); return ret.sub(v); }
    vec& operator-=(vec v) { return this->sub(v); }

    vec  operator* (double f) const { vec ret(*this); return ret.mul(f); }
    vec& operator*=(double f)       { return this->mul(f); }
    vec  operator/ (double f) const { vec ret(*this); return ret.div(f); }
    vec& operator/=(double f)       { return this->div(f); }

    vec operator* (vec v) const { vec ret(0,0,0); return ret.cross(*this, v); }

    bool operator==(const vec &o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator!=(const vec &o) const { return x != o.x || y != o.y || z != o.z; }
    bool operator! ()             const { return iszero(); }
    vec operator-() const { return vec(-x, -y, -z); }
};

// 4 - dim vector
class vec4 : public vec
{
};