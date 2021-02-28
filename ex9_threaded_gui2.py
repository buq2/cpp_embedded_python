print('Python module loaded')

import threading
import time
# import numpy as np # Had problems importing numpy fro some reason
from multiprocessing import Process, Pipe
import random

def cli_loop(conn):
    while True:
        gui_message = conn.recv()

        if gui_message == 'quit':
            print('CLI received quit')
            break

        print(gui_message) 

def gui_loop2(conn):
    import tkinter as tk
    window = tk.Tk()
    label_text = tk.StringVar()
    label = tk.Label(textvariable=label_text)
    label.pack()

    while window.state() == "normal":
        window.update_idletasks()

        # No need for locking in gui process as it is single threaded
        gui_message = conn.recv()

        if gui_message == 'quit':
            print('GUI received quit')
            break

        label_text.set(gui_message)

        window.update()
        time.sleep(0.01)

parent_conn = None
child_conn = None
conn_lock = None
process = None
def update_gui_info(th_idx):
    global conn_lock
    global parent_conn
    with conn_lock:
        gui_message = 'Thread {} called. Here is a random number {}'.format(th_idx, random.random())
        parent_conn.send(gui_message)

def start_gui_process():
    global parent_conn
    global child_conn
    global conn_lock
    global process
    conn_lock = threading.RLock()
    parent_conn, child_conn = Pipe()

    # Check if tkinter can be imported
    try:
        import tkinter as tk
        # Success, we can create the GUI
        fun = gui_loop
    except:
        # Failure, use text output
        fun = cli_loop

    process = Process(target=fun, args=(child_conn,))
    process.start()
    # The GUI process is not able to exit nicely in this example

def quit_gui():
    global parent_conn
    global conn_lock
    global process
    with conn_lock:
        parent_conn.send("quit")
    process.join()