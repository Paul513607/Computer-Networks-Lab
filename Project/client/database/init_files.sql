DROP TABLE repo;
DROP TABLE commits;

CREATE TABLE repo(version INTEGER PRIMARY KEY, hash VARCHAR2(256), filesystem BLOB);
CREATE TABLE commits(version INTEGER PRIMARY KEY, hash VARCHAR2(256), patch BLOB);

INSERT INTO repo VALUES(0, '0', '.msvn_init');
INSERT INTO commits VALUES(0, '0', '', '');
