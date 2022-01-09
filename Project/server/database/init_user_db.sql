DROP TABLE user_perm;

CREATE TABLE user_perm(username VARCHAR2(100) PRIMARY KEY NOT NULL, permissions CHAR(3) NOT NULL);

INSERT INTO user_perm VALUES ('paul', 'rwa');
