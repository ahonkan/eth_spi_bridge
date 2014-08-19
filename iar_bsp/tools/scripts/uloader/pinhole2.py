"""
usage 'pinhole2 port comString'

Pinhole2 forwards traffic from/to the serial port 
to/from the specified port on the localhost. It also
starts up a Disconnect service on the localhost which
when connected to would abort the current pinhole2.

eg. pinhole2 2155 COM1:115200:8:None:1
    Forward any traffic coming into COM1 to localhost:2155
    Forward any traffic coming into localhost:2155 to COM1
"""

import sys
import logging
import serial
import os
from string import *
from serial import Serial
from serial import SerialException
from socket import *
from threading import Thread
import time

LOGGING = 1

def log( s ):
    if LOGGING:
        print '%s:%s' % ( time.ctime(), s )
        sys.stdout.flush()

#*************************************************************************
#* Check for serial port connection 
#*   - To check if the serial port is still open from a previous session, 
#*     we simply connect to a disconnect service started  by the previous 
#*     session. That disconnect service in-turn kills itself on receiving 
#*     this connection thereby freeing the serial port.
#*************************************************************************
def check_serial_connect(ip, port):
    try:
        s = socket( AF_INET, SOCK_STREAM )
    except error, msg:
        return False
    try:
        s.connect(( ip, int(port) ))
        sys.stdout.write("Connected to the disconnect service\n")
    except:
        s.close()
        return True
    s.close()
    return True
    
#***************************
#* Pinhole Serial Interface
#***************************
class Serial_out( Serial ):

    #***************************
    #* Constructor
    #***************************
    def __init__(self, *args, **kwargs):

        #ensure that a reasonable timeout is set
        timeout = kwargs.get('timeout',0.1)
        if timeout < 0.000001: timeout = 0.001
        kwargs['timeout'] = timeout
        Serial.__init__(self, *args, **kwargs)
        self.flushInput()
        self.buf = ''
        
#********************
#* Connect to Serial
#********************
class Serial_Connect():

    # Mapping of command-line serial values to pyserial values
    # NOTE: These parity values are all upper-case since CLI value will be made uppercase
    cliToPySerial = {
        # Data bitsize
        5 : serial.FIVEBITS,
        6 : serial.SIXBITS,
        7 : serial.SEVENBITS,
        8 : serial.EIGHTBITS,

        #Parity
        'N' : serial.PARITY_NONE,
        'NONE' : serial.PARITY_NONE,
        'E' : serial.PARITY_EVEN,
        'EVEN' : serial.PARITY_EVEN,
        'O' : serial.PARITY_ODD,
        'ODD' : serial.PARITY_ODD,

        #Stop bits
        1 : serial.STOPBITS_ONE,
        2 : serial.STOPBITS_TWO,
    }

    # Set serial read timeout value
    READ_TIMEOUT = 0.1

    #********************
    #* Constructor
    #********************
    def __init__(self, next=None):

        self.next = next
        self.serialPort = None

    #********************
    #* Entry
    #********************
    def execute(self, connString):

        sys.stdout.write("Attempting to open serial port\n")

        # extract the serial port info
        if len(connString.split(":")) != 5:
            sys.stderr.write("    ERROR: Serial parameters not accepted. Expecting: port:baudrate:databits:parity:stopbits")
            return False

        # Get port information - "<port>:<baud>:<data bits>:<parity>:<stop bits>"
        connList = connString.split(":")
        port = connList[0]
        rate = int(connList[1])
        data = int(connList[2])
        parity = connList[3].upper()
        stop = int(connList[4])

        try:
            # Try and open specified serial port
            self.serialPort = Serial_out(port, rate, self.cliToPySerial[data], self.cliToPySerial[parity], self.cliToPySerial[stop], timeout=self.READ_TIMEOUT)

        except SerialException, e:
            sys.stderr.write("    ERROR: Could not open serial port %r: %s\n" % (port, e))
            sys.stderr.write("           Please close any application that may be using this serial port")
            return False

        # Show serial port opened successfully
        sys.stdout.write("    SUCCESS: Opened serial port %r\n" % port)
        return True

