#include "cjson.h"
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_SIZE 40

enum changetype_t { INSERT, UPDATE, DELETE };

enum changetype_t changetype = INSERT;

struct key_t {
  char key[KEY_SIZE];
  char old[KEY_SIZE];
  char new[KEY_SIZE];
  struct key_t *next;
};

struct key_t *read_list(FILE *in) {
  char line[3 * KEY_SIZE + 7];
  char *token;
  char *first;
  char *second;
  char *third;
  struct key_t *TOP = NULL;
  struct key_t *mbr;

  while (!feof(in)) {
    fgets(line, sizeof(line), in);

    mbr = (struct key_t *)calloc(1, sizeof(struct key_t));
    first = NULL;
    second = NULL;
    third = NULL;
    first = strtok(line, ",\t");
    while ((token = strtok(NULL, ",\t"))) {
      if (NULL == second) {
        second = token;
      } else if (NULL == third) {
        third = token;
      }
    }
    strcpy(mbr->key, first);
    switch (changetype) {
    case INSERT:
      strcpy(mbr->new, second);
      break;
    case UPDATE:
      strcpy(mbr->old, second);
      strcpy(mbr->new, third);
      break;
    case DELETE:
      strcpy(mbr->old, second);
      break;
    }

    mbr->next = TOP;
    TOP = mbr;
  }

  return TOP;
}

void generate_test(FILE *out, struct key_t *TOP) {
  struct key_t *cur = TOP;

  while (cur) {
    fprintf(out, "Assert.Equal(\"%s\", tbl[\"%s\"].ToString());\n", cur->new,
            cur->key);
    cur = cur->next;
  }
}

void generate_json(FILE *out, struct key_t *TOP, char *tablename) {
  struct key_t *cur = TOP;

  cJSON *doc = cJSON_CreateObject();
  cJSON_AddStringToObject(doc, "utcDateTime", "2017-12-18T12:48:22.8535780");
  cJSON_AddNumberToObject(doc, "transactionId", 18122013);
  cJSON_AddStringToObject(doc, "operation", "I");
  cJSON_AddStringToObject(doc, "tableName", tablename);
  cJSON_AddStringToObject(doc, "schema", "C05DBSYNC");

  cJSON *uniqueKey = cJSON_CreateArray();
  cJSON_AddItemToObject(doc, "uniqueKey", uniqueKey);

  cJSON *key = cJSON_CreateObject();
  cJSON_AddStringToObject(key, "Name", "CourtType");
  cJSON_AddStringToObject(key, "dataType", "varchar");
  cJSON_AddNumberToObject(key, "length", 1);
  cJSON_AddNumberToObject(key, "precision", 0);
  cJSON_AddNumberToObject(key, "scale", 0);
  cJSON_AddItemToArray(uniqueKey, key);

  cJSON *columns = cJSON_AddArrayToObject(doc, "columns");
  while (cur) {
    cJSON *col = cJSON_CreateObject();
    cJSON_AddItemToArray(columns, col);

    cJSON_AddStringToObject(col, "name", cur->key);
    cJSON *val = cJSON_AddObjectToObject(col, "value");
    if (cur->old[0]) {
      cJSON_AddStringToObject(val, "old", cur->old);
    }
    if (cur->new[0]) {
      cJSON_AddStringToObject(val, "new", cur->new);
    }
    cJSON_AddStringToObject(col, "dataType", "varchar");
    cJSON_AddNumberToObject(col, "length", 3);
    cJSON_AddNumberToObject(col, "precision", 0);
    cJSON_AddNumberToObject(col, "scale", 0);
    cur = cur->next;
  }

  fputs(cJSON_Print(doc), out);

  cJSON_Delete(doc);
}

void help() {
  fputs(
      "usage: clbuilder [options] < source.csv\n"
      "\n"
      "Options:\n"
      "--table=name      Use table [name] in Json object, name.json, name.cs\n"
      "--insert          Create a Changelog for an insert operation (default)\n"
      "                    CSV file format:\n"
      "                      key,new\n"
      "--update          Create a Changelog for an update operation\n"
      "                    CSV file format:\n"
      "                      key,old,new\n"
      "--delete          Create a Changelog for a delete operation\n"
      "                    CSV file format:\n"
      "                      key,old\n",
      stderr);
}

int main(int argc, char **argv) {
  char *tablename = NULL;
  char testfilename[80];
  char jsonfilename[80];
  FILE *json = NULL;
  FILE *test = NULL;
  int opt_index = 0;
  int opt;

  struct option opts[] = {{"table", required_argument, 0, 't'},
                          {"insert", no_argument, 0, 'i'},
                          {"update", no_argument, 0, 'u'},
                          {"delete", no_argument, 0, 'd'}};

  while ((opt = getopt_long(argc, argv, "t:o:ud", opts, &opt_index)) != -1) {
    switch (opt) {
    case 't':
      tablename = optarg;
      break;
    case 'u':
      changetype = UPDATE;
      break;
    case 'd':
      changetype = DELETE;
      break;
    case 'i':
      changetype = INSERT;
      break;
    case '?':
      help();
      exit(EXIT_FAILURE);
    }
  }

  if (NULL == tablename) {
    strcpy(testfilename, "changelog.cs");
    strcpy(jsonfilename, "changelog.json");
    tablename = "INSERT_TABLENAME_HERE";
  } else {
    snprintf(testfilename, sizeof(testfilename), "%s.cs", tablename);
    snprintf(jsonfilename, sizeof(jsonfilename), "%s.json", tablename);
  }

  struct key_t *keys = read_list(stdin);

  json = fopen(jsonfilename, "w");
  test = fopen(testfilename, "w");

  generate_json(json, keys, tablename);
  generate_test(test, keys);

  return EXIT_SUCCESS;
}