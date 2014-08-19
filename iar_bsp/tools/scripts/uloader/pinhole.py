"""
usage 'pinhole port host [newport]'

Pinhole forwards the port to the host specified.
The optional newport parameter may be used to
redirect to a different port.

This is a one-shot server. Once the incoming connection
shutdowns, the remaining pipes are shutdown. 

eg. pinhole 80 webserver
    Forward all incoming WWW sessions to webserver.

    pinhole 23 localhost 2323
    Forward all telnet sessions to port 2323 on localhost.
"""

import sys
from socket import *
from threading import Thread
import time

LOGGING = 1
EXITING = 0
HOSTCONNECT = 60.0 # global used to prevent the server
                   # from hanging around when a client never connects
                   # this can occur if EDGE cancels after the image is loaded
                   # but before EDGE connects to the GDB server

#*************************************************************************
#* Check for serial port connection 
#*   - To check if the serial port is still open from a previous session, 
#*     we simply connect to a disconnect service started  by the previous 
#*     session. That disconnect service in-turn kills itself on receiving 
#*     this connection thereby freeing the serial port.
#*************************************************************************
def release_serial_connect(ip, port):
    try:
        s = socket( AF_INET, SOCK_STREAM )
    except error, msg:
        return False
    try:
        s.connect(( ip, int(port) ))
        log('Connected to the disconnect service')
    except:
        s.close()
        return True
    s.close()
    return True
    
def log( s ):
    if LOGGING:
        print '%s:%s' % ( time.ctime(), s )
        sys.stdout.flush()

class PipeThread( Thread ):
    pipes = []
    def __init__( self, source, sink):
        Thread.__init__( self )
        self.source = source
        self.sink = sink
        
        
        log( 'Creating new pipe thread  %s ( %s -> %s )' % \
            ( self, source.getpeername(), sink.getpeername() ))
        PipeThread.pipes.append( self )
        log( '%s pipes active' % len( PipeThread.pipes ))

    def run( self ):
        global EXITING
        self.source.setblocking(0)
        self.source.settimeout(3.0)

        while EXITING == 0:
            try:
                data = self.source.recv( 1024 )
                if data: 
                    self.sink.send( data )
                else:
                    break    
            except timeout, msg:
                continue
            except:
                log( '%s exception.' % (self) )            
                break    
        log( '%s terminating' % self )
        self.source.close()
        self.sink.close()
        PipeThread.pipes.remove( self )
        log( '%s pipes active' % len( PipeThread.pipes ))
        log( 'Exiting thread')
        EXITING = 1
        sys.exit(0)
        
        
class Pinhole( Thread ):

    def __init__( self, port, newhost, newport ):
        Thread.__init__( self )
        log( 'Redirecting: localhost:%s -> %s:%s' % ( port, newhost, newport ))
        self.newhost = newhost
        self.newport = newport
        self.sock = socket( AF_INET, SOCK_STREAM )
        self.sock.bind(( '', port ))
        self.sock.listen(5)
    
    def run( self ):
        global HOSTCONNECT
        
        self.sock.setblocking(0)
        self.sock.settimeout(HOSTCONNECT)
        try:
            newsock, address = self.sock.accept()
        except timeout:
            log( 'Timed out waiting for EDGE to connect. Modify HOSTCONNECT if the current timeout period is too short.' )
            self.sock.close()
            sys.exit(0)
        log( 'Creating new session for %s %s ' % address )
        fwd = socket( AF_INET, SOCK_STREAM )
        fwd.connect(( self.newhost, self.newport ))
        host = PipeThread( newsock, fwd)
        host.start()
        target = PipeThread( fwd, newsock)
        target.start()
        host.join()
        target.join() 
        sys.exit(0)
       
if __name__ == '__main__':

    #print 'Starting Pinhole'

    import sys
    sys.stdout = open( sys.path[0] + "\\pinhole.log", 'w' )
    try:
    
        if len( sys.argv ) > 1:
            port = newport = int( sys.argv[1] )
            newhost = sys.argv[2]
            if len( sys.argv ) == 4: newport = int( sys.argv[3] )
            server = Pinhole( port, newhost, newport )
            server.start()
            server.join()
            log( 'Server thread joined. Exiting pinhole server.' )
            release_serial_connect("localhost", 2156)

            sys.exit(0)
    except KeyboardInterrupt, SystemExit:
        pass
            