class PipeThread( Thread ):
    pipes = []
    def __init__( self, source, sink, direction):
        Thread.__init__( self )
        self.source = source
        self.sink = sink
        # Set the direction of the data meaning whether the source is 
        # the Serial port of the localhost port. If direction os '0'
        # Source is localhost port else it is the Serial port.
        self.side = direction
        
        PipeThread.pipes.append( self )
        log( '%s pipes active' % len( PipeThread.pipes ))

    def run( self ):
        
        if (self.side == 0):
            self.source.setblocking(0)
            self.source.settimeout(3.0)

        while True: 
            try:
                if (self.side == 0):
                    data = self.source.recv(1024)
                    if data:
                        self.sink.write(data)
                    else:
                        break
                    
                elif (self.side == 1):
                    data = self.source.read(1024)
                    if data: 
                        self.sink.send( data )
                    else:
                        pass
                    
            except serial.SerialTimeoutException: 
                continue
            except serial.SerialException:
                break
            except timeout, msg:
                continue
            except Exception, e:
                log( '%s exception.' % (self) )    
                print e        
                break  
              
        log( '%s thread exited, so terminating the program.' % self )

        if (self.side == 0):
            self.source.close()
        elif (self.side == 1):
            self.sink.close()
            
        PipeThread.pipes.remove( self )
        
        # Exit the program altogether
        os._exit(0)
        
#*************************************************************************
#* Serial Disconnection service
#*   - This service can be run to check if the serial port is still open. 
#*     This service starts up on the localhost when the serial port 
#*     connection is initiated and is alive until somebody connects to it. 
#*     As soon as there is an incoming connection this service exists the 
#*     program thereby relinquishing the serial port.
#*************************************************************************
class Serial_Disconnect( Thread ):

    def __init__( self, port ):
        Thread.__init__( self )
        
        try:
            # Create another localhost tcp/ip connection to kill serial
            self.dsock = socket( AF_INET, SOCK_STREAM )
            self.dsock.bind(( '', port ))
            self.dsock.listen(0)
        except:
            # Exit the program altogether
            os._exit(0)
    def run( self ):
        self.dsock.setblocking(1)
        disconnect_sock = self.dsock.accept()
        sys.stdout.write("Somebody else connected so I am exiting\n")
        self.dsock.close()
        # Exit the program altogether
        os._exit(0)
        
class Pinhole2( Thread ):

    def __init__( self, port, connString ):
        Thread.__init__( self )
        log( 'Redirecting: Serial <--> localhost:%s' % ( port ))
        
        # Create the serial connection 
        self.com = Serial_Connect()
        self.com.execute(connString)
        
        # Create the localhost tcp/ip connection 
        self.sock = socket( AF_INET, SOCK_STREAM )
        self.sock.bind(( '', port ))
        self.sock.listen(5)
        
    
    def run( self ):
        self.sock.setblocking(1)
        try:
            newsock, address = self.sock.accept()
    
        except timeout:
            sys.stderr.write("Timed out waiting for connection.\n")
            self.sock.close()
            sys.exit(0)
            
        except e, Exception:
            print e

        # Setup localhost to serial port forwarding
        lhost = PipeThread( newsock, self.com.serialPort, 0)
        lhost.start()
        # Setup serial port to localhost forwarding
        ser = PipeThread( self.com.serialPort, newsock, 1)
        ser.start()
        
        lhost.join()
        ser.join() 
        sys.exit(0)
       
if __name__ == '__main__':


    import sys
    sys.stdout = open( sys.path[0] + "\\pinhole2.log", 'w' )
    try:
    
        sys.stdout.write("Starting Pinhole2\n")
        if len( sys.argv ) > 2:
            
            # Check to see if the serial port is open from the previous run
            chk = check_serial_connect("localhost", 2156)
            
            if chk == True:
                # Wait for the serial port to be available
                time.sleep(1)
                
                # Start the Serial Disconnect service
                disconn = Serial_Disconnect(2156)
                disconn.start()
                
                port = newport = int( sys.argv[1] )
                server = Pinhole2( port, sys.argv[2] )
                sys.stdout.write("connecting to localhost:" + str(port) + "\n")
    
                # Continuously forward connection to and from the serial port
                while 1:
                    server.run()
                    
                log( 'Server thread joined. Exiting pinhole server.' )
                sys.exit(0)
            else:
                sys.stderr.write("Problem encountered while releasing the serial port\r\n")
                    

    except KeyboardInterrupt, SystemExit:
        print "exception"
        pass
            
