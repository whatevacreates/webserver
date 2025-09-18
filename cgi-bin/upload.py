#!/usr/bin/env python3

import os
import cgi
import sys

LOG_FILE = "/tmp/logfile.log"
UPLOAD_DIR = "./uploads"
# whats up with this directory ?


# Ensure log file exists (create if it doesn't)
if not os.path.exists(LOG_FILE):
    with open(LOG_FILE, "w") as f:
        pass

# Ensure the upload directory exists
if not os.path.exists(UPLOAD_DIR):
    try:
        os.makedirs(UPLOAD_DIR)
    except Exception as e:
        with open(LOG_FILE, "a") as log_file:
            log_file.write(f"[ERROR] Failed to create directory '{UPLOAD_DIR}': {e}\n")
        print("Content-Type: text/html\n")
        print("<html><body><pre>Failed to create upload directory.</pre></body></html>")
        sys.exit(1)

with open(LOG_FILE, "a") as log_file:
    log_file.write("[LOG] CGI script started\n")
    log_file.write("[LOG] Environment Variables:\n")
    for key, value in os.environ.items():
        log_file.write(f"{key}={value}\n")

form = cgi.FieldStorage()

# Calculate total incoming form data size
form_data_size = 0
for key in form.keys():
    # Each 'form[key]' might be a file or normal field
    if hasattr(form[key], 'file') and form[key].file:
        # Move file pointer to end to measure size, then reset
        current_pos = form[key].file.tell()
        form[key].file.seek(0, 2)
        total_size = form[key].file.tell()
        form[key].file.seek(current_pos, 0)
        form_data_size += total_size
    else:
        if isinstance(form[key].value, str):
            form_data_size += len(form[key].value.encode('utf-8'))

with open(LOG_FILE, "a") as log_file:
    log_file.write(f"[LOG] Total form data size: {form_data_size} bytes\n")
    if form_data_size == 0:
        log_file.write("[LOG] No form data received from stdin.\n")
    else:
        log_file.write("[LOG] Form data received successfully.\n")

with open(LOG_FILE, "a") as log_file:
    if form.keys():
        log_file.write("[LOG] Form keys and values:\n")
        for key in form.keys():
            if hasattr(form[key], 'filename'):
                log_file.write(f"{key}: {form[key].filename}\n")
            else:
                log_file.write(f"{key}: {form[key].value}\n")
    else:
        log_file.write("[LOG] No keys found in form data.\n")

# Handle file upload in chunks for large files
if "file" in form:
    file_item = form["file"]
    if file_item.file:
        file_name = os.path.basename(file_item.filename)
        upload_path = os.path.join(UPLOAD_DIR, file_name)

        with open(upload_path, "wb") as output_file:
            chunk_size = 1024
            while True:
                chunk = file_item.file.read(chunk_size)
                if not chunk:
                    break
                output_file.write(chunk)

        with open(LOG_FILE, "a") as log_file:
            log_file.write(f"[LOG] File '{file_name}' uploaded successfully to {upload_path}\n")
    else:
        with open(LOG_FILE, "a") as log_file:
            log_file.write("[LOG] No file was uploaded.\n")
else:
    with open(LOG_FILE, "a") as log_file:
        log_file.write("[LOG] File field not found in the form.\n")

with open(LOG_FILE, "a") as log_file:
    log_file.write("[LOG] CGI request completed.\n")

for key, value in os.environ.items():
    if key == "QUERY_STRING":
        print("<p>Query String: {}</p>".format(os.environ.get("QUERY_STRING", "")))



if "file" in form and form["file"].file:
    print("Content-Type: text/plain\n")
    print("File uploaded successfully.")
else:
    print("Content-Type: text/html\n")
    print("<html><body><pre>No file uploaded.</pre></body></html>")
