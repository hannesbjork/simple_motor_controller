# Import Module
import tkinter as tk
import time
from threading import Thread, Lock
from serial import Serial, SerialException
import queue
from math import log10, floor
  

class SerialInterface(tk.Tk):
    def __init__(self):
        super().__init__()
        
        
        self._send_queue = queue.Queue(maxsize = 50)
        
        self._rec_queue = queue.Queue(maxsize = 50)

        self._rec_mutex = Lock()
        
        try:
            self._serial = Serial('/dev/ttyUSB0') 
        except SerialException as e:
            print(e)
            print("Using dummy.")
            self._serial = DummySerial()

        self._act_speed = 0.0
        self._tar_speed = 0.0
        
        tk.Button(self,text="Increase speed",command = lambda: self.change_tar_speed(2)).grid(row=1,column=1)
        tk.Button(self,text="Decrease speed",command = lambda: self.change_tar_speed(-2)).grid(row=1,column=2)
  
  
        self.act_spd_var = tk.StringVar()
        self.act_spd_var.set('0.0')
        tk.Label(self, text="Actual speed").grid(row=2,column=1, columnspan=2)
        self.targ_spd_label = tk.Label(self, textvariable=self.act_spd_var)
        self.targ_spd_label.grid(row=3,column=1, columnspan=2)
  
        self.tar_spd_var = tk.StringVar()
        self.tar_spd_var.set('0.0')
        tk.Label(self, text="Target speed").grid(row=4,column=1, columnspan=2)
        self.targ_spd_label = tk.Label(self, textvariable=self.tar_spd_var)
        self.targ_spd_label.grid(row=5,column=1, columnspan=2)
        
        t1=Thread(target=self._send_loop)
        t1.start()
        
        t2=Thread(target=self._rec_loop)
        t2.start()

        t3=Thread(target=self._check_speed_loop)
        t3.start()
        
        
        
        
    def _send_loop(self):
        while True:
            msg :str = self._send_queue.get(block=True)
            
            self._serial.write(msg.encode('ascii'))
            
            
            time.sleep(0.05)

    def _rec_loop(self):
        while True:
            msg = self._serial.read(6).decode('ascii')
            if msg:
                self._rec_queue.put(msg)
            
            time.sleep(0.05)
            
            
    def _check_speed_loop(self):
        while True:
            
            self.send_msg("getspd")
            msg = self.recieve_msg()

            try:
                spd = float(msg)
                self.set_act_speed( spd )
            except ValueError:
                print("Not vaild msg.")
            
            time.sleep(1)
    
    def set_act_speed(self, speed):
        self.act_spd_var.set( str(speed) )
            
    def change_tar_speed(self, delta):
        self._tar_speed += delta
        self.tar_spd_var.set( str(self._tar_speed) )

        self.send_msg("setspd")

        spd = round_sig(self._tar_speed, 5)
        self.send_msg(str(spd))

    def send_msg(self, msg):
        print("sent", msg)
        self._send_queue.put(msg)
        
    def recieve_msg(self):
        msg = self._rec_queue.get(block=True)
        print("rec", msg)
        return msg


class DummySerial():
    def __init__(self):
        self._queue = queue.Queue(maxsize = 50)
        self._spd = 0
        self._tar_spd = 0

        self._getspd_bool = False


        t1=Thread(target=self._move_towards_target)
        t1.start()

    def write(self, msg):
        msg :str = msg.decode('ascii')

        if self._getspd_bool:
            try:
                spd = float(msg)
                self._tar_spd = spd
            except ValueError:
                print("Not vaild msg.")

            self._getspd_bool = False

        if msg == "getspd":
            spd = str(round_sig(self._spd, 6))
            self._queue.put(spd.encode('ascii'))

        elif msg == "setspd":
            self._getspd_bool = True
            

        else:
            self._queue.put(msg.encode('ascii'))


    def read(self, n=1):
        msg = self._queue.get()
        return msg

    def _move_towards_target(self):
        while True:
            print(list(self._queue))
            self._spd += (self._tar_spd - self._spd)/2
            time.sleep(0.5)

def round_sig(x, sig=2):
    if x == 0:
        return 0
    return round(x, sig-int(floor(log10(abs(x))))-1)
        

if __name__ == "__main__":
    
    root = SerialInterface()
    root.mainloop()
