-- Plyght.lua
-- (c) Chris Osborne 2016-2018
-- MIT License: https://opensource.org/licenses/MIT
---------------------------------------------------------------------------------
-- LuaJIT interface to the Plyght plotting server
-- All of the main plotting functions return the Plyght table so they can be
-- chained as per the builder pattern.

-- For each subplot, the line or imshow should be the last command issued before
-- the next .plot() or final end_frame(). i.e. styling should be provided before
-- the data. Legend however should be called last. This will be easiest to see
-- from examples

-- e.g. 
-- x = {0, 0.2, 0.4, 0.6, 0.8, 1.0}
-- y = {}
-- y2 = {}
-- for i = 1,#x do 
    --     x[i] = x[i] * math.pi 
    --     y[i] = math.sin(x[i])
    --     y2[i] = (math.sin(x[i]))^2
    -- end
    -- Plyght:start_frame()
    --       :plot()
    --       :line_style('+r')
    --       :line_label('Sine')
    --       :line(x, y)
    --       :line_style('--b')
    --       :line_label('SineSquared')
    --       :line(x, y2)
    --       :legend()
    --       :end_frame()
    
    
    local ffi = require('ffi')
    -- cdef based on https://github.com/hnakamur/luajit-examples/blob/master/socket/cdef/socket.lua and C headers.
    ffi.cdef[[
    typedef uint32_t socklen_t;
    typedef unsigned short int sa_family_t;
    typedef uint16_t in_port_t;
    typedef uint32_t in_addr_t;
    static const int PF_INET = 2;
    static const int AF_INET = PF_INET;
    static const int SOCK_STREAM = 1;
    static const int INADDR_LOOPBACK = (in_addr_t)0x7F000001;
    struct in_addr 
    {
        in_addr_t s_addr;
    };
    struct sockaddr 
    {
        sa_family_t sin_family;
        /* Address data.  */
        char sa_data[14];		
    };
    
    struct sockaddr_in 
    {
        sa_family_t sin_family;
        in_port_t sin_port;
        struct in_addr sin_addr;
        
        /* Pad to size of struct sockaddr.  */
        unsigned char sin_zero[sizeof(struct sockaddr) -
        sizeof(sa_family_t) -
        sizeof(in_port_t) -
        sizeof(struct in_addr)];
    };
    
    uint16_t htons(uint16_t hostshort);
    uint32_t htonl(uint32_t hostlong);
    int socket(int domain, int type, int protocol);
    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    typedef int ssize_t;
    ssize_t read(int fd, void *buf, size_t count);
    ssize_t write(int fd, const void *buf, size_t count);
    int close(int fd);
    ]]
    
local C = ffi.C
local plyght = { isInit = false, initError = false, fd = 0 }
-- Initialisation function
-- Plyght () -> Plyght ()
function plyght:init()
    if not self.isInit then
        self.isInit = true
        self.fd = C.socket(C.AF_INET, C.SOCK_STREAM, 0)
        if self.fd == -1 then
            self.initError = true
            error('Unable to create socket')
        else
            local serverName = ffi.new("struct sockaddr_in[1]")
            serverName[0].sin_family = C.AF_INET
            serverName[0].sin_port = C.htons(41410);
            serverName[0].sin_addr.s_addr = C.htonl(C.INADDR_LOOPBACK)
            
            if C.connect(self.fd, ffi.cast("struct sockaddr *", serverName), ffi.sizeof(serverName)) == -1 then
                print('Is Plyght running?')
                self.initError = true
            end
            self.instrBuf = {}
            
            -- NOTE(Chris): Doesn't seem to work, but oh well, the OS
            -- will release all handles and sockets when the process
            -- terminates
            -- local mt = {__gc =
            --                function(self)
                --                   print('Closing')
                --                   if not self.initError then
                    --                      C.close(self.fd)
                    --                      self.initError = true
                    --                   end
                    -- end }
                    
                    -- setmetatable(self, mt)
        end
    end
    return not self.initError
end
        
