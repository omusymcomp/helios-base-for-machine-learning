import os;

#変数の定義
file_path = 'parameters.txt'
parameter = 'pressing'
value = 16

#ファイルへの書き込み
try:
    with open(file_path, 'w') as file:
        file.write(f'{parameter}, {value}\n')
    print("Debug: Write operation successful.")
except Exception as e:
    print(f"Error: Failed to write to file {file_path}. Exception: {e}")


#保存チェック
if os.path.exists(file_path):
    print(f"Debug: File [{file_path}] exists after writing.")
else:
    print(f"Error: File {{file_path}} does not exist afrer attempting to write.")