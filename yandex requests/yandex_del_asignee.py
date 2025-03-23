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

def remove_assignee_from_issue():
    """Удаляет исполнителя из задачи в Яндекс Трекере."""

    url = f"https://api.tracker.yandex.net/v2/issues/679c8736ed61c22d6e28f9b2"
    payload = {
        "assignee": None
    }
    payload = json.dumps(payload)

    response = requests.patch(url, headers=headers, data=payload)

    if response.status_code == 200:
        print("Исполнитель успешно удален.")
        print(response.json())
    else:
        print(f"Ошибка при удалении исполнителя: {response.status_code}")


remove_assignee_from_issue()
