# syntax=docker/dockerfile:1

FROM python:3.12.3-alpine

WORKDIR /treeapi

# Install python packages:
COPY requirements.txt requirements.txt
RUN pip3 install -r requirements.txt

# Expose port 5000 to allow external access to the web server
EXPOSE 5000

CMD ["python3","main.py"]
