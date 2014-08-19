#*****************************************
#* uloader
#*    Loads a raw binary image using 
#*    U-Boot and TFTP.
#*****************************************
import string
import serial
import sys
import tftpy
import logging
import argparse
import os
import thread
import re
import time
import threading
import subprocess
from Tkinter import *
from tkSimpleDialog import *
import tkMessageBox
from socket import *
from pinhole import Pinhole
from serial import Serial
from serial import SerialException

# Constants / Globals
UBOOT_CMD_CTRL_C = "\03"
UBOOT_CMD_PRINTENV = "printenv"
UBOOT_CMD_GO = "go %r\n"
UBOOT_PROMPT = "U-Boot"
UBOOT_CMD_CTRL_C_RESPONSE = "<INTERRUPT>"
UBOOT_CMD_PRINTENV_RESPONSE_FAIL = "## Error:"
UBOOT_CMD_RETRIES = 60
UBOOT_CMD_RESPONSE_RETRIES = 60
TFTP_DONE = "Bytes transferred ="
TFTP_DONE_RETRIES = 10
TFTP_TIMEOUT = "T"
TFTP_LINK_DOWN = "link down"
DEFAULT_TFTP_PORT = 69
DEBUG = False
READLINE_MAX_BUFFER = 4096
WAIT_FOR_IP_RETRIES = 60
GDB_SERVER_PORT = "2159"
USER_ABORT = 0
COMSTRING = ""

def center_screen(wnd):
    sw = wnd.winfo_screenwidth()
    sh = wnd.winfo_screenheight()
    xc = (sw - 500) / 2
    yc = (sh - 150) / 2
    wnd.geometry("+%d+%d" % (xc, yc))
    
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
#* Pop-up Dialog
#***************************
class ShowDialog(threading.Thread):
    
    #***************************
    #* Constructor
    #***************************
    def __init__(self, message):
        self.root=Tk()
        center_screen(self.root)

        self.msg = StringVar()
        self.msg.set(message)
        self.root.title('Power Cycle Target')
        l = Label(self.root, textvariable=self.msg, width=40, height=4, font=("Helvetica", 16))
        l.pack()
        b = Button(self.root, text="Cancel", height=2, width=10, command=self.userCancel)
        self.root.bind("<Return>", self.userCancel)
        self.root.bind("<Escape>", self.userCancel)
        b.pack()
        
        self.root.focus_set()
        self.root.resizable(0,0)
        self.root.protocol('WM_DELETE_WINDOW', self.userCancel)

        threading.Thread.__init__(self)


    #***************************
    #* Entry point
    #***************************
    def run(self):
        self.root.mainloop()
        
    #***************************
    #* User Abort (Cancel) 
    #***************************
    def userCancel(self, event=None):
        global USER_ABORT
        USER_ABORT = 1
        return

    #***************************
    #* Destroy the dialog
    #***************************
    def finish(self):
        self.root.quit()
        self.root.destroy()
        
#***************************
#* Phase Iterator
#***************************
class Phase():

    #***************************
    #* Constructor
    #***************************
    def  __init__(self, next=None):

        self.next = next

    #***************************
    #* Execute a phase
    #***************************
    def execute(self,context):
        if self.executePhase(context) == False:
            return False
        if self.next != None:
            if self.next.execute(context) == False:
                return False
        return True

    #***************************
    #* Override executePhase
    #***************************
    def executePhase(self,context):
        return True


