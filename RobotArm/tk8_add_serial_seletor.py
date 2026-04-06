
import tkinter as tk
from tkinter import ttk
import serial 
import threading 
import sys 
import glob

# 4개 관절(Base, Shoulder, Upperarm, Forearm)의 초기 각도 설정 [cite: 32, 33]
angles = [90, 90, 90, 90]
serial_list = ['포트 검색 중...']
seq = serial.Serial(baudrate=115200, timeout=1) # [cite: 12]

# --- 핸들러 함수 구역 ---

def select_serial(selection):
    var.set(selection)

def start_serial():
    global seq
    selected_port = var.get()
    if selected_port and selected_port not in ['포트 검색 중...', '검색된 포트 없음']:
        try:
            if seq.is_open: seq.close()
            seq.port = selected_port
            seq.open()
            print(f"{selected_port} 연결 성공!")
            start_serial_btn.configure(state='disabled')
            stop_serial_btn.configure(state='normal')
        except Exception as e:
            print(f"연결 에러: {e}")

def stop_serial():
    if seq.is_open:
        seq.close()
        start_serial_btn.configure(state='normal')
        stop_serial_btn.configure(state='disabled')

# 슬라이더 조작 시 (슬라이더 -> 입력창 업데이트)
def slide_handler(idx, val):
    angles[idx] = int(float(val))
    entry_list = [port0, port1, port2, port3]
    entry_list[idx].delete(0, 'end')
    entry_list[idx].insert(0, str(angles[idx]))

# 입력창 엔터 시 (입력창 -> 슬라이더 & 로봇 동작)
def entry_handler(idx, event):
    entry_list = [port0, port1, port2, port3]
    slider_list = [s0, s1, s2, s3]
    try:
        new_val = int(entry_list[idx].get())
        if 0 <= new_val <= 180:
            angles[idx] = new_val
            slider_list[idx].set(new_val)
            run_robot() # 로봇 동작 수행 [cite: 18]
        else:
            print("0~180 사이의 값을 입력하세요.")
    except ValueError:
        print("숫자만 입력 가능합니다.")

def run_robot():
    if seq.is_open:
        # [수정] 사용자의 요청에 따라 Upperarm(angles[2])과 Forearm(angles[3])의 전송 위치를 스왑함
        # 아두이노 수신부: c=Upperarm[cite: 20], d=Forearm[cite: 24]
        # 조작 일치를 위해 c 자리에 Forearm 값을, d 자리에 Upperarm 값을 전송 
        cmd = f"2a{angles[0]}b{angles[1]}c{angles[3]}d{angles[2]}e\n"
        seq.write(cmd.encode())
        print(f"전송 데이터: {cmd.strip()}")
    else:
        print("시리얼 포트를 먼저 연결하세요.")

def stop_robot():
    global angles
    if seq.is_open:
        # 1. 아두이노에 초기화 명령어 전송 [cite: 30]
        seq.write('3\n'.encode())
        
        # 2. 파이썬 내부 데이터 및 GUI 위젯 초기화 (90도) 
        angles = [90, 90, 90, 90]
        slider_list = [s0, s1, s2, s3]
        entry_list = [port0, port1, port2, port3]
        
        for i in range(4):
            slider_list[i].set(90)
            entry_list[i].delete(0, 'end')
            entry_list[i].insert(0, '90')
            
        print("로봇 및 GUI 상태 초기화 완료")

# --- 시리얼 포트 검색 및 UI 업데이트 로직 ---
def update_option_menu():
    global dropdown, serial_list
    menu = dropdown["menu"]
    menu.delete(0, "end")
    for string in serial_list:
        menu.add_command(label=string, command=lambda v=string: select_serial(v))
    priority_ports = ['COM7', 'COM6', 'COM5', 'COM4']
    for p in priority_ports:
        if p in serial_list:
            var.set(p); break

def timerCallBack():
    global serial_list
    result = serial_ports()
    serial_list = result if result else ["검색된 포트 없음"]
    update_option_menu()

def startTimer(iTimeSec):
    threading.Timer(iTimeSec, timerCallBack).start()

def serial_ports():
    if sys.platform.startswith('win'): ports = ['COM%s' % (i + 1) for i in range(256)]
    else: ports = glob.glob('/dev/tty[A-Za-z]*')
    result = []
    for port in ports:
        try:
            s = serial.Serial(port); s.close(); result.append(port)
        except: pass
    return result

# --- GUI 구성 ---
root = tk.Tk()
root.title('KG-KAIROS Robot Control')

m_serial_select = ttk.Frame(root)
var = tk.StringVar(value=serial_list[0])
dropdown = ttk.OptionMenu(m_serial_select, var, serial_list[0])
dropdown.pack()

start_serial_btn = ttk.Button(root, text="Start serial", command=start_serial)
stop_serial_btn = ttk.Button(root, text="Stop serial", command=stop_serial, state='disabled')

def create_link_ui(master, label_text, idx):
    frame = ttk.Frame(master)
    ttk.Label(frame, text=label_text, font='Helvetica 10 bold').pack(side='left')
    entry = ttk.Entry(frame, width=4)
    entry.insert('end', '90')
    entry.bind("<Return>", lambda event: entry_handler(idx, event)) # 엔터 키 바인딩
    entry.pack(side='left')
    return frame, entry

m_link0, port0 = create_link_ui(root, 'Base: ', 0)
m_link1, port1 = create_link_ui(root, 'Shoulder: ', 1)
m_link2, port2 = create_link_ui(root, 'Upperarm: ', 2)
m_link3, port3 = create_link_ui(root, 'Forearm: ', 3)

def create_slider(master, idx):
    frame = ttk.Frame(master)
    slider = ttk.Scale(frame, length=150, from_=0, to=180, orient="vertical", 
                       command=lambda v: slide_handler(idx, v))
    slider.set(90)
    slider.pack()
    return frame, slider

m_slide_0, s0 = create_slider(root, 0)
m_slide_1, s1 = create_slider(root, 1)
m_slide_2, s2 = create_slider(root, 2)
m_slide_3, s3 = create_slider(root, 3)

m_serial_select.grid(column=1, row=0, columnspan=4, pady=10)
start_serial_btn.grid(column=1, row=1); stop_serial_btn.grid(column=2, row=1)
m_link0.grid(column=1, row=2); m_link1.grid(column=2, row=2)
m_link2.grid(column=3, row=2); m_link3.grid(column=4, row=2)
m_slide_0.grid(column=1, row=3); m_slide_1.grid(column=2, row=3)
m_slide_2.grid(column=3, row=3); m_slide_3.grid(column=4, row=3)
ttk.Button(root, text="run robot", command=run_robot).grid(column=1, row=4, pady=15)
ttk.Button(root, text="stop robot", command=stop_robot).grid(column=2, row=4, pady=15)

startTimer(1)
root.mainloop()