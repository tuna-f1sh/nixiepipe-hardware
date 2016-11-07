import sys
import time
import signal
import nixiepipe
import threading
import pyowm

owm = pyowm.OWM('b58b4a4f5a654884d357ffa44443dac3')  # You MUST provide a valid API key
global w

class RepeatedTimer(object):
    def __init__(self, interval, function, *args, **kwargs):
        self._timer     = None
        self.interval   = interval
        self.function   = function
        self.args       = args
        self.kwargs     = kwargs
        self.is_running = False
        self.start()

    def _run(self):
        self.is_running = False
        self.start()
        self.function(*self.args, **self.kwargs)

    def start(self):
        if not self.is_running:
            self._timer = threading.Timer(self.interval, self._run)
            self._timer.start()
            self.is_running = True

    def stop(self):
        self._timer.cancel()
        self.is_running = False

# catch ctrl-c to gracefully close
def sigint_handler(signum, frame):
    print 'Ctrl-C detected, closing port...'
    pipes.close()
    weatherTask.stop()
    sys.exit()

def updateweather():
    print 'Weather update'
    # Search for current weather in London (UK)
    observation = owm.weather_at_place('Bristol,uk')
    global w
    w = observation.get_weather()

signal.signal(signal.SIGINT, sigint_handler)

if len(sys.argv) > 1:
    pipes = nixiepipe.pipe(sys.argv[1])
else:
    pipes = nixiepipe.pipe()

# Grab weather on first run and setup timer task to update
updateweather()
weatherTask = RepeatedTimer(60*30, updateweather)

# Set weather unit pipe
pipes.setNumberUnits(1)
pipes.setPipeColour(0,255,128,0)

while (True):
    print w.get_detailed_status()
    pipes.setWeather(0,w.get_weather_icon_name())
    pipes.setNumber(w.get_temperature('celsius')['temp'])
    pipes.show()
    time.sleep(5)

    pipes.setWeather(0,"Percent")
    pipes.setNumber(w.get_humidity())
    pipes.show()
    time.sleep(5)

    pipes.setWeather(0,"Wind")
    pipes.setNumber(w.get_wind()['speed'])
    pipes.show()
    time.sleep(5)