#***************************
#* U-Boot Serial Interface
#***************************
class UBootSerial(Serial):

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

    #***************************
    #* Read Line from Serial
    #***************************
    """
    def readline(self, maxsize=READLINE_MAX_BUFFER, timeout=0.5):
        # timeout in seconds is the max time to wait for a complete line
        tries = 0
        while 1:
            self.buf += self.read(maxsize)
            pos = self.buf.find('\n')
            if pos >= 0:
                line, self.buf = self.buf[:pos+1], self.buf[pos+1:]
                return line
            tries += 1
            if tries * self.timeout > timeout:
                break
        line, self.buf = self.buf, ''
        return line
    """
    
    #***************************
    #* Check for the correct token
    #***************************
    def check_for_token(self, line, token, tries):
        global UBOOT_PROMPT

        temp_prompt = 0
        if (token == UBOOT_PROMPT):
            if (line.find(token) != 0):
                temp_prompt = line

        while ((line.find(token) != 0) and (tries)):
            line = self.readline()
            tries = tries - 1
     
            if ((line.find(token) != 0) or (line.find(temp_prompt) != 0)):
                break

        if tries:
            if ((token == UBOOT_PROMPT) and (temp_prompt != 0)):
                UBOOT_PROMPT = temp_prompt
            return True
        else:
            return False
     
    #***************************
    #* Wait for U-Boot prompt
    #***************************
    def wait_for_prompt(self):
        sys.stdout.write("Attempting to establish communication with U-Boot: ")
        
        # Change serial timeout
        cur_timeout = self.timeout
        self.timeout = 1.0
        
        ret = self.loop_for_prompt(1)
        if(ret != True):

            # Pop-up a dialog asking the user to reset the target if necessary
            warn = ShowDialog("Please power cycle the target and wait...\n\nClick 'Cancel' to Abort")
            warn.start()
            #ret = self.loop_for_prompt(UBOOT_CMD_RETRIES - 3)
            # Wait indefinitely for the U-Boot prompt
            ret = self.loop_for_prompt(0)
            warn.finish()

        # Restore serial timeout
        self.timeout = cur_timeout
            
        if ret != True:
            sys.stderr.write("\n    ERROR: Unable to establish communication with U-Boot. Expected response: %r\n" % UBOOT_CMD_CTRL_C_RESPONSE)
            return False

        sys.stdout.write("\n    SUCCESS: Established communication with U-Boot\n")
        return True

    #***************************
    #* Poll U-Boot for prompt
    #  - (tries = 0) indicates indefinite loop
    #***************************
    def loop_for_prompt(self, tries):
        global USER_ABORT
        indef_loop = 0
        
        if (tries == 0):
            # User requested a indefinite loop
            indef_loop = 1
            tries = 1
            
        while(tries and (USER_ABORT == 0)):
            
            # Issue 'CTRL-C' command - don't want to use just carriage return since this
            # executes last command with U-Boot
            self.write(UBOOT_CMD_CTRL_C)
            
            # Get line from U-Boot
            line = self.readline()
            
            # Check to see if response to command is found
            if(line.find(UBOOT_CMD_CTRL_C_RESPONSE) != -1):
                return True

            # Show progress....
            if (DEBUG):
                sys.stdout.write(line)
            # Once a pop-up dialog was added this progress is not required.
            #else:
            #    sys.stdout.write("T")
                
            if (indef_loop != 1):
                tries = tries - 1
            
        if (USER_ABORT == 1):
            sys.stderr.write("\n    ERROR: User Aborted the connection")

    #****************************
    #* Wait for U-Boot TFTP Done
    #****************************
    def wait_for_tftp_done(self):
        sys.stdout.write("Waiting for TFTP to complete: ")
        
        tries = TFTP_DONE_RETRIES
        
        # Skip all lines until "load address:" is found
        line = self.readline()
        if (self.check_for_token(line, "Load address:", UBOOT_CMD_RESPONSE_RETRIES)):
            while(tries):
    
                # Read line of input from U-Boot with a max of 2 second delay per line
                #line = self.readline(READLINE_MAX_BUFFER, 2)
                #time.sleep(2)
                line = self.readline() 
                
                # Check to see if U-Boot has finished TFTP transfer
                if (line.find(TFTP_DONE) != -1):
                    break
    
                # Check to see if link is down
                if (line.find(TFTP_LINK_DOWN) != -1):
                    sys.stderr.write("\n    ERROR: Link down")
                    tries = 0
                    break
    
                # Check to see if timeouts or no lines returned from U-Boot
                if ((line.find(TFTP_TIMEOUT) != -1) or (not line)):
                    tries = tries - 1
    
                    # Show "T" for when a tftp time-out is occurring in U-Boot or no response from U-Boot
                    if (DEBUG == False):
                        line = "T"
                else:
                    # Show "#" when successfully completing tftp transfer
                    if (DEBUG == False):
                        line = "#"
    
                # Output progress info
                sys.stdout.write(line)

        if tries == 0:
            sys.stderr.write("\n    ERROR: TFTP Transfer failed\n")
            return False

        sys.stdout.write("\n    SUCCESS: TFTP Transfer is complete\n")
        return True
    
    #****************************
    #* Extract IP address
    #****************************
    def extract_ip_addr(self, line):
        # Search the string for an IP match
        try:
            ip_match = re.search(r'((2[0-5]|1[0-9]|[0-9])?[0-9]\.){3}((2[0-5]|1[0-9]|[0-9])?[0-9])', line, re.I).group()
            ip_match_octets = ip_match.split('.')
        except:
            sys.stderr.write("\n    ERROR: IP Match Error\n")
            return None
        
        return ip_match, ip_match_octets[0], ip_match_octets[1], ip_match_octets[2], ip_match_octets[3]

            
    #****************************
    #* Find the host's address:
    #*  - Parses the output of 
    #*    windows ipconfig to 
    #*    find IP addresses and 
    #*    Subnet Masks
    #****************************
    def gethost_addr(self, tgt_ip_addr):

        find_sm = 0
        
        # Issue an "ipconfig" command on the host
        lines = os.popen("ipconfig -all", "r").readlines()
        for line in lines:
            
            # First, look for the string IPv4 Address
            if ( not(find_sm) and (((line.find("IP Address") != -1)) or ((line.find("IPv4 Address") != -1)))):
                host_ip_addr = self.extract_ip_addr(line)
                if (host_ip_addr != None):
                    if (self.is_valid_ipv4(host_ip_addr[0])):
                        # Now look for the matching Subnet Mask
                        find_sm = 1
                        
            # If and only if an IP address is found, look for the matching Subnet Mask
            elif ((find_sm) and (line.find("Subnet Mask") != -1)):
                host_subnet_mask = self.extract_ip_addr(line)
                if (host_subnet_mask != None):
                    if (self.is_valid_ipv4(host_subnet_mask[0])):
                        # Found the matching Subnet Mask
                        find_sm = 0
                        # Check if both the target and the host are in the same subnet.
                        if ((int(tgt_ip_addr[1]) & int(host_subnet_mask[1])) == ((int(host_ip_addr[1]) & int(host_subnet_mask[1])))):
                            if ((int(tgt_ip_addr[2]) & int(host_subnet_mask[2])) == ((int(host_ip_addr[2]) & int(host_subnet_mask[2])))):
                                if ((int(tgt_ip_addr[3]) & int(host_subnet_mask[3])) == ((int(host_ip_addr[3]) & int(host_subnet_mask[3])))):
                                    if ((int(tgt_ip_addr[4]) & int(host_subnet_mask[4])) == ((int(host_ip_addr[4]) & int(host_subnet_mask[4])))):
                                        sys.stdout.write("    SUCCESS: Found the TFTP Server\n")
                                        return host_ip_addr[0]
                        
        # Failed to find an IP address for the host that's on the same subnet as the target.                        
        sys.stderr.write("    ERROR: Failed to locate the TFTP Server\n")    
        sys.stderr.write("           Please ensure that both the host and the target are in the same subnet.\n")    
                    
    #****************************
    #* Set U-Boot Env Settings
    #****************************
    def set_uboot_env(self):
        
        sys.stdout.write("Setting U-Boot environment variables:\n")

        tries = UBOOT_CMD_RESPONSE_RETRIES
        
        line = self.readline()
        while ((line.find(UBOOT_CMD_CTRL_C_RESPONSE) != -1) and (tries)):
            line = self.readline()
            tries = tries - 1

        env_correct = False
        
        if ((self.check_for_token(line, UBOOT_PROMPT, UBOOT_CMD_RETRIES)) and tries):
            tries = UBOOT_CMD_RESPONSE_RETRIES
            
            self.write("setenv autoload no;dhcp\n")
            line = self.readline()
            while ((line.find("DHCP client bound to address") != 0) and (tries)):
                line = self.readline()
                tries = tries - 1
                
            if (tries):
                line = line.strip()
                tgt_ip_addr = self.extract_ip_addr(line)
                if (tgt_ip_addr != None):
                    sys.stdout.write("    SUCCESS: Obtained a DHCP address for the target\n")
                    
                    # Attempt to obtain the serverip from the target
                    svr_ip_addr = self.gethost_addr(tgt_ip_addr)
                    
                    if (svr_ip_addr != None):
                        if (self.set_svr_ip(svr_ip_addr) == True):
                            env_correct = True
                        else:
                            env_correct = False
                    else:
                        env_correct = False
                else:
                    sys.stderr.write("    ERROR: Failed to obtain a DHCP address for the target\n")
        
        if (env_correct == False):
            sys.stderr.write("    ERROR: Minimum required U-Boot environment parameters not set\n")
        else:
            sys.stdout.write("    SUCCESS: Minimum required U-Boot environment parameters set\n")
            
        return env_correct
    
    #****************************
    #* Check U-Boot Env Settings
    #****************************
    def check_uboot_env(self, cmd_list):
        sys.stdout.write("Checking U-Boot environment variables set:\n")

        env_correct = True
        while len(cmd_list) > 0:

            # Get command
            cmd = cmd_list.pop(0)

            # Issue printenv for ethaddr
            self.write(UBOOT_CMD_PRINTENV + " " + cmd + "\n")

            # Get 2 x lines from U-Boot
            line1 = self.readline()
            line2 = self.readline()

            # Check to see if response to command is found
            if((line2.find(UBOOT_CMD_PRINTENV_RESPONSE_FAIL) != -1) or (line2 == "")):
                sys.stderr.write("    %s:  FAILED\n" % cmd)
                if (DEBUG):
                    sys.stdout.write(line1)
                    sys.stdout.write(line2)
                env_correct = False
            else:
                sys.stdout.write("    %s:  PASSED\n" % cmd)
                if (DEBUG):
                    sys.stdout.write(line1)
                    sys.stdout.write(line2)

        if (env_correct == False):
            sys.stderr.write("    ERROR: Minimum required U-Boot environment parameters not set\n")
        else:
            sys.stdout.write("    SUCCESS: Minimum required U-Boot environment parameters set\n")
        return env_correct

    #****************************
    #* Set Server IP Address
    #****************************
                        
    def set_svr_ip(self, ip_addr):
        tries = UBOOT_CMD_RETRIES
        line = self.readline()
        if self.check_for_token(line, UBOOT_PROMPT, UBOOT_CMD_RETRIES):
            self.write("setenv serverip " + ip_addr + "\n")
            
        line = self.readline()
        return self.check_for_token(line, UBOOT_PROMPT, UBOOT_CMD_RETRIES)

    #***************************
    #* Wait for the IP address
    #***************************
    def wait_for_IP(self):
        tries = WAIT_FOR_IP_RETRIES
        while(tries):
            # Get line from U-Boot
            line = self.readline()
            while ((line.find("## Starting application") != 0) and (tries)):
                line = self.readline()
                tries = tries - 1
                
            #time.sleep(5)
            line = self.readline()

            tries = WAIT_FOR_IP_RETRIES
            while ((not line) and (tries)):
                line = self.readline()
                tries = tries - 1

            # Check to see if response to command is found
            line = line.strip()

            if ((self.is_valid_ipv4(line) != True) and (tries)):
                tries = tries -1
                # Show ">" for when waiting for an IP address from the target
                if (DEBUG == False):
                    line = ">"
            else:
                break
                
        if tries == 0:
            sys.stderr.write("\n    ERROR: Autodetect IP address failed\n")
            return None

        sys.stdout.write("\n    SUCCESS: Autodetected IP address: %s\n" % line)
        return line

    #***************************
    #* Issue U-Boot Go Command
    #***************************
    def issue_go_cmd(self, start_addr):
        line = self.readline()
        if self.check_for_token(line, UBOOT_PROMPT, UBOOT_CMD_RETRIES):
            # Issue go command to U-Boot
            sys.stdout.write("Issuing 'go' command to U-Boot\n")
            self.write(UBOOT_CMD_GO % start_addr)
        
            # Here we need not wait for a token to verify that the 
            # above command was successful because the next step/function
            # that gets executed is "wait_for_IP" which as its first step
            # looks for the correct token. Hence we can afford to just 
            # return from here.
    
            if (DEBUG == True):
                sys.stdout.write(UBOOT_CMD_GO % start_addr)
        else:
            sys.stderr.write("    ERROR: Failed to issue 'go' command")

        return

    #***************************
    #* Basic validation of in IPv4 address
    #***************************
    def is_valid_ipv4(self, ip):
        """Validates IPv4 addresses.
        """
        pattern = re.compile(r"""
            ^
            (?:
              # Dotted variants:
              (?:
                # Decimal 1-255 (no leading 0's)
                [3-9]\d?|2(?:5[0-5]|[0-4]?\d)?|1\d{0,2}
              |
                0x0*[0-9a-f]{1,2}  # Hexadecimal 0x0 - 0xFF (possible leading 0's)
              |
                0+[1-3]?[0-7]{0,2} # Octal 0 - 0377 (possible leading 0's)
              )
              (?:                  # Repeat 0-3 times, separated by a dot
                \.
                (?:
                  [3-9]\d?|2(?:5[0-5]|[0-4]?\d)?|1\d{0,2}
                |
                  0x0*[0-9a-f]{1,2}
                |
                  0+[1-3]?[0-7]{0,2}
                )
              ){0,3}
            |
              0x0*[0-9a-f]{1,8}    # Hexadecimal notation, 0x0 - 0xffffffff
            |
              0+[0-3]?[0-7]{0,10}  # Octal notation, 0 - 037777777777
            |
              # Decimal notation, 1-4294967295:
              429496729[0-5]|42949672[0-8]\d|4294967[01]\d\d|429496[0-6]\d{3}|
              42949[0-5]\d{4}|4294[0-8]\d{5}|429[0-3]\d{6}|42[0-8]\d{7}|
              4[01]\d{8}|[1-3]\d{0,9}|[4-9]\d{0,8}
            )
            $
        """, re.VERBOSE | re.IGNORECASE)
        return pattern.match(ip) is not None


