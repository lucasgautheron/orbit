/*          compilation, code writing tools          */

#ifdef NULL
#undef NULL
#endif
#define NULL 0

// pointers
#define DELETEP(p) { if(p) delete p; p = NULL; }
#define DELETEA(a) { if(a) delete[] a; a = NULL; }

#define ASSERT(c) if(c) {}

//   custom types
typedef unsigned char uchar;
typedef std::string string;
typedef unsigned short ushort;
typedef unsigned int uint;
#ifndef int64_t
    typedef __int64 int64_t;
#endif
typedef int64_t msec_t;

// rand
inline int rnd(int min, int max) { return min + rand() % (max-min); }
inline int rnd(int max) { return rnd(0, max); }

// loop
#define loop(v,m) for(int v = 0; v<int(m); ++v)
#define looprev(v, m) for(int v = m-1; v>0; --v)
#define loopv(v, ve) loop(v, (ve).size())
#define loopvector(v, ve) loop(v, (ve).size())
#define invecrange(ve, i) ((i) >= 0 && (i) < (ve).size())
/*#define loopv(ve) loop(i, (ve).length())
#define loopvj(ve) loop(j, (ve).length())
#define loopvk(ve) loop(k, (ve).length())
#define loopvrev(v) looprev(i, (ve).length())
#define loopvjrev(v) looprev(j, (ve).length())*/

#define MAXSTRLEN 5000
#define defvformatstring(d,last,fmt) char d[MAXSTRLEN]; { va_list ap; va_start(ap, last); _vsnprintf_s(d, MAXSTRLEN, fmt, ap); d[MAXSTRLEN-1] = 0; va_end(ap); }

// output
void dbgoutf(const char *format, ...);

// time
msec_t time_ms(void);

// file
float human_readable_size(int bytes, char &mul);
std::string strtoupper(std::string &from);

// strings
inline char *copystring(char *d, const char *s, size_t len = MAXSTRLEN) { strncpy(d, s, len); d[len-1] = 0; return d; }
inline char *newstring(size_t l)                { return new char[l+1]; }
inline char *newstring(const char *s, size_t l) { return copystring(newstring(l), s, l+1); }
inline char *newstring(const char *s)           { return newstring(s, strlen(s)); }
inline char *newstringbuf()                     { return newstring(MAXSTRLEN-1); }
inline char *newstringbuf(const char *s)        { return newstring(s, MAXSTRLEN-1); }

template <class T>
struct databuf
{
    enum
    {
        OVERREAD  = 1<<0,
        OVERWROTE = 1<<1
    };

    T *buf;
    int len, maxlen;
    uchar flags;

    databuf() : buf(NULL), len(0), maxlen(0), flags(0) {}

    template <class U>
    databuf(T *buf, U maxlen) : buf(buf), len(0), maxlen((int)maxlen), flags(0) {}

    const T &get()
    {
        static T overreadval = 0;
        if(len<maxlen) return buf[len++];
        flags |= OVERREAD;
        return overreadval;
    }

    databuf subbuf(int sz)
    {
        sz = min(sz, maxlen-len);
        len += sz;
        return databuf(&buf[len-sz], sz);
    }

    void put(const T &val)
    {
        if(len<maxlen) buf[len++] = val;
        else flags |= OVERWROTE;
    }

    void put(const T *vals, int numvals)
    {
        if(maxlen-len<numvals) flags |= OVERWROTE;
        memcpy(&buf[len], vals, min(maxlen-len, numvals)*sizeof(T));
        len += min(maxlen-len, numvals);
    }

    int get(T *vals, int numvals)
    {
        int read = min(maxlen-len, numvals);
        if(read<numvals) flags |= OVERREAD;
        memcpy(vals, &buf[len], read*sizeof(T));
        len += read;
        return read;
    }

    int length() const { return len; }
    int remaining() const { return maxlen-len; }
    bool overread() const { return (flags&OVERREAD)!=0; }
    bool overwrote() const { return (flags&OVERWROTE)!=0; }

    void forceoverread()
    {
        len = maxlen;
        flags |= OVERREAD;
    }
};