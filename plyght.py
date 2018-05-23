## plyght.py
#  (c) Chris Osborne 2016-2018
#  MIT License: https://opensource.org/licenses/MIT
#  A Python based server that acts as an interface to the state-based matplotlib.pyplot
#  Plyght therefore allows any programming language with a socket interface and formatted IO to plot to matplotlib.

import socket
import numpy as np
import matplotlib
from math import sqrt, floor
import re
import time
import select
import sys
import platform

# The default Mac OS X backend doesn't support the non-blocking mode. So pick
# another. I chose Qt5. This will impose an additional requirement to install
# on OSX.
if platform.system() == 'Darwin':
    matplotlib.use('Qt5Agg')
import matplotlib.pyplot as plt

# Constant Strings used as control commands.

# Indicates start and end of instruction stream.
EndBufferOp = '!!EndIBuf'
StartBufferOp = '!!StartIBuf'

# Start another 2D plot on the current incomplete figure.
New2DPlotOp = '!!New2D'
# Plot a 2D image on the current figure.
ImShowOp = '!!ImShow'
# Indicates the start and end of a list of points.
StartPtListOp = '!!StartPts'
EndPtListOp = '!!EndPts'
# (x,y) point object.
PointOp = '!!Pt<'
# Style of line, provided before drawing. Matlab style, i.e. '-', 'o', '--', 'x'...
LineStyleOp = '!!Line<'

# Axis types
# Types are: linlin, semilogx, semilogy, loglog
PlotTypeOp = '!!Plot<'

# Provide titles for various elements as strings.
TitleOp = '!!Title<'
XTitleOp = '!!XTitle<'
YTitleOp = '!!YTitle<'
SupTitleOp = '!!SupTitle<'
# String label for a line to be included in the legend.
LabelOp = '!!Label<'

# Draw the legend, with an optional forced location string.
LegendOp = '!!Legend<'
# Possible Location Strings
# '', 'best', 'upper right', 'upper left', 'lower left', 'lower right',
# 'right', 'center left', 'center right', 'lower center', 'upper center',
# 'center'

# Print to filename provided.
PrintOp = '!!Print<'
# Limit x-range (2-tuple)
XRangeOp = '!!XRange<'
# Limit y-range (2-tuple)
YRangeOp = '!!YRange<'
# Add a colourbar.
ColorbarOp = '!!Colorbar'
# Provide the dimensions of an incoming 2D image array so it can be stored efficiently.
DimensionOp = '!!Dimension<'
# Contains a single value, typically 64-bit float.
ValueOp = '!!Value<'


# Calculate subplot grid layout -- biased towards producing columns over rows.
# This can be modified quite simply
# Num -> (Num, Num)
def subplot_grid_dims(num_plots):
    if num_plots == 1:
        return (1, 1)
    norm_ar = 1
    grid_height = sqrt(norm_ar * num_plots)
    grid_width = sqrt(num_plots / norm_ar)

    grid_height = floor(grid_height)
    grid_width = floor(grid_width)

    if grid_height * grid_width >= num_plots:
        return (grid_height, grid_width)
    else:
        if grid_height < grid_width:
            if (grid_height + 1) * grid_width >= num_plots:
                return (grid_height + 1, grid_width)
            elif grid_height * (grid_width + 1) >= num_plots:
                return (grid_height, grid_width + 1)
            else:
                return (grid_height + 1, grid_width + 1)  # This has to be big enough
        else:
            if grid_height * (grid_width + 1) >= num_plots:
                return (grid_height, grid_width + 1)
            elif (grid_height + 1) * grid_width >= num_plots:
                return (grid_height + 1, grid_width)
            else:
                return (grid_height + 1, grid_width + 1)  # This has to be big enough


# Parses all the !!Pt<x,y> commands from the provided list and returns a list [x...], [y...]
# [Str] -> ([Num], [Num])
def parse_pt_list(pts):
    xs = []
    ys = []
    pt_regex = re.compile('^!!Pt<([^,>]*),([^,>]*)>$')
    for pt in pts:
        if pt.startswith(PointOp):
            res = pt_regex.match(pt)
            try:
                xs.append(float(res.group(1)))
                ys.append(float(res.group(2)))
            except AttributeError:
                print('Parsing pt failed: ' + pt)
    return xs, ys