#********************
#* Connect to U-Boot
#********************
class UBootConnectPhase(Phase):

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
    def executePhase(self,context):
        global COMSTRING

        sys.stdout.write("Attempting to open serial port\n")
        # Check to see if the serial port is open from the previous run
        chk = check_serial_connect("localhost", 2156)
                
        if chk == False:
            sys.stderr.write("    ERROR: Failed to open Serial Port\n")
            sys.exit(2)

        # extract the serial port info
        connString = context["serial"]
        COMSTRING = connString
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
            time.sleep(1)
            # Try and open specified serial port
            self.serialPort = context["serialObject"] = UBootSerial(port, rate, self.cliToPySerial[data], self.cliToPySerial[parity], self.cliToPySerial[stop], timeout=self.READ_TIMEOUT)

        except SerialException, e:
            sys.stderr.write("    ERROR: Could not open serial port %r: %s\n" % (port, e))
            sys.stderr.write("           Please close any application that may be using this serial port")
            return False

        # Show serial port opened successfully
        sys.stdout.write("    SUCCESS: Opened serial port %r\n" % port)

        # Try connecting with U-Boot
        if (self.serialPort.wait_for_prompt() == True):

            # Check to see if appropriate U-Boot environment variables are set
            #if (self.serialPort.check_uboot_env(["ethaddr", "ipaddr", "serverip"]) == True):
            if (self.serialPort.set_uboot_env() == True):

                return True

        # If it makes it here, U-Boot connection failed or environment variables not set correctly
        return False


