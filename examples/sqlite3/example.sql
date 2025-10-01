CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    login TEXT NOT NULL,
    password TEXT UNIQUE,
    age INTEGER NOT NULL,
    height REAL NOT NULL,
    MARRIED BOOLEAN NOT NULL
);

INSERT INTO users (login, password, age, height, married) VALUES ('bob', 'verysecurepassword', 22, 69.100000, 0);
INSERT INTO users (login, password, age, height, married) VALUES ('alice', 'strongpassword', 22, 420.200012, 1);