# Parse an image, from its Dimension<> and Value<> s.
# [Str] -> 2D[Num]
def parse_2d_pt_list(pts):
    grid = None
    dims = []
    try:
        res = re.match('^!!Dimension<([^,>]*),([^,>]*)>$', pts[0])
        xDim = int(res.group(1))
        yDim = int(res.group(2))
        grid = np.zeros((yDim, xDim))
        dims.append(xDim)
        dims.append(yDim)
        if not pts[1].startswith(StartPtListOp):
            print('No point list provided')
            return
    except:
        print('Parsing Dimension of 2D List failed')

    val_regex = re.compile('^!!Value<([^,>]*)>$')
    ptVals = pts[2:] 
    for y in range(dims[1]):
        for x in range(dims[0]):
            grid[y, x] = float(val_regex.match(ptVals[y * dims[0] + x]).group(1))
    return grid


# Save the current figure to the provided filename. Format is inferred from extension.
# Str -> IO ()
def print_graph(filename):
    if filename is not None:
        plt.savefig(filename)


# Set the x and y ranges of the current plot
# Maybe (Num, Num) -> Maybe (Num, Num) -> IO ()
def set_ranges(x_range, y_range):
    if x_range is not None:
        plt.xlim(x_range)
    if y_range is not None:
        plt.ylim(y_range)

# Function that does the majority of the work.
# Processes a list of arguments that make up the description of a graph and plots the corresponding output.
# [Str] -> IO ()
def process_frame(f):
    # Clear previous frame
    plt.clf()
    # Calculate num plots
    subplt_cnt = f.count(New2DPlotOp)
    if subplt_cnt == 0:
        return

    # Work out grid shape
    grid_rows, grid_cols = subplot_grid_dims(subplt_cnt)

    # idx is the "instruction pointer" (i.e. here this just implies the index of the current instruction in the instruction buffer)
    idx = f.index(New2DPlotOp)
    for i in range(subplt_cnt):
        idx += 1
        # Retrieve the associated instruction
        inst = f[idx]
        # Create blank figure with the correct number of subplots and select the i+1th one.
        plt.subplot(grid_rows, grid_cols, i+1)
        # Create empty "style" dict.
        next_style = {'Plot': None, 'Line': None, 'Label': None, 'Print': None, 'XRange': None,
                      'YRange': None, 'Colorbar': None}
        # Loop over all instructions relating to this subplot.
        while inst != New2DPlotOp:

            # Parse a list of points (i.e. a line), or a 2D array of points.
            if inst == StartPtListOp or inst.startswith(DimensionOp):

                # Find end of current point list
                endPtListIdx = f[idx:].index(EndPtListOp) + idx

                # If this is a 2D plot then parse list and use imshow with optional colourbar.
                if next_style['Plot'] == 'imshow':
                    # 2D image plot
                    data = parse_2d_pt_list(f[idx:endPtListIdx+1])
                    plt.imshow(data, cmap='plasma', origin='lower')
                    if next_style['Colorbar'] is not None:
                        plt.colorbar()

                else:
                    # Line plots
                    line = parse_pt_list(f[idx:endPtListIdx+1])

                    # There seems to be no better way of doing this, but at least there's only 4/5 cases.
                    if next_style['Plot'] is None or next_style['Plot'] == 'linlin':
                        if next_style['Line'] is not None:
                            plt.plot(line[0], line[1], next_style['Line'], label=next_style['Label'])
                        else:
                            plt.plot(line[0], line[1], label=next_style['Label'])
                    elif next_style['Plot'] == 'semilogx':
                        if next_style['Line'] is not None:
                            plt.semilogx(line[0], line[1], next_style['Line'], label=next_style['Label'])
                        else:
                            plt.semilogx(line[0], line[1], label=next_style['Label'])
                    elif next_style['Plot'] == 'semilogy':
                        if next_style['Line'] is not None:
                            plt.semilogy(line[0], line[1], next_style['Line'], label=next_style['Label'])
                        else:
                            plt.semilogy(line[0], line[1], label=next_style['Label'])
                    elif next_style['Plot'] == 'loglog':
                        if next_style['Line'] is not None:
                            plt.loglog(line[0], line[1], next_style['Line'], label=next_style['Label'])
                        else:
                            plt.loglog(line[0], line[1], label=next_style['Label'])

                # Move instruction pointer to end of point list.
                idx = endPtListIdx
                inst = f[idx]

            ## Parse other options, mostly just styling strings.
            if inst.startswith(LineStyleOp):
                next_style['Line'] = re.match('^!!Line<([^>]*)>$', inst).group(1)

            if inst.startswith(PlotTypeOp):
                next_style['Plot'] = re.match('^!!Plot<([^>]*)>$', inst).group(1)

            if inst.startswith(TitleOp):
                title = re.match('^!!Title<([^>]*)>$', inst).group(1)
                print(title)
                plt.title(title)

            if inst.startswith(XTitleOp):
                x_title = re.match('^!!XTitle<([^>]*)>$', inst).group(1)
                plt.xlabel(x_title)

            if inst.startswith(YTitleOp):
                y_title = re.match('^!!YTitle<([^>]*)>$', inst).group(1)
                plt.ylabel(y_title)

            if inst.startswith(SupTitleOp):
                sup_title = re.match('^!!SupTitle<([^>]*)>$', inst).group(1)
                plt.suptitle(sup_title)

            if inst.startswith(LabelOp):
                next_style['Label'] = re.match('^!!Label<([^>]*)>$', inst).group(1)

            if inst.startswith(LegendOp):
                loc = re.match('^!!Legend<([^>]*)>$', inst).group(1)
                if loc == '':
                    loc = 'best'
                plt.legend(loc=loc)

            if inst.startswith(PrintOp):
                next_style['Print'] = re.match('^!!Print<([^>]*)>$', inst).group(1)

            if inst.startswith(XRangeOp):
                lims = re.match('^!!XRange<([^,>]*),([^,>]*)>$', inst)
                next_style['XRange'] = (float(lims.group(1)), float(lims.group(2)))

            if inst.startswith(YRangeOp):
                lims = re.match('^!!YRange<([^,>]*),([^,>]*)>$', inst)
                next_style['YRange'] = (float(lims.group(1)), float(lims.group(2)))

            if inst.startswith(ColorbarOp):
                next_style['Colorbar'] = True

            if inst.startswith(ImShowOp):
                next_style['Plot'] = 'imshow'

            if inst == EndBufferOp:
                break

            idx += 1
            inst = f[idx]

        set_ranges(next_style['XRange'], next_style['YRange'])
    plt.tight_layout()
    print_graph(next_style['Print'])
    # This seems to help matplotlib behave better -- not really sure why.
    plt.pause(0.00001)


