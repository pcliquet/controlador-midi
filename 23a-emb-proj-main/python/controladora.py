import pyautogui
import serial
import argparse
import time
import logging
import time
import rtmidi
import mido
import sys


#[NAO PERDER ESSE LINK]
# https://spotlightkid.github.io/python-rtmidi/rtmidi.html#module-rtmidi.midiutil


class MyControllerMap:
    def __init__(self):
        self.button = {'A': 'L'} # Fast forward (10 seg) pro Youtube

class SerialControllerInterface:
    # Protocolo
    # byte 1 -> Botão 1 (estado - Apertado 1 ou não 0)
    # byte 2 -> EOP - End of Packet -> valor reservado 'X'

    def __init__(self, port, baudrate, midiout, midiIn):

        self.ser = serial.Serial(port, baudrate=baudrate)
        self.mapping = MyControllerMap()
        self.incoming = '0'
        self.note_on = [mido.Message('note_on', note=60, velocity=64), [0x90, 60, 112]]
        self.note_off = 0
        self.data_conf = ['0',[0x80, 60, 0]]
        self.after_touch = [0xA0, 60, 100]
        self.last_mode = ['note_on', 60, 100]
        self.available_ports = midiout.get_ports()
        self.output = mido.open_output("loopMIDI Port 3")
        self.i = 0
        
        
        pyautogui.PAUSE = 0  ## remove delay

    def make_note(self, nota, estado, velocity):
        note = [estado, nota, velocity] 
        self.output.send(mido.Message(estado, note=nota, velocity=velocity))
        return note
    
    def update(self,midiout):
        ## Sync protocol
        #print("update")
        
       
        while self.incoming != b'X' :
            # if self.data_conf[0] == b'1':
            #     self.estado(midiout=midiout,note=self.data_conf[1])
            self.incoming = self.ser.read()
            logging.debug("Received INCOMING: {}".format(self.incoming))
            # print("lendo")


        #LEITURA DE DATA
        data = self.ser.read(size=8)
        #print(data[0])

        
        logging.debug("Received DATA: {}".format(data))
    

        #Compara o estado com o anterior
        if self.data_conf[0] == data:
            if self.data_conf[1] == self.last_mode:
                #print(self.after_touch)
                
                print("entrou")
            #print("legas")
            else:
                pass

        #Recebe a mensagem do botão pressionado

        #Dó
        elif data[0] == 49:
            print("Nota apertada")
            note = self.make_note(60,'note_on',100)
            self.data_conf[1] = note
            self.note_off = 60
            
            
        
        #Ré
        elif data[1] == 49:
            print("Nota apertada")
            note = self.make_note(63,'note_on',100)
            self.data_conf[1] = note
            self.note_off = 63
        
        #Mi
        elif data[2] == 49:
            print("Nota apertada")
            note = self.make_note(64,'note_on',100)
            self.data_conf[1] = note
            self.note_off = 64

        #Fa
        elif data[3] == 49:
            print("Nota apertada")
            note = self.make_note(65,'note_on',100)
            self.data_conf[1] = note
            self.note_off = 65

        #Sol
        elif data[4] == 49:
            print("Nota apertada")
            note = self.make_note(66,'note_on',100)
            self.data_conf[1] = note
            self.note_off = 66
                
        #Lá
        elif data[5] == 49:
            print("Nota apertada")
            note = self.make_note(67,'note_on',100)
            self.data_conf[1] = note
            self.note_off = 67

        #Si
        elif data[6] == 49:
            print("Nota apertada")
            note = self.make_note(69,'note_on',100)
            self.data_conf[1] = note
            self.note_off = 69
        
        #Rece a mensagem de botão solto
        else:
            print("Soltou")
            note = self.make_note(self.note_off,'note_off',0)
            self.data_conf[1] = note
                
            
        #Salva no estado anterior
        self.incoming = self.ser.read()
        self.data_conf[0] = data


        
        

class DummyControllerInterface:
    def __init__(self):
        self.mapping = MyControllerMap()

    def update(self):
        pyautogui.keyDown(self.mapping.button['A'])
        time.sleep(0.1)
        pyautogui.keyUp(self.mapping.button['A'])
        logging.info("[Dummy] Pressed A button")
        time.sleep(1)


if __name__ == '__main__':
    
    interfaces = ['dummy', 'serial']

   
    argparse = argparse.ArgumentParser()
    argparse.add_argument('-b', '--baudrate', type=int, default=115200)
    argparse.add_argument('-c', '--controller_interface', type=str, default='serial', choices=interfaces)
    argparse.add_argument('-d', '--debug', default=False, action='store_true')
    args = argparse.parse_args()
    if args.debug:
        logging.basicConfig(level=logging.DEBUG)

    
    
    
    print("Connection to {} using {} interface ({})".format("com3", args.controller_interface, args.baudrate))
    

    if args.controller_interface == 'dummy':
        controller = DummyControllerInterface()
    else:
        midiout = rtmidi.MidiOut()
        midiin = rtmidi.MidiIn()
        controller = SerialControllerInterface(port="COM3", baudrate=args.baudrate, midiout=midiout, midiIn = midiin)

    while True:
        controller.update(midiout)
    self.output.close()
    sys.exit()
