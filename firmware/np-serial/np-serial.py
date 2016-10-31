import sys
import time
import signal
import serial
import glob
from collections import defaultdict

# TODO
# * parse reply function
# * auto find FTDI chip

# catch ctrl-c to gracefully close
def sigint_handler(signum, frame):
    print 'Ctrl-C detected, closing port...'
    pipe.close()
    sys.exit()

class NixiePipe:
    # Nixie Pipe command dictionary
    _commands = {
            "setNumber" : 0x40,
            "setPipeNumber" : 0x41,
            "setColour" : 0x42,
            "setPipeColour" : 0x43,
            "brightness" : 0x44,
            "clear" : 0x45,
            "clearPipe" : 0x46,
            "getNumber" : 0x47,
            "connect" : 0x48,
            "setNumberUnits" : 0x49,
            "show" : 0x50,
    }
    _units = {
            "Volts" : 0,
            "Amps" : 1,
            "Watts" : 2,
            "Grams" : 3,
            "Hertz": 4,
            "Celsius" : 5,
            "Newtons" : 6,
            "Meters" : 7,
            "Seconds" : 8,
            "Ohm" : 9,
    }
    _prefix = {
            "Pico" : 0,
            "Nano" : 1,
            "Micro" : 2,
            "Milli" : 3,
            "Kila": 4,
            "Mega" : 5,
            "Giga" : 6,
            "Tera" : 7,
            "Neg" : 8,
            "Pos" : 9,
    }
    _weather = {
            "Sun" : 0,
            "Rain" : 1,
            "Cloud" : 2,
            "SunCloud" : 3,
            "Snow" : 4,
            "Wind" : 5,
            "Storm" : 6,
            "Fog" : 7,
            "Pascal" : 8,
            "Percent" : 9,
    }

    # set the default as int (0) for error
    _commands = defaultdict(int,**_commands)
    _units = defaultdict(int,**_units)
    _prefix = defaultdict(int,**_prefix)
    _weather = defaultdict(int,**_weather)

    def __init__(self,port):
        # Create serial object
        self.ser = serial.Serial(port,57600,timeout=10)
        # Wait for boot
        time.sleep(3)

        # Flush serial in case of junk
        self.ser.flush()
        # Send connect
        version = self.connect()
        print version
        if version >= 0:
            print 'Connected to Nixie Pipe version ' + str(version)
        else:
            self.close()
            raise RuntimeError('Could not connect to Nixie Pipe!')

    def close(self):
        self.ser.close()

    def _sendCommand(self, command, message, size):
        # packet is size of message plus two bytes for header
        packet = [0] * (size + 2)
        # size is first byte
        packet[0] = size & 0xFF
        # command is second byte
        packet[1] = command & 0xFF
        # append the message
        if size > 1:
            packet[2:2+size] = message
        else:
            packet[2] = message

        # write packet
        self.ser.write(packet)

    def _valueToMessage(self,value):
        message = [0] * 4
        message[0] = value & 0xFF
        message[1] = (value >> 8) & 0xFF
        message[2] = (value >> 16) & 0xFF
        message[3] = (value >> 24) & 0xFF

        return message

    def connect(self):
        self._sendCommand(self._commands["connect"],[0x4E, 0x50],2)
        res = self.ser.read(4)
        res = list(bytearray(res))
        print res

        version = -1
        if (res[1] == self._commands["connect"]):
            major = res[3]
            minor = res[2]
            version = str(major) + '.' + str(minor)

        return float(version)

    def setNumber(self,value):
        self._sendCommand(self._commands["setNumber"],self._valueToMessage(value),4)

    def setPipeNumber(self,pipe,value):
        self._sendCommand(self._commands["setPipeNumber"],[pipe, value],4)

    def setColour(self,r,g,b):
        # sanity check arguments are bytes
        r &= 0xFF
        g &= 0xFF
        b &= 0xFF
        self._sendCommand(self._commands["setColour"],[r, g, b],3)

    def setPipeColour(self,pipe,r,g,b):
        # sanity check arguments are bytes
        r &= 0xFF
        g &= 0xFF
        b &= 0xFF
        self._sendCommand(self._commands["setPipeColour"],[pipe,r, g, b],4)

    def clear(self):
        self._sendCommand(self._commands["clear"],1,1)
    
    def clearPipe(self,pipe):
        self._sendCommand(self._commands["clearPipe"],pipe & 0xFF,1)

    def setBrightness(self, value):
        self._sendCommand(self._commands["brightness"],value & 0xFF,1)

    def setNumberUnits(self, value):
        self._sendCommand(self._commands["setNumberUnits"],value & 0xFF,1)

    def show(self)
        self._sendCommand(self._commands["show"],1,1]
        # give time for FastLED during which interrupts disabled and so serial will be ignored
        time.sleep(0.01)

signal.signal(signal.SIGINT, sigint_handler)

if len(sys.argv) > 1:
    pipe = NixiePipe(sys.argv[1])
else:
    pipe = NixiePipe('/dev/tty.usbmodem1421')

# pipe.setColour(0,255,255)
pipe.setPipeColour(0,255,128,0)
pipe.setPipeColour(1,255,128,0)
pipe.setPipeNumber(1,4)
pipe.show()

for num in range(0,9999):
    pipe.setNumber(num)
    pipe.show()
    time.sleep(0.1)