# Keeps the output window updated and interactive without matplotlib consuming huge amounts of CPU cycles.
# Time -> Time
def update_plots(prev_time):
    # Sleep for at least 25 msj
    if time.clock() < prev_time + 0.025:
        time.sleep(prev_time + 0.025 - time.clock())

    # Loop over figures and check for events
    for i in plt.get_fignums():
        fig = plt.figure(i)
        try:
            fig.canvas.flush_events()
        except NotImplementedError:
            pass
    prev_time = time.clock()
    return prev_time

# Main driver for plyght.
# Instruction buffer
instr_list = []
# Enable matplotlib in interactive mode and open an empty plotting window.
plt.ion()
plt.subplots()
plt.show()

# Set up and start listening to the socket on 41410
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.bind(('localhost', 41410))
server_socket.listen(1)
prev_time = time.clock()
# Process input to the socket.
while True:
    try:
        # Select the socket if there is data to be ready
        socket_ready = select.select([server_socket], [], [], 0.02)[0]
        if len(socket_ready) > 0:
            connection, address = server_socket.accept()
            while True:
                # Check for data and receive up to 1MB if there is.
                readable = select.select([connection], [], [], 0.02)[0]
                if len(readable) > 0:
                    buf = connection.recv(1048576)

                    if len(buf) > 0:
                        # Decode buffer, split on \n and append to extant
                        # instruction list whilst taking care to stick
                        # instructions that may have been broken by the buffer
                        # boundary back together.
                        buf = buf.decode('UTF-8')
                        buf = buf.splitlines()
                        if len(instr_list) > 0:
                            # Check if leading character of next data is !!, if
                            # it is then make it a new op. This could go wrong
                            # if we broke around this in a title, but I feel
                            # it's unlikely to have !! in a title, especially
                            # with a 1MB buffer
                            if not buf[0].startswith('!!'):
                                instr_list[-1] += buf[0]
                                buf = buf[1:]
                        instr_list = instr_list + buf

                        # If we have both a Start and EndBufferOp in the list
                        # then we are good to process the sub-buffer containing
                        # these and produce a frame.
                        while EndBufferOp in instr_list:
                            idx = instr_list.index(EndBufferOp)
                            current_frame = instr_list[:idx+1]
                            instr_list = instr_list[idx+1:]
                            if StartBufferOp in current_frame:
                                idx = current_frame.index(StartBufferOp)
                                current_frame = current_frame[idx:]
                                process_frame(current_frame)
                            else:
                                print("Error parsing frame")
                    else:
                        break

                else:
                    # Otherwise just update the current plots
                    prev_time = update_plots(prev_time)
        else:
            # Otherwise just update the current plots
            prev_time = update_plots(prev_time)

    except KeyboardInterrupt:
        # Having this keyboard interrupt allows plyght to be closed from the terminal with ^C.
        server_socket.close()
        sys.exit(0)
