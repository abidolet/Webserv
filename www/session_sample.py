import sys
import requests
import time

body = {
	"grant_type" : "client_credentials",
}

url = "http://127.0.0.1:8008"
response = requests.post(url, data=body)
if (response.status_code != 200):
	sys.exit()

uid = response.text
print(f"client uid has been aquired, uid:{uid}")
time.sleep(0.5)
body = {
	"grant_type" : "client_visits",
	"UID": str(uid),
}
response = requests.post(url, data=body)
if (response.status_code == 200):
	print(f"you have made {response.text} request in total")