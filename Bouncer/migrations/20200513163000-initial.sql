BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS migrations(
    migration TEXT PRIMARY KEY
);
INSERT OR FAIL INTO migrations(migration) VALUES('20200513163000-initial');
CREATE TABLE users(
    bot INTEGER,
    createdAt REAL,
    firstMessageTime REAL,
    firstSeenTime REAL,
    id INTEGER PRIMARY KEY,
    isBanned BOOLEAN,
    isWhitelisted BOOLEAN,
    lastMessageTime REAL,
    login TEXT,
    name TEXT,
    note TEXT,
    numMessages INTEGER,
    role INTEGER,
    timeout REAL,
    totalViewTime REAL,
    watching BOOLEAN
);
CREATE TABLE chat(
    seq INTEGER PRIMARY KEY AUTOINCREMENT,
    userid INTEGER,
    message TEXT
);
END TRANSACTION;
