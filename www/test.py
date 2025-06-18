#!/usr/bin/env python3
import cgi
import cgitb; cgitb.enable()  # Enable debugging (remove in production)

print("Content-Type: text/html")  # HTTP header
print()  # Blank line to separate headers from body
print("<html>")
print("<head><title>Hello World - First CGI Program</title></head>")
print("<body>")
print("<h2>Hello World! This is my first CGI program</h2>")
print("</body>")
print("</html>")