#********************
#* Loal Application
#********************
class UBootLoadApplicationPhase(Phase):

    #********************
    #* Constructor
    #********************
    def __init__(self, next=None):

        self.next = next
        self.serialPort = None
        self.tftpServer = None

    #********************
    #* Entry point
    #********************
    def executePhase(self,context):

        # Get serial port
        self.serialPort = context["serialObject"]

        # Check if there is an existing TFTP server running on localhost by
        # attempting to bind a socket to the default TFTP port.  If the port is
        # already bound an exception will occur, indicating the presense of an
        # exsiting TFTP server.
        tftp_check_s = socket(AF_INET, SOCK_DGRAM)

        try:
            tftp_check_s.bind(('localhost', DEFAULT_TFTP_PORT))

        except error, msg:
            tftp_check_s.close()
            sys.stderr.write("    ERROR: Existing TFTP Server detected on localhost\n" % msg)
            sys.exit(2)

        # Create TFTP Server object
        self.tftpServer = tftpy.TftpServer(context["serverdir"])

        # Turn off logging if debugging disabled
        if (DEBUG == False):
            tftpy.setLogLevel(logging.CRITICAL)

        # Start new thread to send TFTP command to U-Boot
        if (DEBUG == True):
            sys.stdout.write("Starting TFTP Server - listening on port %r for all interfaces\n" % DEFAULT_TFTP_PORT)
        thread.start_new_thread(self.start_tftpserver, (context,))
        
        # Issue tftp command
        if (DEBUG == True):
            sys.stdout.write("Issuing 'tftp' command to U-Boot\n")

        tftpCommand = "tftp %r %r\n" % (context["load_address"],context["application"])
        
        # No need to wait for the prompt here because the prompt 
        # would have been consumed by the previous command.
        self.serialPort.write(tftpCommand)
        
        # Wait for tftp connection and transfer to complete
        if (self.serialPort.wait_for_tftp_done()):
            
            # Return true if U-Boot prompt returns / false if not
            return self.serialPort.wait_for_prompt()

        # TFTP done not returned - return false for failure
        return False

    #*********************
    #* TFTP Server Thread
    #*********************
    def start_tftpserver(self,context):

        try:
            # Listen on port DEFAULT_TFTP_PORT for all interfaces
            self.tftpServer.listen('0.0.0.0', DEFAULT_TFTP_PORT, TFTP_DONE_RETRIES)

        except TftpException, err:
            sys.stderr.write("    ERROR: TFTP Server Exception\n")
            sys.exit(2)

        except KeyboardInterrupt:
            pass

