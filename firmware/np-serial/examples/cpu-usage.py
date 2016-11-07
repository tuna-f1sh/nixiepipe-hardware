import sys
import time
import signal

import nixiepipe
import psutil

# catch ctrl-c to gracefully close
def sigint_handler(signum, frame):
    print 'Ctrl-C detected, closing port...'
    pipe.close()
    sys.exit()

signal.signal(signal.SIGINT, sigint_handler)

if len(sys.argv) > 1:
    pipe = nixiepipe.pipe(sys.argv[1])
else:
    pipe = nixiepipe.pipe()

pipe.setNumberUnits(1)
pipe.setColour(255,255,255)
pipe.setPipeColour(0,255,128,0)
pipe.setWeather(0,"Percent")

while (True):
    cpu = psutil.cpu_percent(interval=2)
    pipe.setNumber(cpu)
    pipe.show()
    time.sleep(2)
