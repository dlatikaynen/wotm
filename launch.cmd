@echo off
copy /y .\index.html .\build-em\
start /b python -m http.server
start /b http://localhost:8000/build-em