#********************
#* Start Application
#********************
class UBootStartApplicationPhase(Phase):
    def __init__(self, next=None):
        '''
        Constructor
        '''
        self.next = next
        self.serialPort = None

    def executePhase(self,context):

        # Ensure start address is not 0xFFFFFFFF (skip issuing go cmd if it is)
        if (context["start_address"] != '0xFFFFFFFF'):

            # Get serial port
            self.serialPort = context["serialObject"]

            # Issue go command
            self.serialPort.issue_go_cmd(context["start_address"])

        return True

#********************
#* Autodetect IP
#********************
class UBootAutoDetectIPPhase(Phase):
    def __init__(self, next=None):
        '''
        Constructor
        '''
        self.next = next
        self.serialPort = None

    #***************************
    #* Verify server on IP 
    #***************************
    def verify_server(self, ip, port):
        tries = 3
        time.sleep(3)
        while(tries):
            try:
                s = socket( AF_INET, SOCK_STREAM )
            except error, msg:
                sys.stderr.write("    ERROR: Could not create socket: %s\n" % (msg))
                return False
            try:
                s.connect(( ip, int(port) ))
            except error, msg:
                sys.stderr.write("    WARNING: Could not connect to server: %s, %s\n" % (ip,msg))
                s.close()
                tries = tries - 1
                continue
            s.close()
            return True
        return False
        
    def executePhase(self,context):
        # Get serial port
        self.serialPort = context["serialObject"]

        ip = self.serialPort.wait_for_IP()
        if ip is None:
            sys.stderr.write("    ERROR: Timeout waiting for IP address advertisement from target.\n")
            context["gdb_target_ip"] = None
            context["gdb_target_port"] = None
        elif self.verify_server(ip, GDB_SERVER_PORT):
            context["gdb_target_ip"] = ip.strip()
            context["gdb_target_port"] = GDB_SERVER_PORT
            sys.stderr.write("    SUCCESS: Found server at: %s\n" % (ip))
            return True
        return False

