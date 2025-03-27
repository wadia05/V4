#!/usr/bin/env python3
import os
import sys
import cgi

# Print HTTP headers
print("Content-Type: text/html")
print()  # Empty line, end of headers

# Get environment variables
method = os.environ.get('REQUEST_METHOD', '')
query_string = os.environ.get('QUERY_STRING', '')
body_part = os.environ.get('BODY_PART', '')

# Parse query string
params = {}
if query_string:
    for pair in query_string.split('&'):
        if '=' in pair:
            key, value = pair.split('=', 1)
            params[key] = value
        else:
            params[pair] = ''

# Parse body part if it's a POST request
body_params = {}
if method == 'POST' and body_part:
    for pair in body_part.split('&'):
        if '=' in pair:
            key, value = pair.split('=', 1)
            body_params[key] = value
        else:
            body_params[pair] = ''

# Generate HTML response
print("<html>")
print("<head><title>CGI Test</title></head>")
print("<body>")
print("<h1>Hello from Python CGI!</h1>")

print("<h2>Environment Variables:</h2>")
print("<ul>")
print(f"<li>REQUEST_METHOD: {method}</li>")
print(f"<li>QUERY_STRING: {query_string}</li>")
print("</ul>")

if params:
    print("<h2>Query Parameters:</h2>")
    print("<ul>")
    for key, value in params.items():
        print(f"<li>{key}: {value}</li>")
    print("</ul>")

if body_params:
    print("<h2>POST Data:</h2>")
    print("<ul>")
    for key, value in body_params.items():
        print(f"<li>{key}: {value}</li>")
    print("</ul>")

print("<h2>All Environment Variables:</h2>")
print("<ul>")
for key, value in sorted(os.environ.items()):
    print(f"<li>{key}: {value}</li>")
print("</ul>")

print("</body>")
print("</html>")