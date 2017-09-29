#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "support.h"

/*
 * Make sure that the student name and email fields are not empty.
 */
void check_student(char * progname)
{
	if((strcmp("", student.name) == 0) || (strcmp("", student.email) == 0))
	{
		printf("%s: Please fill in the student struct in student.c\n", progname);
		exit(1);
	}
	printf("Student: %s\n", student.name);
	printf("Email  : %s\n", student.email);
	printf("\n");
}
