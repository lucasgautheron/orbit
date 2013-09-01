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
#define loopv(v, ve) loop(v, (ve).length())
#define loopvector(v, ve) loop(v, (ve).size())
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

template <class T> struct svector
{
   static const int MINSIZE = 8;

    T *buf;
    int alen, ulen;

    svector() : buf(NULL), alen(0), ulen(0)
    {
    }

    svector(const svector &v) : buf(NULL), alen(0), ulen(0)
    {
        *this = v;
    }

    ~svector() { shrink(0); if(buf) delete[] (uchar *)buf; }

    svector<T> &operator=(const svector<T> &v)
    {
        shrink(0);
        if(v.length() > alen) growbuf(v.length());
        loopv(i, v) add(v[i]);
        return *this;
    }

    T &add(const T &x)
    {
        if(ulen==alen) growbuf(ulen+1);
        new (&buf[ulen]) T(x);
        return buf[ulen++];
    }

    T &add()
    {
        if(ulen==alen) growbuf(ulen+1);
        new (&buf[ulen]) T;
        return buf[ulen++];
    }

    T &dup()
    {
        if(ulen==alen) growbuf(ulen+1);
        new (&buf[ulen]) T(buf[ulen-1]);
        return buf[ulen++];
    }

    bool inrange(size_t i) const { return i<size_t(ulen); }
    bool inrange(int i) const { return i>=0 && i<ulen; }

    T &pop() { return buf[--ulen]; }
    T &last() { return buf[ulen-1]; }
    void drop() { buf[--ulen].~T(); }
    bool empty() const { return ulen==0; }

    int capacity() const { return alen; }
    int length() const { return ulen; }
    int size() const { return ulen; }
    T &at(int i) { ASSERT(i>=0 && i<ulen); return buf[i]; }
    T &operator[](int i) { ASSERT(i>=0 && i<ulen); return buf[i]; }
    const T &operator[](int i) const { ASSERT(i >= 0 && i<ulen); return buf[i]; }

    void shrink(int i)         { ASSERT(i<=ulen); while(ulen>i) drop(); }
    void setsize(int i) { ASSERT(i<=ulen); ulen = i; }

    void deletecontents() { while(!empty()) delete   pop(); }
    void deletearrays() { while(!empty()) delete[] pop(); }

    T *getbuf() { return buf; }
    const T *getbuf() const { return buf; }
    bool inbuf(const T *e) const { return e >= buf && e < &buf[ulen]; }

    template<class ST>
    void sort(int (__cdecl *cf)(ST *, ST *), int i = 0, int n = -1)
    {
        qsort(&buf[i], n<0 ? ulen : n, sizeof(T), (int (__cdecl *)(const void *,const void *))cf);
    }

    template<class ST>
    T *search(T *key, int (__cdecl *cf)(ST *, ST *), int i = 0, int n = -1)
    {
        return (T *) bsearch(key, &buf[i], n<0 ? ulen : n, sizeof(T), (int (__cdecl *)(const void *,const void *))cf);
    }

    void growbuf(int sz)
    {
        int olen = alen;
        if(!alen) alen = max(MINSIZE, sz);
        else while(alen < sz) alen *= 2;
        if(alen <= olen) return;
        uchar *newbuf = new uchar[alen*sizeof(T)];
        if(olen > 0)
        {
            memcpy(newbuf, buf, olen*sizeof(T));
            delete[] (uchar *)buf;
        }
        buf = (T *)newbuf;
    }

    databuf<T> reserve(int sz)
    {
        if(ulen+sz > alen) growbuf(ulen+sz);
        return databuf<T>(&buf[ulen], sz);
    }

    void advance(int sz)
    {
        ulen += sz;
    }

    void addbuf(const databuf<T> &p)
    {
        advance(p.length());
    }

    T *pad(int n)
    {
        T *buf = reserve(n).buf;
        advance(n);
        return buf;
    }

    void put(const T &v) { add(v); }

    void put(const T *v, int n)
    {
        databuf<T> buf = reserve(n);
        buf.put(v, n);
        addbuf(buf);
    }

    void remove(int i, int n)
    {
        for(int p = i+n; p<ulen; p++) buf[p-n] = buf[p];
        ulen -= n;
    }

    T remove(int i)
    {
        T e = buf[i];
        for(int p = i+1; p<ulen; p++) buf[p-1] = buf[p];
        ulen--;
        return e;
    }

    int find(const T &o)
    {
        loop(i, ulen) if(buf[i]==o) return i;
        return -1;
    }

    void removeobj(const T &o)
    {
        loop(i, ulen) if(buf[i]==o) remove(i--);
    }

    void replacewithlast(const T &o)
    {
        if(!ulen) return;
        loop(i, ulen-1) if(buf[i]==o)
        {
            buf[i] = buf[ulen-1];
        }
        ulen--;
    }

    T &insert(int i, const T &e)
    {
        add(T());
        for(int p = ulen-1; p>i; p--) buf[p] = buf[p-1];
        buf[i] = e;
        return buf[i];
    }

    T *insert(int i, const T *e, int n)
    {
        if(ulen+n>alen) growbuf(ulen+n);
        loop(j, n) add(T());
        for(int p = ulen-1; p>=i+n; p--) buf[p] = buf[p-n];
        loop(j, n) buf[i+j] = e[j];
        return &buf[i];
    }
};
