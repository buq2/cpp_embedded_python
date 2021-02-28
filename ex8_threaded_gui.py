print('Python module loaded')


import threading
import time
import numpy as np

gui_lock = threading.RLock()
gui_message = ''

def gui_loop():
    import tkinter as tk
    window = tk.Tk()
    label_text = tk.StringVar()
    label = tk.Label(textvariable=label_text)
    label.pack()

    while window.state() == "normal":
        window.update_idletasks()

        with gui_lock:
            label_text.set(gui_message)

        window.update()
        time.sleep(0.01)

def update_gui_info(th_idx):
    global gui_message
    with gui_lock:
        gui_message = 'Thread {} called. Here is a random number {}'.format(th_idx, np.random.rand())
    
# When we load the module first time, we create the GUI thread
gui_th = threading.Thread(target=gui_loop)
gui_th.start()
# The GUI thread is not able to exit nicely in this example