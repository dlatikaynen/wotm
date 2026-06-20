@echo off
copy /y .\index.html .\build-em\
copy /y .\favicon.png .\build-em\
start /b python -m http.server
start /b http://localhost:8000/build-em