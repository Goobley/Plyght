# Plyght: _a simple socket server interface to matplotlib_

When writing simulations it is often essential to use a low-level
high-performance language.
Debugging scientific code and algorithms written in these languages can be
difficult due to the difficulty of seeing the intermediate outputs produced.
Printing and plotting these outputs separately also becomes rapidly tiresome.
Plyght was created to fill this niche with as little code as possible.

Plyght is definitely not the most high-performance plotting tool out there,
instead it aims provide a familiar interface that is easy to use whilst
introducing a minimum of code into the target.
This is achieved by running a socket server in Python that accepts a
text-based stream of commands corresponding to a simple but relatively
complete subset of matplotlib functionality.
The functionality implemented so far is based on what I use, but the program
is designed in such a way that it is quite simple to add additional features
present in the pyplot API.

Plyght is composed of two components, the server, written in Python 3 and the
client (currently available in LuaJIT and C++). 
The client translates function calls that resemble the pyplot/MATLAB interface.

To use Plyght first run the server and then launch the program. 
On calling `init()` the client will connect to the server.
Plyght can then be called for plotting. 
In languages with some form of object-oriented capability the methods can be
chained from the `Plyght` object in the style of the builder pattern.

To plot a simple graph of `x` against `y` where `x` and `y` are tables in Lua
or vectors in C++ (i.e. arrays that also contain a method for accessing their
length):

- call `start_frame()`, this indicates that the plotting window is to be cleared and a new plot is to be started
- call `plot()`, this indicates the start of a new subplot on the current figure
- (optional) call `line_label("FooLine")`, this tags the line with a string to be shown in the legend if this is requested later
- (optional) call `line_style("--r")`, use a format string to specify the line drawing style. Check MATLAB or matplotlib docs for available options
- (optional) call `plot_style("semilogx")` to plot the graph on a log x-axis and linear y axis. Other options are `linlin` (default), `semilogy`. `loglog`
- call `line(x, y)`. If `x` and `y` do not have a `__len` metamethod (Lua) or a `size()` method (C++) then their length can be passed in as a third, optional argument.
- (optional) call `x_label("Some fundamental parameter")`
- (optional) call `y_label("Thing being measured")`
- (optional) call `title("Graph of some thing")`
- (optional) call `legend()` if a legend is desired
- (optional) call `print("file.png")` save graph to file.png, the format is inferred from the extension
- call `end_frame()` to finalise the graph and have it displayed

If more subplots are desired, then `plot()` can be called again before
`end_frame()` and the new subplot can be built in exactly the same way as the
previous. The order of these calls is is not entirely fixed, though there are
some ordering requirments, such as the line style being provided before the
line is drawn, and the legend can only be drawn after all lines on the graph.
It is also worth noting that setting the line style persists into other lines
and subplots due to the stateful nature of pyplot.

Plyght has been tested on Mac OS 10.11, 10.12, Linux Mint 17.2, Ubunutu 14.04 & CentOS 7.
Pull requests providing additional backends written in the same style as
those provided will be accepted, and feel free to ask for help constructing a
new client.
