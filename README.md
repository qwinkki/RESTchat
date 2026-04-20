# RESTchat

- start all project `docker compose up --build` (bad choice)
- start PostgreSQL `docker compose up --build postgres`
- start server `docker compose up --build server`
- start client `docker compose run --rm client`

Nessesary:
- docker

start project:
- `docker compose up --build postgres server`
- client chat: `docker compose run --rm client` as much as you need

#### Additionally:
Services run on ubuntu:24.04  
Using curl and pqxx libraries for api and DB  
