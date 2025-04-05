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


import requests
import json

task_red = "67b2fc17626b4074db2b4f4b"
task_blue = "67e69ed22d2ae1712cc18426"


def get_all_tasks():
    url = f"https://api.tracker.yandex.net/v2/issues"

    response = requests.get(url, headers=headers,)

    if response.status_code == 200:
        data = response.json()

        for i in data:
            task_id = i["id"]
            task_position = i["status"]["key"]
            if ((task_id != task_red) and (task_id != task_blue)):
                remove_assignee_from_issue(task_id);

            #print(i)

            if (task_position != "open"):
                print(task_position)
                if task_position == "closed":
                    move_to_backlog(task_id, "reopen")
                elif task_position == "inProgress":
                    move_to_backlog(task_id, "stop_progress")
                else:
                    move_to_backlog(task_id, "provide_info")
    else:
        print(f"Ошибка: {response.status_code}")
    
    

def remove_assignee_from_issue(task_id):
    """Удаляет исполнителя из задачи в Яндекс Трекере."""

    url = f"https://api.tracker.yandex.net/v2/issues/" + task_id
    payload = {
        "assignee": None
    }
    payload = json.dumps(payload)

    response = requests.patch(url, headers=headers, data=payload)

    if response.status_code == 200:
        print("Исполнитель успешно удален.")
    else:
        print(f"Ошибка при удалении исполнителя: {response.status_code}")


def move_to_backlog(task_id, move):
    update_url = 'http://api.tracker.yandex.net/v2/issues/{issue_id}/transitions/{move}/_execute'  # Placeholder for issue ID

    # Отправка GET-запроса для получения задач
    response = requests.post(update_url.format(issue_id = task_id, move = move), headers=headers)

    # Проверка статуса запроса и вывод результата

    if response.status_code == 200:
        print("Moved")
    else:
        print(f"Ошибка: {response.status_code} {task_id}")


get_all_tasks()
#remove_assignee_from_issue()
