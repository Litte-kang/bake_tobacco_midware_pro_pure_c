#include <stdio.h>
#include "json.h"

int main(int args, int argv[])
{
	FILE *fp					= NULL;
	char version[20]			= {0};
	struct json_object *my_json = NULL;
	struct json_object *my_obj 	= NULL;
	
	if (2 > args)
	{
		printf("please input a args!\n");
		return 1;
	}
		
	fp = fopen((char*)argv[1], "r");
	if (NULL == fp)
	{
		printf("open version faild!\n");
		return 1;
	}
	
	if (0 >= fscanf(fp, "%s", version))
	{
		printf("read version failed!\n");
		return 1;
	}
		
	my_json = json_tokener_parse(version);
	my_obj = json_object_object_get(my_json, "version");
	
	printf("%s", json_object_get_string(my_obj));
	
	json_object_put(my_obj);
	
	json_object_put(my_json);
	
	return 0;
}
