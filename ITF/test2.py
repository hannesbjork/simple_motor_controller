import tkinter as tk
from tkinter import ttk
from tkinter.messagebox import showerror
from threading import Thread

from serial import Serial
import time

class SendAndRecieve(Thread):
    def __init__(self, serial):
        super().__init__()
        
        self._serial = serial

    def run(self):
        self._serial.write(b'hello')
        #time.sleep(1)


class App(tk.Tk):
    def __init__(self):
        super().__init__()

        self.title('Webpage Download')
        self.geometry('680x430')
        self.resizable(0, 0)

            
        with Serial('/dev/ttyUSB0') as ser:
            serial_thread = SendAndRecieve(ser)
            serial_thread.start()

if __name__ == "__main__":
    app = App()
    app.mainloop()
    
    

