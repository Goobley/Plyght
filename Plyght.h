// Plyght.h
// (c) Chris Osborne 2016-2019
// MIT License: https://opensource.org/licenses/MIT
//-------------------------------------------------------------------------------
// C interface to the Plyght plotting server
//
// To use this library define PLYGHT_IMPL in _one_ C file, before including the 
// header, elsewhere, just include the header.
//
// For each subplot, the line or imshow should be the last command issued before
// the next plyght_plot() or final plyght_end_frame(). i.e. styling should be provided before
// the data. Legend however should be called last. This will be easiest to see
// from examples (see examples/PlyghtTest.c in the repo)
#if !defined(PLYGHT_H)
#define PLYGHT_H
#include <stdbool.h>

typedef struct Plyght
{
    bool isInit;
    bool initError;
    int fd;
} Plyght; 

#define get_p() Plyght* p = plyght()
#define init_check() do { if (!plyght_init()) { return; } } while (false)
#define CONCAT_IMPL(x,y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#ifndef NO_PLYGHT_PREFIX
    #ifndef PLYGHT_PREFIX
        #define PLYGHT_PREFIX plyght_
    #endif
#endif
#ifdef PLYGHT_PREFIX 
    #define PLYGHT_FN(x) CONCAT(PLYGHT_PREFIX, x) 
#else 
    #define PLYGHT_FN(x) x
#endif

#ifndef PLYGHT_IMPL

bool plyght_init();
void plyght_close();
void plyght_send(Plyght* p, const char* s, int n);
void PLYGHT_FN(start_frame)();
void PLYGHT_FN(end_frame)();
void PLYGHT_FN(line)(double* xs, double* ys, int len);
void PLYGHT_FN(line_style)(const char* style);
void PLYGHT_FN(line_label)(const char* style);
void PLYGHT_FN(plot)();
void PLYGHT_FN(plot_type)(const char* type);
void PLYGHT_FN(title)(const char* title);
void PLYGHT_FN(x_label)(const char* title);
void PLYGHT_FN(y_label)(const char* title);
void PLYGHT_FN(legend)(const char* location);
void PLYGHT_FN(print)(const char* file, int dpi);
void PLYGHT_FN(fig_size)(double xSize, double ySize);
void PLYGHT_FN(x_range)(double min, double max);
void PLYGHT_FN(y_range)(double min, double max);
void PLYGHT_FN(colormap)(const char* colormap);

#else
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

Plyght* plyght()
{
    static Plyght p;
    return &p;
}

bool plyght_init()
{
    get_p();
    if (!p->isInit)
    {
        p->isInit = true;
        p->fd = socket(AF_INET, SOCK_STREAM, 0);
        if (p->fd == -1)
        {
            p->initError = true;
            assert(false && "Unable to create socket");
        }
        else
        {
            struct sockaddr_in server;
            server.sin_family = AF_INET;
            server.sin_port = htons(41410);
            server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

            if (connect(p->fd, (struct sockaddr*)&server, sizeof(server)) == -1)
            {
                printf("Is Plyght running?\n");
                p->initError = true;
            }
        }
    }
    return !p->initError;
}

void plyght_close()
{
    get_p();
    if (!p->initError && p->fd != -1)
    {
        close(p->fd);
    }
    p->isInit = false;
}

void plyght_send(Plyght* p, const char* s, int n)
{
    write(p->fd, s, n);
}

void PLYGHT_FN(start_frame)()
{
    init_check();
    get_p();
    const char str[] = "!!StartIBuf\n";
    plyght_send(p, str, strlen(str));
}

void PLYGHT_FN(end_frame)()
{
    init_check();
    get_p();
    const char str[] = "!!EndIBuf\n";
    plyght_send(p, str, strlen(str));
}

