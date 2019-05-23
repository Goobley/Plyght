#include <cstdio>
#include <cmath>
#include <vector>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// Do this define in 1 file.
#define PLYGHT_IMPL
#include "Plyght.hpp"

constexpr bool WaitForKeyDown = true;
void interactive_wait()
{
    if (WaitForKeyDown)
    {
        getchar();
    }
}

int main(void)
{
    const int nPtsSmall = 10;
    const int nPtsLarge = 10000;
    const int nPeriods = 100;
    double xsSmall[nPtsSmall];
    double ysSmall[nPtsSmall];
    double ysSmall2[nPtsSmall];
    double xsLarge[nPtsLarge];
    double ysLarge[nPtsLarge];
    double ysLarge2[nPtsLarge];

    for (int i = 0; i < nPtsSmall; ++i)
    {
        xsSmall[i] = (double)(i * nPeriods) * M_PI / (double)(nPtsSmall - 1);
        ysSmall[i] = sin(xsSmall[i]);
        ysSmall2[i] = ysSmall[i] * ysSmall[i];
    }

    for (int i = 0; i < nPtsLarge; ++i)
    {
        xsLarge[i] = (double)(i * nPeriods) * M_PI / (double)(nPtsLarge - 1);
        ysLarge[i] = sin(xsLarge[i]);
        ysLarge2[i] = ysLarge[i] * ysLarge[i];
    }

    plyght().start_frame()
            .plot()
            .line_label("Sine (low res)")
            .line(xsSmall, ysSmall, nPtsSmall)
            .line_label("SineSquared (low res)")
            .line(xsSmall, ysSmall2, nPtsSmall)
            .line_label("Sine (high res)")
            .line(xsLarge, ysLarge, nPtsLarge)
            .line_style("--b")
            .line_label("SineSquared (high res)")
            .line(xsLarge, ysLarge2, nPtsLarge)
            .legend()
            .end_frame();

    interactive_wait();

    std::vector<double> v = {0, 2, 1, 0.5};
    std::vector<double> x = {0, 1, 2, 3, 4};

    // Using templates to get the length of vectors directly - no need to provide n
    plyght().start_frame()
            .plot()
            .line(x, v)
            .end_frame();

    interactive_wait();

    // 2 subplots
    plyght().start_frame()
            .plot()
            .line_label("Sine (low res)")
            .line_style("+r")
            .line(xsSmall, ysSmall, nPtsSmall)
            .line_label("SineSquared (low res)")
            .line_style("-.g")
            .line(xsSmall, ysSmall2, nPtsSmall)
            .legend()
            .plot()
            .line_label("Sine (high res)")
            .line(xsLarge, ysLarge, nPtsLarge)
            .line_style("--b")
            .line_label("SineSquared (high res)")
            .line(xsLarge, ysLarge2, nPtsLarge)
            .x_range(100,150)
            .legend()
            .end_frame();

    return 0;
}