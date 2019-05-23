#include <stdio.h>
#include <tgmath.h>
#include <stdbool.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// Do this define in 1 file.
#define PLYGHT_IMPL
#include "Plyght.h"

static bool WaitForKeyDown = true;
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

    // Do the init and close once per program run, and issue the frame commands every time you want a new plot.
    plyght_init();

    plyght_start_frame();
    plyght_plot();
    plyght_line_label("Sine (low res)");
    plyght_line(xsSmall, ysSmall, nPtsSmall);
    plyght_line_label("SineSquared (low res)");
    plyght_line(xsSmall, ysSmall2, nPtsSmall);
    plyght_line_label("Sine (high res)");
    plyght_line(xsLarge, ysLarge, nPtsLarge);
    plyght_line_style("--b");
    plyght_line_label("SineSquared (high res)");
    plyght_line(xsLarge, ysLarge2, nPtsLarge);
    plyght_legend(NULL);
    plyght_end_frame();

    interactive_wait();

    // 2 subplots
    plyght_start_frame();
    plyght_plot();
    plyght_line_label("Sine (low res)");
    plyght_line_style("+r");
    plyght_line(xsSmall, ysSmall, nPtsSmall);
    plyght_line_label("SineSquared (low res)");
    plyght_line_style("-.g");
    plyght_line(xsSmall, ysSmall2, nPtsSmall);
    plyght_legend(NULL);
    plyght_plot();
    plyght_line_label("Sine (high res)");
    plyght_line(xsLarge, ysLarge, nPtsLarge);
    plyght_line_style("--b");
    plyght_line_label("SineSquared (high res)");
    plyght_line(xsLarge, ysLarge2, nPtsLarge);
    plyght_x_range(100,150);
    plyght_legend(NULL);
    plyght_end_frame();

    plyght_close();
    return 0;
}