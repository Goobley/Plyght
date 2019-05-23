// Plyght.hpp
// (c) Chris Osborne 2016-2019
// MIT License: https://opensource.org/licenses/MIT
//-------------------------------------------------------------------------------
// C++ interface to the Plyght plotting server
// All of the main plotting functions return the Plyght object so they can be
// chained as per the builder pattern.
//
// To use this library define PLYGHT_IMPL in _one_ C++file, before including the 
// header, elsewhere, just include the header.
// For each subplot, the line or imshow should be the last command issued before
// the next .plot() or final end_frame(). i.e. styling should be provided before
// the data. Legend however should be called last. This will be easiest to see
// from examples
//
// e.g. 
// typedef std::vector<double> DoubleVec;
// DoubleVec x;
// DoubleVec y; 
// DoubleVec y2; 
// const int iMax = 10;
// for (int i = 0; i < iMax; ++i) {
//     x.emplace_back(f64(i) / (iMax-1) * pi)
//     y.emplace_back(sin(x[i]))
//     y2.emplace_back(y[i] * y[i]);
// }
// plyght().start_frame()
//         .plot()
//         .line_style("+r")
//         .line_label("Sine")
//         .line(x, y)
//         .line_style("--b')
//         .line_label("SineSquared")
//         .line(x, y2)
//         .legend()
//         .end_frame()
#if !defined(PLYGHT_H)
#define PLYGHT_H
#include <string>
#include <sstream>

#define init_check() do { if (!init()) { return *this; } } while(false)

struct Plyght
{
    bool isInit;
    bool initError;
    int fd;
    Plyght();
    ~Plyght();

    bool init();
    void send(const std::string& str);

    Plyght& start_frame();
    Plyght& end_frame();

    template <typename T>
    Plyght& line(T xs, T ys, int len = 0)
    {
        init_check();
        send("!!StartPts\n");
        if (len == 0)
        {
            len = std::min(xs.size(), ys.size());
        }
        for (int i = 0; i < len; ++i)
        {
            std::stringstream str;
            str << "!!Pt<" << xs[i] << "," << ys[i] << ">\n";
            send(str.str());
        }
        send("!!EndPts\n");
        return *this;
    }

    template <typename T>
    Plyght& line(T* xs, T* ys, int len)
    {
        init_check();
        send("!!StartPts\n");
        for (int i = 0; i < len; ++i)
        {
            std::stringstream str;
            str << "!!Pt<" << xs[i] << "," << ys[i] << ">\n";
            send(str.str());
        }
        send("!!EndPts\n");
        return *this;
    }

    Plyght& line_style(const std::string& style);
    Plyght& line_label(const std::string& label);
    Plyght& plot();
    Plyght& plot_type(const std::string& type);
    Plyght& title(const std::string& title);
    Plyght& x_label(const std::string& title);
    Plyght& y_label(const std::string& title);
    Plyght& legend(const std::string& location = "");
    Plyght& print(const std::string& file, int dpi = 0);
    Plyght& fig_size(double xSize, double ySize);
    Plyght& x_range(double min, double max);
    Plyght& y_range(double min, double max);
    Plyght& colormap(const std::string& cmap);
};

static Plyght& plyght();

#define PLYGHT_IMPL
#ifdef PLYGHT_IMPL
#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

Plyght::Plyght() : isInit(false), initError(false), fd(0) {}
Plyght::~Plyght() 
{ 
    if (!initError && fd != -1) 
    {
        close(fd);
    }
}

bool Plyght::init()
{
    if (!isInit)
    {
        isInit = true;
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
        {
            initError = true;
            assert(false && "Unable to create socket");
        }
        else
        {
            sockaddr_in server;
            server.sin_family = AF_INET;
            server.sin_port = htons(41410);
            server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

            if (connect(fd, (sockaddr*)&server, sizeof(server)) == -1)
            {
                printf("Is Plyght running?\n");
                initError = true;
            }
        }
    }
    return !initError;
}

void Plyght::send(const std::string& str)
{
    write(fd, str.c_str(), str.size());
}

Plyght& Plyght::start_frame()
{
    init_check();

    send("!!StartIBuf\n");
    return *this;
}

Plyght& Plyght::end_frame()
{
    init_check();
    send("!!EndIBuf\n");
    return *this;
}

Plyght& Plyght::line_style(const std::string& style)
{
    init_check();
    std::stringstream str;
    str << "!!Line<" << style << ">\n";
    send(str.str());
    return *this;
}

Plyght& Plyght::line_label(const std::string& label)
{
    init_check();
    std::stringstream str;
    str << "!!Label<" << label << ">\n";
    send(str.str());
    return *this;
}

Plyght& Plyght::plot()
{
    init_check();
    send("!!New2D\n");
    return *this;
}

Plyght& Plyght::plot_type(const std::string& type)
{
    init_check();
    std::stringstream str;
    str << "!!Plot<" << type << ">\n";
    send(str.str());
    return *this;
}

Plyght& Plyght::title(const std::string& title)
{
    init_check();
    std::stringstream str;
    str << "!!Title<" << title << ">\n";
    send(str.str());
    return *this;
}

Plyght& Plyght::x_label(const std::string& title)
{
    init_check();
    std::stringstream str;
    str << "!!XTitle<" << title << ">\n";
    send(str.str());
    return *this;
}

Plyght& Plyght::y_label(const std::string& title)
{
    init_check();
    std::stringstream str;
    str << "!!YTitle<" << title << ">\n";
    send(str.str());
    return *this;
}

Plyght& Plyght::legend(const std::string& location)
{
    init_check();
    std::stringstream str;
    str << "!!Legend<" << location << ">\n";
    send(str.str());
    return *this;
}

Plyght& Plyght::print(const std::string& file, int dpi)
{
    init_check();
    std::stringstream str;
    if (dpi != 0)
    {
        str << "!!Dpi<" << dpi << ">\n";
    }
    str << "!!Print<" << file << ">\n";
    send(str.str());
    return *this;
}

Plyght& Plyght::fig_size(double xSize, double ySize)
{
    init_check();
    std::stringstream str;

    str << "!!FigSize<" << xSize << "," << ySize << ">\n";
    send(str.str());
    return *this;
}

Plyght& Plyght::x_range(double min, double max)
{
    init_check();
    std::stringstream str;
    str << "!!XRange<" << min << "," << max << ">\n";
    send(str.str());
    return *this;
}

Plyght& Plyght::y_range(double min, double max)
{
    init_check();
    std::stringstream str;
    str << "!!YRange<" << min << "," << max << ">\n";
    send(str.str());
    return *this;
}

Plyght& Plyght::colormap(const std::string& cmap)
{
    init_check();
    std::stringstream str;
    str << "!!Colormap<" << cmap << ">\n";
    send(str.str());
    return *this;
}

static Plyght& plyght()
{
    static Plyght p;
    return p;
}

#endif // PLYGHT_IMPL

#else
#endif // PLYGHT_H