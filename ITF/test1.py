
from serial import Serial
import threading, queue, concurrent.futures
import tkinter as tk
    
class SerialInterface(tk.Tk):

    def __init__(self, serial):
        super().__init__()
        
        self._outgoing_msg = queue.Queue(maxsize = 15)
        self._serial_mutex = threading.Lock()
        
        self._show_string = " "*50
        
        self._serial = serial
        
    def _send(self):
        msg  = self._outgoing_queue.get(block=True)
        self._serial_mutex.aquire()
        
        
        
        self._serial_mutex.release()

        
    def _recieve(self):
        self._serial_mutex.aquire()
        
        x = ser.read(10)
        
        self._serial_mutex.release()
        
    def _open_interface(self):
        window = tk.Tk()
        greeting = tk.Label(text="Hello, Tkinter")
        greeting.pack()
        
        return window
    
    def run(self):
    
        window = self._open_interface()
    
        with concurrent.futures.ThreadPoolExecutor(max_workers=3) as executor:
            executor.submit(self._send)
            executor.submit(self._recieve)
            window.mainloop()
            
        self._mqtt_client.loop_stop()
        
if __name__ == "__main__":
    
    with Serial('/dev/ttyUSB0') as ser:
        si = SerialInterface(ser)
        si.run()
        
