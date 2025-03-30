import requests

# Твой OAuth-токен
token = 'y0__xCGzMSsAxip6zQg-OyIkxILFCnLzmZ85p9Vo_ez69Ec0_XiqA'
org_id = 'bpfofdlbl4usqgkdim8s'

id_task =  ["679c8736ed61c22d6e28f9b2",
 "67b2fbce626b4074db2b4ec6",
 ]

# Заголовки для авторизации
headers = {
    'Authorization': f'OAuth {token}',
    'Content-Type': 'application/json',
    'X-Cloud-Org-Id': org_id  # Для обычных организаций
}

# URL для получения проектов
url = 'https://api.tracker.yandex.net/v2/issues'

# Заголовок для обновления задачи
update_url = 'https://api.tracker.yandex.net/v2/issues/{issue_id}/transitions/start_progress/_execute'  # Placeholder for issue ID
url_all_transitions = 'https://api.tracker.yandex.net/v2/issues/{issue_id}/transitions'

# Отправка GET-запроса для получения задач
response = requests.get(url_all_transitions.format(issue_id = id_task[0]), headers=headers)

# Проверка статуса запроса и вывод результата
print(f"Статус ответа: {response.status_code}")

if response.status_code == 200:
    issues = response.json()  # Информация о задачах
    print(issues)
    
    
##    # Проходим по каждой задаче
##    for issue in issues:
##        issue_id = issue.get('id')
##        status_type = issue.get('status', {}).get('key')
##
##        data = {
##                "status": {
##                    "key": "open"  # Замените на корректный ID или ключ статуса для "new"
##                },
##            }
##
##        if (status_type):
##            update_response = requests.post(update_url.format(issue_id=issue_id), headers=headers)
##
##            if update_response.status_code == 200:
##                print(f"Task {issue_id} has been moved back to 'new' status.")
##            else:
##                print(f"Failed to update task {issue_id}:{update_response.status_code}, {update_response.text}")
                
else:
    print(f"Ошибка: {response.status_code}, {response.text}")
