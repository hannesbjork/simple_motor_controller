# Import Module
import tkinter as tk
import time
from threading import Thread, Lock
from typing import BinaryIO
from serial import Serial, SerialException
import queue
from math import log10, floor
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import (FigureCanvasTkAgg, 
NavigationToolbar2Tk)
from matplotlib import pyplot as plt
import struct


from matplotlib.figure import Figure 
  

class SerialInterface(tk.Tk):
    def __init__(self):
        super().__init__()
        
        
        self._send_queue = queue.Queue(maxsize = 50)
        
        self._rec_queue = queue.Queue(maxsize = 50)

        self._mutex = Lock()
        
        try:
            self._serial = Serial('/dev/ttyUSB0', 4800, timeout=5) 
        except SerialException as e:
            print(e)
            print("Using dummy.")
            self._serial = DummySerial()

        self._act_speed = 0.0
        self._tar_speed = 0.0
        
        self.entry_var = tk.StringVar()
        entry = tk.Entry(self, textvariable=self.entry_var)
        entry.grid(row=1,column=1)
        entry.bind('<Return>', self.set_tar_speed)
        tk.Button(self,text="Set speed",command = self.set_tar_speed).grid(row=1,column=2)
  
  
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




        self._plot_mutex = Lock()
        x = [x for x in range(15)]

        self.y_ref = [0 for _ in range(15)]
        self.y_act = [0 for _ in range(15)]

        plt.ion()

        fig = Figure() 
        ax = fig.add_subplot(111)
        
        self.line_act, = ax.plot(x, self.y_act, 'r-')
        self.line_ref, = ax.plot(x, self.y_ref, 'b.')

        ax.set_ylim([0, 100])
        ax.set_xlim([0, 15])
    
        self.canvas = FigureCanvasTkAgg(fig, master = self)  
        #self.canvas.draw()
        self._update_plot()

        self.canvas.get_tk_widget().grid(row=6,column=1, columnspan=2)



        #t1=Thread(target=self._send_loop)
        #t1.start()
        
        #t2=Thread(target=self._rec_loop)
        #t2.start()

        t3=Thread(target=self._check_speed_loop)
        t3.start()

    def _update_plot(self):

        self.canvas.draw()

        self.after(100, self._update_plot)

    def insert_plot_data(self, ref, act):
        #return

        self._plot_mutex.acquire()


        self.y_ref.pop(0)
        self.y_act.pop(0)

        self.y_ref.append(ref)
        self.y_act.append(act)

        self.line_ref.set_ydata(self.y_ref)
        self.line_act.set_ydata(self.y_act)

        self._plot_mutex.release()

        
    #def _send_loop(self):
    #    while True:
    #        msg :str = self._send_queue.get(block=True)
            
    #        self._serial.write(msg.encode('ascii'))
            
            
    #        time.sleep(0.05)

    def send_msg(self, msg: str):
        self.send_raw(msg.encode('ascii'))
        
    def send_raw(self, msg):
        self._serial.write(msg)
        


            
    def _check_speed_loop(self):
        while True:

            print("checking speed")
            self.send_msg('a')

            #time.sleep(0.05)
            
            msg = self._serial.read(3)
            if not msg:
                raise ValueError
            print("get",int(msg))
            #num = struct.unpack('i',msg[0:4])
            #print(num)

            try:
                spd = int(msg)
                
                self._act_speed = spd
                self.act_spd_var.set( str(spd) )

                self.insert_plot_data(self._tar_speed, spd)
            except ValueError:
                print("Not vaild msg.")

            time.sleep(0.05)
            
            self.send_msg('b')
            print("tar",int(self._tar_speed))
            

            msg = struct.pack('I', int(self._tar_speed))

            self.send_raw(msg)

            time.sleep(0.05)

            msg = self._serial.read(3)
            if msg:
                print("set",int(msg))

            time.sleep(0.05)

    def set_tar_speed(self, *args):


        speed = int(self.entry_var.get())

        self._tar_speed = speed
        self.tar_spd_var.set( str(speed) )




    #def send_msg(self, msg):
    #    self._send_queue.put(msg)
        
    #def recieve_msg(self):
    #    msg = self._rec_queue.get(block=True, timeout=1)
    #    return msg


class DummySerial():
    def __init__(self):
        self._list = []
        self._spd = 0
        self._tar_spd = 0

        self._getspd_bool = False


        t1=Thread(target=self._move_towards_target)
        t1.start()

    def write(self, msg :bytes):
        msg :str = msg.decode('ascii')

        if self._getspd_bool:
            try:
                spd = float(msg)
                self._tar_spd = spd
            except ValueError:
                print("Not vaild msg.")

            self._getspd_bool = False

        elif msg == "getspd":
            spd = str(round_sig(self._spd, 6))
            self._list.append(spd.encode('ascii'))

        elif msg == "setspd":
            self._getspd_bool = True
            

        else:
            self._list.append(msg.encode('ascii'))


    def read(self, n=1):
        if len(self._list):
            msg = self._list.pop(0)

            return msg
        else:
            return "".encode('ascii')

    def _move_towards_target(self):
        while True:
            self._spd += (self._tar_spd - self._spd)/4
            time.sleep(0.437)

def round_sig(x, sig=2):
    if x == 0:
        return 0
    return round(x, sig-int(floor(log10(abs(x))))-1)
        

if __name__ == "__main__":
    """
    with Serial('/dev/ttyUSB0', 4800, timeout=1) as serial:

        
        while True:

            print('try a')
            msg = 'a'
            serial.write(msg.encode('ascii'))
            ##time.sleep(0.05)
            ans = serial.read(3)
            if ans:
                print("a",ans)
            
            time.sleep(0.5)
            
            print('try b')
            msg = 'b'
            serial.write(msg.encode('ascii'))
            #msg = 1
            #time.sleep(0.05)
            #msg = b'f/x10'
            #msg = hex(5)
            #msg = '5'.encode('ascii')
            
            msg = struct.pack('I', 30)
            #msg = 0b0d
            #msg = b'30'
            print('trying to send', msg)
            serial.write(msg)

            ans = serial.read(3)
            if ans:
                print("b",ans)

            
            time.sleep(0.5)
        

        while True:

            msg = 'a'
            serial.write(msg.encode('ascii'))
            ##time.sleep(0.05)
            ans = serial.read(1)
            print("a", ans.decode('ascii'))
            
            time.sleep(0.5)
            
            msg = b'020'
            serial.write(msg)
            ##time.sleep(0.05)
            ans = serial.read(3)
            print("num",int(ans))
            
            time.sleep(0.5)
            """

    root = SerialInterface()
    root.mainloop()