#*****************
#* Main
#*****************
if __name__ == "__main__":

    # Make standard-out / standard error unbuffered
    sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)
    sys.stderr = os.fdopen(sys.stderr.fileno(), 'w', 0)

    params = {}

    parser = argparse.ArgumentParser(prog="uloader", usage="%(prog)s serial <port:baud_rate:data_bits:parity:stop_bits> app <binary app filename> s <hex start address> l <hex load address> [proxy <target_ip> <target_port> <protocol>]", add_help=True)
    parser.add_argument('serial', nargs=2, help='com<N>:baud_rate:data_bits:parity:stop_bits')
    parser.add_argument('app', nargs=2, help='application file name')
    parser.add_argument('s', nargs=2, help='application start address')
    parser.add_argument('l', nargs=2, help='application load address')
    parser.add_argument('--proxy', nargs=3, help='target_ip_address target_gdb_port protocol(TCP,UDP)')    
    args = parser.parse_args()

    # Ensure the correct arguments are passed into the application
    if (args.serial != None and args.app[0] == "app" and args.s[0] == "s" and args.l[0] == "l"):

        # Ensure application actually exists
        if (os.path.isfile(args.app[1])):

            # Get serial info
            params['serial'] = args.serial[1]

            # Get server directory and application name from passed in value
            params["serverdir"] = os.path.dirname(args.app[1])
            params["application"] = os.path.basename(args.app[1])

            # Get image start address and load address
            params["start_address"] = args.s[1]
            params["load_address"] = args.l[1]
            params["proxy"] = False # assume proxy is not on, we'll turn it on only if all conditions are met
            if (args.proxy != None):
                # pick up the parameters for the proxy
                params["socket_address"] = args.proxy[0]
                params["socket_port"] = args.proxy[1]
                params["socket_protocol"] = args.proxy[2]
                # determine if we will actually use the proxy
                if (params["socket_protocol"].upper() == "TCP") :
                    if (params["socket_address"].upper() == "LOCALHOST" or params["socket_address"] == "127.0.0.1") :
                        params["proxy"] = True
        else:
            sys.stderr.write("ERROR: Application file not found\n")
            sys.exit(2)

    else:
        parser.print_help()
        sys.exit(2)

    try:
        # Build linked-list of phases to run going from last phase to first
        
        if params["proxy"] == True:
            # only do IP detection when proxy is set
            ip = UBootAutoDetectIPPhase()
            go = UBootStartApplicationPhase(ip)
        else:
            go = UBootStartApplicationPhase()
        load = UBootLoadApplicationPhase(go)
        connect = UBootConnectPhase(load)

        sys.stdout.write("\n")

        # Start executing the phases
        if (connect.execute(params)):
            if (params["start_address"] != '0xFFFFFFFF'):
                sys.stdout.write("    SUCCESS: Image successfully loaded and executed on target\n")
                if (params["proxy"] == True):
                    subprocess.Popen(sys.path[0] + "\\pinhole2.bat 2155 " + COMSTRING, shell=True)
                    time.sleep(1)
                    subprocess.Popen(sys.path[0] + "\\..\\..\\bin\utils\\puttytel -load \"Nucleus_serial\"", shell=True)
                    os.execl(sys.path[0] + "\\pinhole.bat", sys.path[0] + "\\pinhole.bat", params["gdb_target_port"], params["gdb_target_ip"])
            else:
                sys.stdout.write("    SUCCESS: Image successfully loaded on target\n")
            sys.exit(0)
        else:
            sys.stderr.write("    ERROR: Image not loaded / executed on target\n")
            sys.stderr.write("    ERROR: Please cycle power to the target and try again.\n")
            sys.stderr.write("            OR\n")
            sys.stderr.write("           If you have already reset the target once, just try loading again.\n")
            sys.exit(2)

    except KeyboardInterrupt, SystemExit:
        pass