-- Internal function to add to the pending instruction buffer
-- Plyght Str -> Plyght ()
function plyght:send(str)
    self.instrBuf[#self.instrBuf + 1] = str
end

-- Close plyght's file handles
-- Plyght () -> Plyght ()
function plyght:close()
    if not self.initError then
        C.close(self.fd)
        self.initError = true
    end
end
        
-- First instruction to be issued, indicating the start of a new plot
-- Plyght () -> Plyght ()
function plyght:start_frame()
    if not self:init() then
        return self
    end
    self.instrBuf = {}
    
    self:send('!!StartIBuf\n')
    return self
end

-- End current figure, causes the entire message to be sent, parsed and displayed on the graph
-- Plyght () -> IO () -> Plyght ()
function plyght:end_frame()
    if not self:init() then
        return self
    end
    
    self:send('!!EndIBuf\n\n')
    local finalStr = table.concat(self.instrBuf)
    local err = C.write(self.fd, finalStr, #finalStr)
    if err <= 0 then
        print('Write error', err)
        assert(false, 'Plyght write error')
    end
    
    self.instrBuf = {}
    return self
end

-- Plot a line, len optional if xs and ys are table like. Thus if they are
-- C-style arrays then it is required.
function plyght:line(xs, ys, len)
    if not self:init() then
        return self
    end
    
    self:send('!!StartPts\n')
    if len == nil then len = math.min(#xs, #ys) end
    for i = 1, len do
        local pt = '!!Pt<'..xs[i]..','..ys[i]..'>\n'
        self:send(pt)
    end
    
    self:send('!!EndPts\n')
    return self
end

-- Plot a 2D image, dimensions required
function plyght:imshow(mat, xDim, yDim, minVal, maxVal)
    if not self:init() then
        return self
    end
    if minVal and maxVal then
        self:send('!!ImShow<'..minVal..','..maxVal..'>\n')
    else
        self:send('!!ImShow\n')
    end
    
    
    self:send('!!Dimension<'..xDim..','..yDim..'>\n')
    self:send('!!StartPts\n')
    for y = 1, yDim do
        for x = 1, xDim do
            local val = '!!Value<'..mat[(y-1) * xDim + x]..'>\n'
            self:send(val)
        end
    end
    
    self:send('!!EndPts\n')
    return self
end

-- Add a colourbar next to a 2D image.
function plyght:colorbar()
    if not self:init() then
        return self
    end
    
    self:send('!!Colorbar\n')
    return self
end


-- Label for line in legend (string)
function plyght:line_label(label)
    if not self:init() then
        return self
    end
    
    self:send('!!Label<'..label..'>\n')
    return self
end

-- Add a subplot to the figure
function plyght:plot()
    if not self:init() then
        return self
    end
    
    self:send('!!New2D\n')
    return self
end

-- Specify plot type as a string see plyght.py for options
function plyght:plot_type(plotType)
    if not self:init() then
        return self
    end
    
    self:send('!!Plot<'..plotType..'>\n')
    return self
end

-- Title of subplot as a string
function plyght:title(title)
    if not self:init() then
        return self
    end
    
    self:send('!!Title<'..title..'>\n')
    return self
end

-- x-axis label of subplot as a string
function plyght:x_label(title)
    if not self:init() then
        return self
    end
    
    self:send('!!XTitle<'..title..'>\n')
    return self
end

-- y-axis label of subplot as a string
function plyght:y_label(title)
    if not self:init() then
        return self
    end
    
    self:send('!!YTitle<'..title..'>\n')
    return self
end

-- Legend with optional location string (see plyght.py) 
function plyght:legend(location)
    if not self:init() then
        return self
    end
    
    if not location then location = '' end
    self:send('!!Legend<'..location..'>\n')
    return self
end

-- Save to provided filename (relative to python interpreter). Format inferred from extension.
-- Optional DPI argument
function plyght:print(file, dpi)
    if not self:init() then
        return self
    end
    if dpi then
        self:send('!!Dpi<'..dpi..'>\n')
    end
    
    self:send('!!Print<'..file..'>\n')
    return self
end

-- Set the figure (and interactive window size) in inches
function plyght:fig_size(xSize, ySize)
    if not self:init() then
        return self
    end
    self:send('!!FigSize<'..xSize..','..ySize..'>\n')
    return self
end

-- Set x-range on current subplot
function plyght:x_range(min, max)
    if not self:init() then
        return self
    end
    
    self:send('!!XRange<'..min..','..max..'>\n')
    return self
end

-- Set y-range on current subplot
function plyght:y_range(min, max)
    if not self:init() then
        return self
    end
    
    self:send('!!YRange<'..min..','..max..'>\n')
    return self
end

-- Set the colormap (does not persist)
function plyght:colormap(cmap)
    if not self:init() then
        return self
    end
    
    self:send('!!Colormap<'..cmap..'>\n')
    return self
end

local function dub_quote(s)
    return '"'..tostring(s)..'"'
end

local function dub_quote_if_string(s)
    if type(s) == 'string' then
        return '"'..tostring(s)..'"'
    else
        return tostring(s)
    end
end

local function table_to_json(t)
    local j = '{'
    local count = 1
    for k, v in pairs(t) do
        if not (type(v) == 'string' or type(v) == 'number' or type(v) == 'boolean') then
            print(('invalid value: %s for key: %s'):format(tostring(v), tostring(k)))
            return '{}'
        end
        if count > 1 then j = j .. ',' end
        j = j .. dub_quote(k) .. ' : ' .. dub_quote_if_string(v)
        count = count + 1
    end
    j = j ..'}'
    return j
end

function plyght:rectangle(x, y, width, height, kwargs)
    if not self:init() then
        return self
    end
    
    if not fill then fill = true end
    if not rotation then rotation = 0 end
    if not kwargs then kwargs = '{}' else kwargs = table_to_json(kwargs) end
    -- else handle converting a lua table to simple json
        self:send('!!Rectangle<'..x..','..y..','..width..','..height..','..kwargs..'>\n')
        return self
    end
    
    -- Matlab style line format specifier (as a string) or a table that will be
    -- converted to a python dict and provided as the kwargs to plt.plot
    function plyght:line_style(style)
        if not self:init() then
            return self
        end
        
        if type(style) == 'string' then
            self:send('!!Line<'..style..'>\n')
        elseif type(style) == 'table' then
            kwargs = table_to_json(style)
            self:send('!!LineJson<'..kwargs..'>\n')
        else
            print(('Type %s is not a valid line style'):format(type(style)))
        end

        return self
    end
    
    
    return plyght
    