void PLYGHT_FN(line)(double* xs, double* ys, int len)
{
    init_check();
    get_p();
    const char str[] = "!!StartPts\n";
    plyght_send(p, str, strlen(str));

    char buf[512];
    for (int i = 0; i < len; ++i)
    {
        snprintf(buf, 512, "!!Pt<%.16e,%.16e>\n", xs[i], ys[i]);
        plyght_send(p, buf, strlen(buf));
    }
    const char endStr[] = "!!EndPts\n";
    plyght_send(p, endStr, strlen(endStr));
}

void PLYGHT_FN(line_style)(const char* style)
{
    init_check();
    get_p();

    char buf[512];
    snprintf(buf, 512, "!!Line<%s>\n", style);
    plyght_send(p, buf, strlen(buf));
}

void PLYGHT_FN(line_label)(const char* style)
{
    init_check();
    get_p();

    char buf[512];
    snprintf(buf, 512, "!!Label<%s>\n", style);
    plyght_send(p, buf, strlen(buf));
}

void PLYGHT_FN(plot)()
{
    init_check();
    get_p();

    const char str[] = "!!New2D\n";
    plyght_send(p, str, strlen(str));
}

void PLYGHT_FN(plot_type)(const char* type)
{
    init_check();
    get_p();

    char buf[512];
    snprintf(buf, 512, "!!Plot<%s>\n", type);
    plyght_send(p, buf, strlen(buf));
}

void PLYGHT_FN(title)(const char* title)
{
    init_check();
    get_p();

    char buf[2048];
    snprintf(buf, 2048, "!!Title<%s>\n", title);
    plyght_send(p, buf, strlen(buf));
}

void PLYGHT_FN(x_label)(const char* title)
{
    init_check();
    get_p();

    char buf[2048];
    snprintf(buf, 2048, "!!XTitle<%s>\n", title);
    plyght_send(p, buf, strlen(buf));
}

void PLYGHT_FN(y_label)(const char* title)
{
    init_check();
    get_p();

    char buf[2048];
    snprintf(buf, 2048, "!!YTitle<%s>\n", title);
    plyght_send(p, buf, strlen(buf));
}

void PLYGHT_FN(legend)(const char* location)
{
    init_check();
    get_p();

    char buf[512];
    if (location)
    {
        snprintf(buf, 512, "!!Legend<%s>\n", location);
    }
    else
    {
        snprintf(buf, 512, "!!Legend<>\n");
    }
    
    plyght_send(p, buf, strlen(buf));
}

void PLYGHT_FN(print)(const char* file, int dpi)
{
    init_check();
    get_p();

    char buf[2048];
    if (dpi != 0)
    {
        snprintf(buf, 2048, "!!Dpi<%d>\n", dpi);
        plyght_send(p, buf, strlen(buf));
    }
    snprintf(buf, 2048, "!!Print<%s>\n", file);
    
    plyght_send(p, buf, strlen(buf));
}

void PLYGHT_FN(fig_size)(double xSize, double ySize)
{
    init_check();
    get_p();

    char buf[512];
    snprintf(buf, 512, "!!FigSize<%f,%f>\n", xSize, ySize);
    plyght_send(p, buf, strlen(buf));
}

void PLYGHT_FN(x_range)(double min, double max)
{
    init_check();
    get_p();

    char buf[512];
    snprintf(buf, 512, "!!XRange<%.16e,%.16e>\n", min, max);
    plyght_send(p, buf, strlen(buf));
}

void PLYGHT_FN(y_range)(double min, double max)
{
    init_check();
    get_p();

    char buf[512];
    snprintf(buf, 512, "!!YRange<%.16e,%.16e>\n", min, max);
    plyght_send(p, buf, strlen(buf));
}

void PLYGHT_FN(colormap)(const char* colormap)
{
    init_check();
    get_p();

    char buf[512];
    snprintf(buf, 512, "!!Colormap<%s>\n", colormap);
    plyght_send(p, buf, strlen(buf));
}


#endif // PLYGHT_IMPL

#else
#endif // PLYGHT_H