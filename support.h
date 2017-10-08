#ifndef SUPPORT_H__
#define SUPPORT_H__

/*
 * Store information about the student who completed the assignment, to
 * simplify the grading process.  This is just a declaration.  The definition
 * is in student.c.
 */
extern struct student_t
{
	char *name;
	char *email;
} student;

/*
 * This function verifies that the student name is filled out
 */
void check_student(char *);

#endif
