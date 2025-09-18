# 🌐 Webserv – C++98 HTTP Server  

A lightweight **HTTP/1.1 webserver** implemented in **C++98** at [42 Lausanne](https://42lausanne.ch).  
This project replicates the core functionality of real servers like **Nginx** or **Apache** — parsing requests, managing sockets, serving static files, and handling **CGI scripts**.  

---

## 🚀 About the Project  

The goal was to implement a **fully functional HTTP server** from scratch, respecting the **RFC 7230** specification, without relying on external web frameworks.  

**Key Features:**  
- 📄 Serve **static HTML/CSS/JS** files  
- 🗂️ **File uploads** via `multipart/form-data`  
- 🐍 **CGI execution** (Python scripts supported, e.g. `upload.py`)  
- 🔄 Keep-alive & connection timeout handling  
- ⚡ Event-driven loop using **epoll()** for concurrency  
- 🛠️ Custom logging  

---

## 🧑‍💻 Authors & Roles  

- **Eva Przybyla**  
  - Designed the **main server loop**  
  - Implemented **sockets** and connection handling  
  - Built the **CGI upload pipeline** (`upload.py`)  
  - Added static HTML interface for testing uploads  

- **Rémi**  
  - Co-designed overall **architecture & configuration system**  
  - Worked on request parsing & response generation  

Together, we designed the server’s architecture and debugging workflow.  

---

## 🛠️ Technologies  

- **C++98** (core server)  
- **POSIX sockets** (`epoll`, `accept`, `send`, `recv`)  
- **Python** (CGI scripts for file upload)  
- **HTML/CSS/JS** (frontend test interface)  

---

## ⚙️ How to Build & Run  

1. **Clone the repo**  

```bash
git clone <your-repo-url>
cd webserv

Build

make


Run the server

./webserv config/default.conf


Test locally

curl -v http://127.0.0.1:8080/


👉 This serves the included index.html and gives access to the file upload form.

📂 Features to Try

Static file serving

curl -v http://127.0.0.1:8080/index.html


Upload a file via CGI (POST multipart)

curl -F "file=@test.txt" http://127.0.0.1:8080/cgi-bin/upload.py


Simulate many requests (load test with ApacheBench)

ab -n 1000 -c 50 http://127.0.0.1:8080/

🔨 Siege Load Tests

Run these from another terminal while the server is running.
Make sure to always use 127.0.0.1 instead of localhost (IPv6 is not supported yet).

Basic stress test (50 concurrent users, 30 seconds):

siege -c50 -t30S http://127.0.0.1:8080/


Target a specific static file:

siege -c30 -t20S http://127.0.0.1:8080/index.html


POST upload test (simulates multiple users uploading a file):

siege -c10 -r5 'http://127.0.0.1:8080/cgi-bin/upload.py POST file=@test.txt'


➡️ Expected:

Availability close to 100%

Thousands of requests served per second

Zero failed transactions if the server is stable

📎 Example UI

The project includes a minimal HTML interface (index.html) where you can:

-Upload a file via browser form

-Test CGI responses directly

http://localhost:8080/index.html

📝 Notes

Built for educational purposes at 42 Lausanne.

Implements the essential core of a real-world HTTP server.

