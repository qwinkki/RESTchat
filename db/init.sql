CREATE TABLE IF NOT EXISTS messages (
    id          SERIAL PRIMARY KEY,
    user        TEXT NOT NULL,
    text        TEXT NOT NULL,
    createdAt   TIMESTAMP DEFAULT NOW()
);