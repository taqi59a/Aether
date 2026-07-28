import sys
import time
import threading

# Friendly import check for pyserial
pyserial_available = True
try:
    import serial
    import serial.tools.list_ports
except ImportError:
    pyserial_available = False

# Import Tkinter
try:
    import tkinter as tk
    from tkinter import ttk
    from tkinter import messagebox
except ImportError:
    print("[ERROR] Tkinter is not available. Please install python-tk or use standard macOS python3.")
    sys.exit(1)


class MotorControllerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Aether Linear Motor Controller v1")
        self.root.geometry("600x650")
        self.root.configure(bg="#1E1E24")
        self.root.resizable(False, False)

        # Style configurations
        self.setup_styles()

        self.ser = None
        self.running = False
        self.read_thread = None

        if not pyserial_available:
            self.show_missing_dependency_warning()
            return

        self.create_widgets()
        self.scan_ports()

    def setup_styles(self):
        style = ttk.Style()
        style.theme_use('clam')
        style.configure('TFrame', background='#1E1E24')
        style.configure('Card.TFrame', background='#2A2A35', relief='flat')
        style.configure('TLabel', background='#2A2A35', foreground='#FFFFFF', font=('Helvetica', 12))
        style.configure('Title.TLabel', background='#1E1E24', foreground='#00CEC9', font=('Helvetica', 16, 'bold'))
        style.configure('Status.TLabel', background='#2A2A35', foreground='#00CEC9', font=('Helvetica', 14, 'bold'))
        style.configure('TButton', font=('Helvetica', 11, 'bold'), borderwidth=0, focuscolor='none')
        style.map('TButton',
                  background=[('pressed', '#0984E3'), ('active', '#74B9FF'), ('!disabled', '#0984E3')],
                  foreground=[('!disabled', '#FFFFFF')])

    def show_missing_dependency_warning(self):
        # Draw a nice UI warning when pyserial is not installed
        frame = ttk.Frame(self.root, padding=30, style='TFrame')
        frame.pack(fill='both', expand=True)

        title = tk.Label(frame, text="Dependency Missing", bg="#1E1E24", fg="#D63031", font=('Helvetica', 18, 'bold'))
        title.pack(pady=20)

        msg = (
            "The 'pyserial' package is required to communicate with the ESP32-S3 over USB.\n\n"
            "Please open your Mac terminal and run the following command:\n\n"
            "   pip3 install pyserial\n\n"
            "After installing, restart this application."
        )
        label = tk.Label(frame, text=msg, bg="#1E1E24", fg="#FFFFFF", font=('Helvetica', 12), justify='left', wraplength=500)
        label.pack(pady=15)

        btn = tk.Button(frame, text="Exit", command=self.root.quit, bg="#D63031", fg="#FFFFFF", font=('Helvetica', 11, 'bold'), relief='flat', padx=20, pady=10)
        btn.pack(pady=30)

    def create_widgets(self):
        # Main container
        self.main_container = ttk.Frame(self.root, padding=15, style='TFrame')
        self.main_container.pack(fill='both', expand=True)

        # Header Title
        self.title_label = ttk.Label(self.main_container, text="AETHER TECHNOLOGIES — MOTOR CONSOLE v1", style='Title.TLabel')
        self.title_label.pack(pady=(5, 15))

        # ---------------- CONNECTION CARD ----------------
        self.conn_card = ttk.Frame(self.main_container, padding=15, style='Card.TFrame')
        self.conn_card.pack(fill='x', pady=5)

        conn_lbl = ttk.Label(self.conn_card, text="Serial Connection Setup:", font=('Helvetica', 12, 'bold'))
        conn_lbl.grid(row=0, column=0, columnspan=2, sticky='w', pady=(0, 10))

        ttk.Label(self.conn_card, text="Select Port:").grid(row=1, column=0, sticky='w')
        self.port_var = tk.StringVar()
        self.port_dropdown = ttk.Combobox(self.conn_card, textvariable=self.port_var, width=25, state='readonly')
        self.port_dropdown.grid(row=1, column=1, padx=10, sticky='w')

        self.btn_refresh = tk.Button(self.conn_card, text="Refresh", command=self.scan_ports, bg="#636E72", fg="#FFFFFF", relief='flat', padx=10)
        self.btn_refresh.grid(row=1, column=2, padx=5)

        self.btn_connect = tk.Button(self.conn_card, text="Connect", command=self.toggle_connection, bg="#00B894", fg="#FFFFFF", relief='flat', padx=20, width=10)
        self.btn_connect.grid(row=1, column=3, padx=10)

        # ---------------- STATUS CARD ----------------
        self.status_card = ttk.Frame(self.main_container, padding=15, style='Card.TFrame')
        self.status_card.pack(fill='x', pady=5)

        status_lbl = ttk.Label(self.status_card, text="System Live Feedback:", font=('Helvetica', 12, 'bold'))
        status_lbl.grid(row=0, column=0, columnspan=4, sticky='w', pady=(0, 10))

        ttk.Label(self.status_card, text="Status :").grid(row=1, column=0, sticky='w')
        self.lbl_status = ttk.Label(self.status_card, text="DISCONNECTED", style='Status.TLabel')
        self.lbl_status.grid(row=1, column=1, sticky='w', padx=(5, 30))

        ttk.Label(self.status_card, text="Pos (steps):").grid(row=1, column=2, sticky='w')
        self.lbl_steps = ttk.Label(self.status_card, text="0", style='Status.TLabel')
        self.lbl_steps.grid(row=1, column=3, sticky='w', padx=5)

        ttk.Label(self.status_card, text="Pos (mm) :").grid(row=2, column=2, sticky='w', pady=(10, 0))
        self.lbl_mm = ttk.Label(self.status_card, text="0.0 mm", style='Status.TLabel')
        self.lbl_mm.grid(row=2, column=3, sticky='w', padx=5, pady=(10, 0))

        # ---------------- CONTROL CARD ----------------
        self.ctrl_card = ttk.Frame(self.main_container, padding=15, style='Card.TFrame')
        self.ctrl_card.pack(fill='x', pady=5)

        ctrl_lbl = ttk.Label(self.ctrl_card, text="Linear Actuator Control Panel:", font=('Helvetica', 12, 'bold'))
        ctrl_lbl.pack(anchor='w', pady=(0, 10))

        # Slider mm
        slider_frame = ttk.Frame(self.ctrl_card, style='Card.TFrame')
        slider_frame.pack(fill='x', pady=5)
        
        ttk.Label(slider_frame, text="Set Position (0 - 50 mm):").pack(side='left', padx=(0, 10))
        self.val_label = ttk.Label(slider_frame, text="25 mm", style='Status.TLabel')
        self.val_label.pack(side='right', padx=(10, 0))

        self.slider = tk.Scale(
            self.ctrl_card, from_=0, to=50, orient='horizontal',
            bg="#2A2A35", fg="#FFFFFF", highlightthickness=0,
            troughcolor="#1E1E24", activebackground="#0984E3",
            showvalue=0, command=self.update_slider_label
        )
        self.slider.set(25)
        self.slider.pack(fill='x', pady=5)

        btn_frame = ttk.Frame(self.ctrl_card, style='Card.TFrame')
        btn_frame.pack(fill='x', pady=10)

        self.btn_go = tk.Button(btn_frame, text="Go to Position", command=self.send_go, bg="#0984E3", fg="#FFFFFF", relief='flat', font=('Helvetica', 11, 'bold'), pady=8)
        self.btn_go.pack(side='left', fill='x', expand=True, padx=5)

        self.btn_home = tk.Button(btn_frame, text="Calibrate (Home)", command=self.send_home, bg="#6C5CE7", fg="#FFFFFF", relief='flat', font=('Helvetica', 11, 'bold'), pady=8)
        self.btn_home.pack(side='left', fill='x', expand=True, padx=5)

        self.btn_sweep = tk.Button(btn_frame, text="Test Sweep", command=self.send_sweep, bg="#2C3E50", fg="#FFFFFF", relief='flat', font=('Helvetica', 11, 'bold'), pady=8)
        self.btn_sweep.pack(side='left', fill='x', expand=True, padx=5)

        # Big Red Emergency Stop Button
        self.btn_stop = tk.Button(self.ctrl_card, text="EMERGENCY STOP (HALT)", command=self.send_stop, bg="#D63031", fg="#FFFFFF", relief='flat', font=('Helvetica', 12, 'bold'), pady=12)
        self.btn_stop.pack(fill='x', pady=(10, 5))

        # ---------------- CONSOLE / LOGS CARD ----------------
        self.log_card = ttk.Frame(self.main_container, padding=10, style='Card.TFrame')
        self.log_card.pack(fill='both', expand=True, pady=5)

        ttk.Label(self.log_card, text="Micro-Console Serial Logs:", font=('Helvetica', 11, 'bold')).pack(anchor='w', pady=(0, 5))

        self.txt_logs = tk.Text(self.log_card, height=6, bg="#1E1E24", fg="#00FF00", font=('Courier', 10), state='disabled', wrap='word', borderwidth=0)
        self.txt_logs.pack(fill='both', expand=True)

        self.log("Ready. Select serial port and connect to begin.")

    def log(self, text):
        self.txt_logs.config(state='normal')
        self.txt_logs.insert(tk.END, f"[{time.strftime('%H:%M:%S')}] {text}\n")
        self.txt_logs.see(tk.END)
        self.txt_logs.config(state='disabled')

    def scan_ports(self):
        ports = serial.tools.list_ports.comports()
        port_list = [p.device for p in ports]
        self.port_dropdown['values'] = port_list
        if port_list:
            # Prefer USB serial devices if available
            usb_ports = [p for p in port_list if "usb" in p.lower() or "usbmodem" in p.lower()]
            if usb_ports:
                self.port_var.set(usb_ports[0])
            else:
                self.port_var.set(port_list[0])
            self.log(f"Scanned: Found {len(port_list)} serial port(s).")
        else:
            self.port_var.set('')
            self.log("Scanned: No serial ports found. Connect S3 dev kit via USB.")

    def update_slider_label(self, val):
        self.val_label.config(text=f"{val} mm")

    def toggle_connection(self):
        if self.ser is None:
            self.connect()
        else:
            self.disconnect()

    def connect(self):
        port = self.port_var.get()
        if not port:
            messagebox.showwarning("Connection Error", "Please select a serial port from the dropdown.")
            return

        try:
            self.ser = serial.Serial(port, 115200, timeout=0.1)
            self.btn_connect.config(text="Disconnect", bg="#D63031")
            self.lbl_status.config(text="CONNECTED", fg="#00B894")
            self.log(f"Connected to port {port} successfully.")
            
            # Start serial reading thread
            self.running = True
            self.read_thread = threading.Thread(target=self.read_serial_loop, daemon=True)
            self.read_thread.start()
        except Exception as e:
            self.ser = None
            messagebox.showerror("Connection Failed", f"Could not connect to port {port}:\n{str(e)}")
            self.log(f"Connection to {port} failed.")

    def disconnect(self):
        self.running = False
        if self.ser:
            try:
                self.ser.close()
            except:
                pass
            self.ser = None

        self.btn_connect.config(text="Connect", bg="#00B894")
        self.lbl_status.config(text="DISCONNECTED", fg="#00CEC9")
        self.log("Disconnected from serial port.")

    def read_serial_loop(self):
        buffer = ""
        while self.running and self.ser:
            try:
                if self.ser.in_waiting > 0:
                    data = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
                    buffer += data
                    while "\n" in buffer:
                        line, buffer = buffer.split("\n", 1)
                        line = line.strip()
                        if line:
                            self.parse_serial_line(line)
            except Exception as e:
                self.log(f"Read error: {str(e)}")
                self.root.after(0, self.disconnect)
                break
            time.sleep(0.02)

    def parse_serial_line(self, line):
        if line.startswith("STATUS:"):
            # Format: STATUS:<state>|POS:<steps>|MM:<mm>
            try:
                parts = line.split("|")
                state = parts[0].split(":")[1]
                pos = parts[1].split(":")[1]
                mm = parts[2].split(":")[1]
                
                # Update UI elements in main thread safely
                self.root.after(0, lambda: self.update_status_ui(state, pos, mm))
            except Exception as e:
                pass
        else:
            # Print non-status raw printouts to the console log
            self.root.after(0, lambda: self.log(f"ESP32: {line}"))

    def update_status_ui(self, state, pos, mm):
        self.lbl_status.config(text=state.upper())
        # Color status depending on state
        if state.lower() == "idle" or state.lower() == "ready":
            self.lbl_status.config(fg="#00B894") # green
        elif "calibrat" in state.lower() or "wait" in state.lower():
            self.lbl_status.config(fg="#E1B12C") # yellow
        elif "vend" in state.lower() or "mov" in state.lower():
            self.lbl_status.config(fg="#E84118") # orange/red
        
        self.lbl_steps.config(text=pos)
        self.lbl_mm.config(text=f"{mm}.0 mm")

    def send_cmd(self, cmd):
        if not self.ser:
            messagebox.showwarning("Connection Offline", "Please connect to the serial port first.")
            return False
        try:
            self.ser.write(f"{cmd}\n".encode('utf-8'))
            self.log(f"Sent command: {cmd}")
            return True
        except Exception as e:
            self.log(f"Write error: {str(e)}")
            self.disconnect()
            return False

    def send_go(self):
        val = self.slider.get()
        self.send_cmd(f"GO:{val}")

    def send_home(self):
        self.send_cmd("HOME")

    def send_sweep(self):
        self.send_cmd("SWEEP")

    def send_stop(self):
        self.send_cmd("STOP")


if __name__ == "__main__":
    root = tk.Tk()
    app = MotorControllerApp(root)
    root.mainloop()
