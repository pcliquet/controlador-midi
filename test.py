import sched
import time
from rtmidi.midiconstants import NOTE_ON, NOTE_OFF, POLY_AFTERTOUCH
import rtmidi
import mido

    
def schedule_note(scheduler, port, midi_no, time, duration, volume):
    print(f'Scheduling {midi_no} to be played at {time} for {duration} seconds.')
    #
    scheduler.enter(time, 4, port.send_message, argument=([NOTE_ON, midi_no, volume],))
    scheduler.enter(time + duration, 4, port.send_message, argument=([NOTE_OFF, midi_no, 0],))
    
print(mido.get_output_names())

output = mido.open_output("loopMIDI Port 3")
note_on = mido.Message('note_on', note=60, velocity=64)
output.send(note_on)

    
import sys

while True:
    output.send(note_on)
    if 'q' in input():
        break

output.close()
sys.exit()