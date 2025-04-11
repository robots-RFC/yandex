import requests

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
update_url = 'https://api.tracker.yandex.net/v2/issues/{issue_id}/transitions/stop_progress/_execute'  # Placeholder for issue ID

# Отправка GET-запроса для получения задач
response = requests.get(url, headers=headers)

# Проверка статуса запроса и вывод результата
print(f"Статус ответа: {response.status_code}")

if response.status_code == 200:
    issues = response.json()  # Информация о задачах
    print(issues)

                
else:
    print(f"Ошибка: {response.status_code}, {response.text}")
