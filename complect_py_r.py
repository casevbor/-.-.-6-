""" 
Удаленный доступ на закрытую территорию, Py (под Linux + raspberrypi)

---         Скрипт для работы с базой данных .xlsx        ---
---                  Получаем UNIX по UID                 ---
---  Перезаписыаем просроченый UNIX для UID = 1945136253  ---
""" 


import pandas
import openpyxl
import serial
import time

# Открываем файл .xlsx
wb = openpyxl.load_workbook('/home/user/Documents/card_UIDs.xlsx')
sheet = wb.active

# Открываем последовательный порт для общения с Arduino
ser = serial.Serial('/dev/ttyACM0', 9600) # ttyACM0 - аналог COM для Linux

# Функция перезаписи даты для просроченой карты
def rewrite_unix():
    target_uid = "1945136253"
    new_unix_value = str(round(time.time() + 30 * 24 * 60 * 60))
    df.loc[df['UID'] == target_uid, 'unix'] = new_unix_value
    df.to_excel('/home/user/Documents/card_UIDs.xlsx')


# Функция перезаписи базы данных (сброс)
def create_DB():
    global df
    df = pandas.DataFrame(
        {'SFNP': ["Иванов Иван Иванович", "Сигмабоев Сигмабой Сигмабоевич"],
         'UID': ["1945136253", "131164183239"],
         'unix': [str(round(time.time() - 30 * 24 * 60 * 60)), str(round(time.time() + 30 * 24 * 60 * 60))]
    })
    df.to_excel('/home/user/Documents/card_UIDs.xlsx')


# Функция получения UNIX карты по ее UID (UNIX - время в секундах, начиная с 01.01.1970)
def get_unix_by_uid(uid):
    temp = pandas.read_excel('/home/user/Documents/card_UIDs.xlsx', index_col=0, dtype={'SFNP': str, 'UID': str, 'unix': int})
    unix_value = temp.loc[temp['UID'] == uid, 'unix']
    if not unix_value.empty:
        return unix_value.iloc[0]
    else:
        return None
    

create_DB()

try:
    # Обработка запросов с ttyACM0 порта (с ардуинки)
    while True:
        if ser.in_waiting > 0:
            uid_to_search = str(ser.readline().decode().strip())
            if "Запрос: UID =" in uid_to_search:
                uid_to_search = uid_to_search.replace("Запрос: UID = ", "")
                print(f"Получен запрос от Arduino: Search UID = {uid_to_search}")
                unix_time = get_unix_by_uid(uid_to_search)

                if unix_time is not None:
                    ser.write(f"{str(time.time() < unix_time)}".encode())
                else:
                    ser.write("False".encode())
            elif "Rewrite: UID =" in uid_to_search:
                print(f"Получен запрос от Arduino: Rewrite UID = 1945136253")
                rewrite_unix()
                ser.write(f"TRewrite".encode())
                
        time.sleep(0.01)

except KeyboardInterrupt:
    print("Завершаем работу.")
    ser.close()

