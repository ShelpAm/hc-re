CREATE TABLE student (
    student_id CHAR(12) NOT NULL,
    name VARCHAR(50) NOT NULL,
    PRIMARY KEY (student_id)
);

CREATE TABLE assignment (
    name VARCHAR(50) NOT NULL,
    start_time TIMESTAMP NOT NULL,
    end_time TIMESTAMP NOT NULL,
    PRIMARY KEY (name)
);

CREATE TABLE submission (
    assignment_name VARCHAR(50) NOT NULL,
    student_id CHAR(12) NOT NULL,
    submission_time TIMESTAMP NOT NULL,
    filepath VARCHAR(1024) NOT NULL,
    original_filename VARCHAR(1024) NOT NULL,
    PRIMARY KEY (assignment_name, student_id),
    FOREIGN KEY (student_id) REFERENCES student(student_id),
    FOREIGN KEY (assignment_name) REFERENCES assignment(name)
);
