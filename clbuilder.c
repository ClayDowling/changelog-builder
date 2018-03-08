#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_SIZE 40

struct key_t {
  char key[KEY_SIZE];
  char value[KEY_SIZE];
  struct key_t *next;
};

struct key_t *read_list(FILE *in) {
  char line[2 * KEY_SIZE + 5];
  char *comma;
  char *trail;

  struct key_t *TOP = NULL;

  while (!feof(in)) {
    fgets(line, sizeof(line), in);
    comma = strchr(line, ',');
    if (comma) {
      struct key_t *mbr = (struct key_t *)calloc(1, sizeof(struct key_t));
      strncpy(mbr->key, line, comma - line);
      strncpy(mbr->value, comma + 1, strlen(comma) - 1);

      trail = &mbr->value[strlen(mbr->value) - 1];
      if (isspace(*trail))
        *trail = 0;

      mbr->next = TOP;
      TOP = mbr;
    }
  }

  return TOP;
}

void generate_test(FILE *out, struct key_t *TOP) {
  struct key_t *cur = TOP;

  while (cur) {
    fprintf(out, "Assert.Equal(\"%s\", tbl[\"%s\"].ToString());\n", cur->value,
            cur->key);
    cur = cur->next;
  }
}

void generate_json(FILE *out, struct key_t *TOP, char *tablename) {
  struct key_t *cur = TOP;

  fprintf(out,
          "{\n"
          "    'utcDateTime': '2017-12-18T12:48:22.8535780',\n"
          "    'transactionId': 18122013,\n"
          "    'operation': 'I',\n"
          "    'tableName': '%s',\n"
          "    'schema': 'C05DBSYNC',\n"
          "    'uniqueKey': [\n"
          "        {\n"
          "            'Name': 'CourtType',\n"
          "            'dataType': 'varchar',\n"
          "            'length': 1,\n"
          "            'precision': 0,\n"
          "            'scale': 0\n"
          "        }\n"
          "    ],\n"
          "    'columns': [\n",
          tablename);
  while (cur) {
    fprintf(out,
            "    {\n"
            "        'name': '%s',\n"
            "        'value': {\n"
            "            'new': '%s'\n"
            "        },\n"
            "        'dataType': 'varchar',\n"
            "        'length': 3,\n"
            "        'precision': 0,\n"
            "        'scale': 0\n"
            "    },\n",
            cur->key, cur->value);
    cur = cur->next;
  }
  fputs("   ]\n}\n", out);
}

int main(int argc, char **argv) {
  char *tablename = "TABLENAME";
  char *objectname = NULL;
  char testfilename[80];
  char jsonfilename[80];
  FILE *json = NULL;
  FILE *test = NULL;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--table") == 0) {
      tablename = argv[i + 1];
    }
    if (strcmp(argv[i], "--object") == 0) {
      objectname = argv[i + 1];
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