import requests
import json

# Твой OAuth-токен
token = 'y0__xCGzMSsAxip6zQg-OyIkxILFCnLzmZ85p9Vo_ez69Ec0_XiqA'
org_id = 'bpfofdlbl4usqgkdim8s'

# Заголовки для авторизации
headers = {
    'Authorization': f'OAuth {token}',
    'Content-Type': 'application/json',
    'X-Cloud-Org-Id': org_id  # Для обычных организаций
}

# URL для получения проектов
url = 'https://api.tracker.yandex.net/v2/issues'

# Заголовок для обновления задачи
change_tag = 'https://api.tracker.yandex.net/v2/issues/67b2fc17626b4074db2b4f4b'  # Placeholder for issue ID

data = {

    "tags": {

        "remove": ["hide-task"]  # Replace with the tag you want to add
        # "add": ["hide-task"]

    }

}

# Convert the data to JSON format

json_data = json.dumps(data)

# Отправка GET-запроса для получения задач
#response = requests.patch(change_tag, headers=headers)
response = requests.patch(change_tag, headers=headers, data=json_data)

# Проверка статуса запроса и вывод результата
print(f"Статус ответа: {response.status_code}")

if response.status_code == 200:
    issues = response.json()  # Информация о задачах
    print(issues)

                
else:
    print(f"Ошибка: {response.status_code}, {response.text}")
