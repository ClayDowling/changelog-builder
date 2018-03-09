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
  cJSON_AddStringToObject(doc, "transactionId", "18122013");
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

int main(int argc, char **argv) {
  char *tablename = "TABLENAME";
  char *objectname = NULL;
  char testfilename[80];
  char jsonfilename[80];
  FILE *json = NULL;
  FILE *test = NULL;
  int opt_index = 0;
  int opt;

  struct option opts[] = {{"table", required_argument, 0, 't'},
                          {"object", required_argument, 0, 'o'},
                          {"update", no_argument, 0, 'u'}};

  while ((opt = getopt_long(argc, argv, "t:o:u", opts, &opt_index)) != -1) {
    switch (opt) {
    case 't':
      tablename = optarg;
      break;
    case 'o':
      objectname = optarg;
      break;
    case 'u':
      changetype = UPDATE;
      break;
    }
  }

  if (NULL == tablename) {
    strcpy(testfilename, "test.cs");
  } else {
    snprintf(testfilename, sizeof(testfilename), "%s.cs", tablename);
  }

  if (NULL == objectname) {
    strcpy(jsonfilename, "changelog.json");
  } else {
    snprintf(jsonfilename, sizeof(jsonfilename), "%s.json", objectname);
  }

  struct key_t *keys = read_list(stdin);

  json = fopen(jsonfilename, "w");
  test = fopen(testfilename, "w");

  generate_json(json, keys, tablename);
  generate_test(test, keys);

  return 0;
}