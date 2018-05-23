-- LuaJIT interface to the Plyght plotting server
-- All of the main plotting functions return the Plyght table so they can be
-- chained as per the builder pattern.
local ffi = require('ffi')
ffi.cdef[[
    static const int SOMAXCONN = 128;

    static const int PF_INET = 2;
    static const int AF_INET = PF_INET;

    static const int SOCK_STREAM = 1;

    int socket(int domain, int type, int protocol);

    typedef uint32_t socklen_t;

    static const int SOL_SOCKET = 1;
    static const int SO_REUSEADDR = 2;

    int setsockopt(int sockfd, int level, int optname, const void *optval,
    socklen_t optlen);

    typedef unsigned short int sa_family_t;
    typedef uint16_t in_port_t;
    typedef uint32_t in_addr_t;
    struct in_addr {
    in_addr_t s_addr;
    };
    static const int INADDR_ANY = (in_addr_t)0x00000000;
    static const int INADDR_LOOPBACK = (in_addr_t)0x7F000001;

    struct sockaddr {
    sa_family_t sin_family;
    char sa_data[14];		/* Address data.  */
    };

    struct sockaddr_in {
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
    int inet_aton(const char *cp, struct in_addr *inp);
    int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    int listen(int sockfd, int backlog);
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
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

function plyght:imshow(mat, xDim, yDim)
    if not self:init() then
        return self
    end
    self:send('!!ImShow\n')

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

function plyght:colorbar()
    if not self:init() then
        return self
    end

    self:send('!!Colorbar\n')
    return self
end

function plyght:line_style(style)
   if not self:init() then
      return self
   end

   self:send('!!Line<'..style..'>\n')
   return self
end

function plyght:line_label(label)
   if not self:init() then
      return self
   end

   self:send('!!Label<'..label..'>\n')
   return self
end

function plyght:plot()
   if not self:init() then
      return self
   end

   self:send('!!New2D\n')
   return self
end

function plyght:plot_type(plotType)
   if not self:init() then
      return self
   end

   self:send('!!Plot<'..plotType..'>\n')
   return self
end

function plyght:title(title)
   if not self:init() then
      return self
   end

   self:send('!!Title<'..title..'>\n')
   return self
end

function plyght:x_label(title)
   if not self:init() then
      return self
   end

   self:send('!!XTitle<'..title..'>\n')
   return self
end

function plyght:y_label(title)
   if not self:init() then
      return self
   end

   self:send('!!YTitle<'..title..'>\n')
   return self
end

function plyght:legend(location)
   if not self:init() then
      return self
   end

   if not location then location = '' end
   self:send('!!Legend<'..location..'>\n')
   return self
end

function plyght:print(file)
   if not self:init() then
      return self
   end

   self:send('!!Print<'..file..'>\n')
   return self
end

function plyght:x_range(min, max)
   if not self:init() then
      return self
   end

   self:send('!!XRange<'..min..','..max..'>\n')
   return self
end

function plyght:y_range(min, max)
   if not self:init() then
      return self
   end

   self:send('!!YRange<'..min..','..max..'>\n')
   return self
end



return plyght
