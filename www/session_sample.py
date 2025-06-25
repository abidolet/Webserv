import sys
import requests
import time

headers = {
	"request_type" : "client_credentials",
}

url = "http://127.0.0.1:8080"
response = requests.post(url, headers=headers)

uid = response.text
print(f"client uid has been aquired, uid:{uid}")
headers = {
		"request_type" : "client_visits",
		"UID": str(uid),
		}
response = requests.post(url, headers=headers)
if (response.status_code == 200):
	print(f"you have made {response.text} request in total")
