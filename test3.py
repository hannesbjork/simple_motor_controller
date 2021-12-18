# Import Module
import tkinter as tk
import time
from threading import Thread, Lock
from serial import Serial
import queue
  

class SerialInterface(tk.Tk):
    def __init__(self):
        super().__init__()
        
        
        self._send_queue = queue.Queue(maxsize = 15)
        
        self._rec_string = ""
        self._rec_mutex = Lock()
        
        self._serial = Serial('/dev/ttyUSB0') 
        
        self._act_speed = 0.0
        self._tar_speed = 0.0
        
        t1=Thread(target=self._send_loop)
        t1.start()
        
        t2=Thread(target=self._rec_loop)
        t2.start()
        
        
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
        
        
        
        
    def _send_loop(self):
        while True:
            msg = self._send_queue.get(block=True)
            self._serial.write(msg)
            time.sleep(0.05)
            
            
    def _check_speed_loop(self):
        while True:
            self.send_msg("getspd")
            self._rec_mutex.aquire()
            
            
            
            self._rec_mutex.release()
            time.sleep(1)
  
    def _rec_loop(self):
        while True:
            r = self._serial.read().decode('ascii')
            self._rec_string.append(r)
            
            time.sleep(0.05)
    
    def set_act_speed(self, speed):
        self.act_spd_var.set( str(speed) )
            
    def change_tar_speed(self, delta):
        self._tar_speed += delta
        self.tar_spd_var.set( str(self._tar_speed) )

    def send_msg(self, msg):
        self._send_queue.put(msg.encode('ascii'))
        
    def recieve_msg(self, msg):
        pass
        
    


root = SerialInterface()
root.mainloop()
